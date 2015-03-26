#include "Client.hpp"
#include "ClientManager.hpp"

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



Client::~Client()
{
}

void Client::Initialize()
{
	SetSocketOptions();
	local_ = socket_.local_endpoint().address().to_string();
	remote_ = socket_.remote_endpoint().address().to_string();
	localp_ = socket_.local_endpoint().port();
	remotep_ = socket_.remote_endpoint().port();

	/* Generate ID for this session */
	char buf[256];

	std::string withoutDots = ReplaceString(local_, ".", "");
	sprintf_s(buf, "%s%hu", withoutDots.c_str(), localp_);

	m_ulSessionID = _atoi64(buf);

	printf("> session_connect  \n	(Client) ip : %s, port : %hu \n	(Server) ip : %s, port : %hu, sid : %llu \n<\n", GetLocalIPAddress().c_str(), GetLocalPort(), GetRemoteIPAddress().c_str(), GetRemotePort(), GetSessionID());
}

void Client::SetSocketOptions()
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

void Client::Connect()
{
	tcp::resolver::query query(tcp::v4(), (char*)szServerIP.c_str(), serverp_);
	tcp::resolver::iterator iterator = resolver_.resolve(query);

	resolver_.async_resolve(query,
		boost::bind(&Client::OnResolve, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::iterator));
}

void Client::OnConnect(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator)
{
	if (!err)
	{
		/* 
			We're not disconnected anymore. 
			Set the flag.
		*/
		isDisconnected = false;
		// Connection was successful.
		Initialize();
		/* 
			Dispatch the read operation 
		*/
		DispatchRead();
		
		#if defined TEST_MODE
			DispatchFlood();
		#endif
	}
	else if (endpoint_iterator != tcp::resolver::iterator())
	{
		// The connection failed. Try the next endpoint in the list.
		socket_.close();
		tcp::endpoint endpoint = *endpoint_iterator;
		socket_.async_connect(endpoint,
			boost::bind(&Client::OnConnect, shared_from_this(),
			boost::asio::placeholders::error, ++endpoint_iterator));
	}
	else
	{
		/* 
			Check if automatic reconnect is enabled.
			If so, call the reconnect wait function to try connecting again.
		*/
		if (reconnectAttempt > 0)
			ReconnectWait();
		/*
			Otherwise, remove this client from manager, and free the memory.
		*/
		else
			Disconnect();

	}
	
}

void Client::ReconnectWait()
{
	/* 
		If we're running out of attempts, we should disconnect.
	*/
	if (--reconnectAttempt == 0)
		Disconnect();

	/*
		Set the expiration time for reconnect timer
	*/
	t_reconnect.expires_from_now(boost::posix_time::milliseconds(reconnectInterval));
	/* 
		Wait until reconnect interval expires 
	*/
	t_reconnect.async_wait(boost::bind(&Client::OnReconnectWaitEnd, shared_from_this()));
}

void Client::OnReconnectWaitEnd()
{
	/* Call connect again */
	Connect();
}

void Client::OnResolve(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator)
{
	if (!err)
	{
		/* 
			Fetch the first endpoint we've just resolved.
		*/
		tcp::endpoint endpoint = *endpoint_iterator;
		/* 
			Try to connect to the endpoint.

			NOTE: If we've got more than one endpoint, the others will be tried next
			if current endpoint fails to connect.
		*/
		socket_.async_connect(endpoint, boost::bind(&Client::OnConnect, shared_from_this(),
			boost::asio::placeholders::error,
			++endpoint_iterator));
	}
	else
	{
		std::cout << "[onresolve]Error: " << err.message() << "\n";
	}
		return;
}
#if defined TEST_MODE
/*
	Send null data chunks to the server.

	This is *just here for* testing purposes.
*/
	void Client::DispatchFlood()
	{
		const int val = FLOOD_PACKET_SIZE - 8;
		static unsigned char * packet = new unsigned char[FLOOD_PACKET_SIZE];
		packet[0] = 0xAA;
		packet[1] = 0x55;
		memcpy(&packet[2], &val, 4);
		packet[FLOOD_PACKET_SIZE - 2] = 0x55;
		packet[FLOOD_PACKET_SIZE - 1] = 0xAA;

		Send(packet, FLOOD_PACKET_SIZE);
	}
#endif

void Client::DispatchRead()
{
	/* 
		Read next chunk into the read buffer
	*/
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
		&Client::OnReadCompleted,
		shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
		)
		);
}

void Client::DispatchParse(size_t rSize)
{
	if (isDisconnected)
		return;

	unsigned char first_chunk[6];
	unsigned char head[2], tail[2], tmpSize[4];
	unsigned char core[MAX_INDIVIDUAL_PACKET_SIZE];

	int size = 0;

	// We need at least 7 byte(s) to issue a parse.
	if (m_readBuffer.GetSize() < MIN_RECV_TRANSFER_AMOUNT)
		return;

	memcpy(&first_chunk, m_readBuffer.GetBufferStart(), 6);
	memcpy(&head, &first_chunk, 2);
	memcpy(&tmpSize, &first_chunk[2], 4);

	// Check the packet header
	if (memcmp(head, (unsigned char*)(HEADER), 2) == 0)
	{
		// Read the size of the content

		size = ((int*)tmpSize)[0];
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

		if (m_readBuffer.GetSize() >= (unsigned int)(8 + size))
		{
			m_readBuffer.Remove(6);
			// Read the message to the new allocated buffer
			m_readBuffer.Read(&core, size);
			// Read the tail
			m_readBuffer.Read((char*)tail, 2);

			if (memcmp(tail, (unsigned char*)(TAIL), 2) == 0)
			{
				// Valid packet.
				// TO-DO : Process it.
			} // EOF tail check
		} // EOF size > 0
		else
		{
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
		printf("header mismatch.%x,%x\n", head[0], head[1]);
		Disconnect();
	}
}

/*
	Sends given buffer to the connected endpoint.
	@params
	void * buf			The buffer to send.
	size_t len			The length of the buffer.
*/
void Client::Send(void * buf, size_t len)
{
	boost::asio::async_write
		(
		socket_,
		boost::asio::buffer(buf,len),
		// Bind callback function
		boost::bind
		(
		&Client::OnWriteCompleted,
		shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred)
		);
}

void Client::OnWriteCompleted(const boost::system::error_code&, size_t s)
{
	#if defined TEST_MODE
		t_flood.expires_from_now(boost::posix_time::milliseconds(FLOOD_WAIT));
		t_flood.async_wait(boost::bind(&Client::OnFloodWaitEnd, shared_from_this()));
	#endif
}

#if defined TEST_MODE
	void Client::OnFloodWaitEnd()
	{
		DispatchFlood();
	}
#endif

void Client::OnReadCompleted(const boost::system::error_code& err, size_t s)
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
		//((Server*)serverPtr)->OnReceive(s);

		// Dispatch the read event again
		if (!isDisconnected)
			DispatchRead();
	}
}

void Client::Disconnect()
{
	boost::unique_lock<boost::recursive_mutex> scoped_lock(readMutex);
	{
		if (isDisconnected)
			return;
		isDisconnected = true;
		socket_.close();

		/* Inform the manager */
		((ClientManager*)mgrPtr)->OnClientDisconnect(shared_from_this());
	}
}
