#include "../Headers/stdafx.h"
#include "../Headers/Session.hpp"
#include "../Headers/Server.hpp"



std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}



void Session::Initialize()
{

	SetSocketOptions();

	local_ = socket_.local_endpoint().address().to_string();
	remote_ = socket_.remote_endpoint().address().to_string();
	localp_ = socket_.local_endpoint().port();
	remotep_ = socket_.remote_endpoint().port();

	m_aulLastReceiveTime.store(GetTickCount64());
	m_aulReceiveCount.store(0);
	m_aulPrevReceiveCount.store(0);

	/* Generate ID for this session */
	char buf[256];

	std::string withoutDots = ReplaceString(remote_, ".", "");
	sprintf_s(buf, "%s%hu", withoutDots.c_str(), remotep_);

	m_ulSessionID = _atoi64(buf);

}

void Session::SetSocketOptions()
{
	socket_.set_option(boost::asio::socket_base::send_buffer_size(INTERNAL_SEND_BUFFER_SIZE));
	socket_.set_option(boost::asio::socket_base::receive_buffer_size(INTERNAL_RECV_BUFFER_SIZE));

#ifdef USE_KEEP_ALIVE
	socket_.set_option(boost::asio::socket_base::keep_alive(true));
#endif

#ifdef USE_TCP_NODELAY
	socket_.set_option(tcp::no_delay(true));
#endif
}



void Session::CheckAlive()
{
	if (isDisconnected)
	{
		// printf("disconnected, but lingering..\n");
		((Server<Session>*)serverPtr)->OnBrokenSession(shared_from_this());
		return;
	}
}

/*
	Sends given buffer to the connected endpoint.
	@params
	void * buf			The buffer to send.
	size_t len			The length of the buffer.
*/
void Session::Send(void * buf, size_t len)
{
	boost::asio::async_write
		(
		socket_,
		boost::asio::buffer(buf, len),
		// Bind callback function
		boost::bind
		(
		&Session::OnWriteCompleted,
		shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred)
		);
}



void Session::DispatchRead()
{
	if (isDisconnected)
		return;

	unsigned long long curTick = GetTickCount64();

	/*
	Auto kick sessions which are inactive
	*/
#ifdef AUTOKICK_INACTIVE_SESSIONS
	unsigned long long diff = curTick - m_aulLastReceiveTime;
	if (diff >= MAX_NO_RESPONSE_PERIOD)
	{
		printf("> socket_dispatchread \n	socket is inactive for [ %llu ] seconds\n	disconnecting..\n>\n", diff / 1000);
		Disconnect();
		return;
	}

#endif

	/*
	Auto kick flooding sessions
	*/
#if 0 /*AUTOKICK_ON_EXCESSIVE_TRANSMISSION*/
	/* TO-DO : It spends way too much unneccessary CPU time.*/
	/* The old method*/
	if (socket_.available() > MAX_TRANSMISSION_AMOUNT)
	{
		printf("> socket_dispatchread \n	someone's probably flooding(avail too high) [ %u ]\n	disconnecting..\n>\n", socket_.available());
		Disconnect();
		return;
	}
#endif


	m_aulReceiveCount++;

	if (curTick - m_aulLastReceiveTime >= 1000)
	{
#ifdef AUTOKICK_ON_EXCESSIVE_RECV
		if (m_aulReceiveCount - m_aulPrevReceiveCount > MAX_RECV_COUNT_PER_SEC)
		{
			printf("> socket_dispatchread \n	someone's probably flooding(packet per second) [ %d ]\n	disconnecting..\n>\n", m_aulReceiveCount - m_aulPrevReceiveCount);
			Disconnect();
			return;
		}
#endif
		m_aulPrevReceiveCount.store(m_aulReceiveCount);
		m_aulLastReceiveTime.store(GetTickCount64());
	}




	boost::asio::async_read
		(
		// Socket to read from
		socket_,
		// Buffer
		boost::asio::buffer(m_readBuffer.GetBuffer(), m_readBuffer.GetSpace()),
		// Minimum transfer amount
		boost::asio::transfer_at_least(MIN_RECV_TRANSFER_AMOUNT),
		boost::bind
		(
		&Session::OnReadCompleted,
		shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
		)
		);
}

#pragma warning(suppress: 6262)
void Session::DispatchParse(size_t rSize)
{
	if (isDisconnected)
		return;

	#ifdef AUTOKICK_ON_EXCESSIVE_TRANSMISSION
	/* The new implementation */
	if (rSize > MAX_TRANSMISSION_AMOUNT)
	{
		printf("> socket_dispatchparse \n	someone's probably flooding(avail too high) [ %u ]\n	disconnecting..\n>\n", rSize);
		Disconnect();
		return;
	}

	#endif

	unsigned char first_chunk[2 + SIZE_TYPE];
	unsigned char head[2], tail[2], tmpSize[SIZE_TYPE];
    char core[MAX_INDIVIDUAL_PACKET_SIZE];

	__int64 size = 0;

	// We need at least 7 byte(s) to issue a parse.
	if (m_readBuffer.GetSize() < MIN_RECV_TRANSFER_AMOUNT)
		return;

	memcpy(&first_chunk, m_readBuffer.GetBufferStart(), 2 + SIZE_TYPE);
	memcpy(&head, &first_chunk, 2);
	memcpy(&tmpSize, &first_chunk[2], SIZE_TYPE);

	// Check the packet header
	if (memcmp(head, (unsigned char*)(HEADER), 2) == 0)
	{
		// Read the size of the content

		switch (SIZE_TYPE)
		{
		case 2:
			size = ((short*)tmpSize)[0];
			break;
		case 4:
			size = ((__int32*)tmpSize)[0];
			break;
		case 8:
			size = ((__int64*)tmpSize)[0];
			break;
		}
		//printf("size :%d\n", m_readBuffer.GetSize());
		// Confirm that content size + header + 4 + tail matches received amount.
		// Confirm that size is not minus.
		if (size <= 0 || size > MAX_INDIVIDUAL_PACKET_SIZE)
		{
			// That means something's wrong with this packet.
			// We should disconnect this session.
			printf("< dispatch_parse\n	session [%s:%hu]\n	either content size is zero or exceeds the maximum amount.\n	disconnecting..\n>\n", GetRemoteIPAddress().c_str(), GetRemotePort());
			Disconnect();
		}

		if (m_readBuffer.GetSize() >= (unsigned int)(4 + SIZE_TYPE + size))
		{
			m_readBuffer.Remove(2 + SIZE_TYPE);
			// Read the message to the new allocated buffer
			m_readBuffer.Read(&core, size);
			// Read the tail
			m_readBuffer.Read((char*)tail, 2);

			if (memcmp(tail, (unsigned char*)(TAIL), 2) == 0)
			{
				// Valid packet.
				// TO-DO : Process it.
				OnDataReceived(&core[0], size);
			} // EOF tail check
		} // EOF size > 0
		else
		{
			if (++fragment_count == MAX_PACKET_FRAGMENT_COUNT)
			{
				printf("< dispatch_parse \n	fragmentation level exceeded maximum value.\n	disconnecting..\n>\n");
				Disconnect();
				return;
			}
			else
				return;
		}

		/*

		Else, it means we don't have the whole data yet.
		But, it should never happen.

		We're using async_read_until to read data from socket, so incoming data
		always must have "the tail".


		*/
	}
	else
	{
		printf("< session_dispatchparse\n	Header mismatch.\n	Value : [%x,%x]\n>\n", head[0], head[1]);
		Disconnect();
	}
}

void Session::OnConnect()
{
	// Start reading..
	DispatchRead();
	/* 
		Call the virtual function to inform inherited class
	*/
	OnConnected();
}



void Session::OnWriteCompleted(const boost::system::error_code&, size_t s)
{
	//	TO-DO: Implement this.
	((Server<Session>*)serverPtr)->OnSend(s);
}

void Session::OnReadCompleted(const boost::system::error_code& err, size_t s)
{
	// scoped lock
	{
		boost::unique_lock<boost::recursive_mutex> scoped_lock(readMutex);

		switch (err.value())
		{
		case ERROR_SUCCESS:
		case boost::asio::error::in_progress:
		case boost::asio::error::operation_aborted:
			break;
			// Operation aborted error only occurs when socket is closed by us.
			return;
		default:
			Disconnect();
			return;
		}

		// Commit the data
		m_readBuffer.IncrementWritten(s);

		// Parse incoming data
		DispatchParse(s);

		// Update the recv amount..
		((Server<Session>*)serverPtr)->OnReceive(s);

		// Dispatch the read event again
		if (!isDisconnected)
			DispatchRead();
	}
}

void Session::Disconnect()
{
	boost::unique_lock<boost::recursive_mutex> scoped_lock(readMutex);
	{
		
		if (isDisconnected)
			return;
		isDisconnected = true;
		socket_.close();
		/*
			Call the virtual function.
		*/
		OnDisconnected();
		((Server<Session>*)serverPtr)->OnSessionDisconnect(shared_from_this());
	}
}



