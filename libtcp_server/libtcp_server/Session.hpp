
/*

	DISCLAIMER
	This software can be used freely under GNU/LGPL license.

	File    : Session.h
	Author  : Mustafa K. GILOR
	Purpose : Session class for the libtcp server.
	All individual session info are stored in this class.
	It is automatically destroyed when last shared_ptr instance is released.

*/

#pragma once

#pragma region Includes

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/atomic.hpp>
#include <boost/noncopyable.hpp>


#include "CircularBuffer.hpp"
#include "Session_Configuration.hpp"

using boost::asio::ip::tcp;


#pragma endregion


class Session : public boost::enable_shared_from_this<Session>, public boost::noncopyable
{
public:
	typedef boost::shared_ptr<Session> pointer;

	/* 
		Invoked when client is connected 
	*/
	void OnConnect();

	/*
		Disconnects the session from connected endpoint.
	*/
	void Disconnect();
	/*
		Initializes the variables required for session.
	*/
	void Initialize();
	/*
		Keep alive system callback.
	*/
	void CheckAlive();

	/* 
		Inheriting class should override this method by this signature : 
				void OnDataReceived(char *, size_t) override;
	*/
	virtual void OnDataReceived(char *, size_t) = 0;
	virtual void OnConnected() = 0;
	virtual void OnDisconnected() = 0;

	/* 
		Returns server's IP address 
	*/
	std::string GetLocalIPAddress()   const     { return local_; }
	/*
		Returns IP address of connected client
	*/
	std::string GetRemoteIPAddress()  const     { return remote_; }
	/* 
		Returns server's port 
	*/
	unsigned short GetLocalPort()     const     { return localp_; }
	/* 
		Returns connected client's port 
	*/
	unsigned short GetRemotePort()    const     { return remotep_; }
	/* 
		Returns the ID assigned to this session
	*/
	unsigned long long GetSessionID() const     { return m_ulSessionID; }
	/* 
		Returns the socket object 
	*/
	tcp::socket& socket()                       { return socket_; }
protected:
	/* 
		Default constructor (protected) 
	*/
	Session(boost::asio::io_service& io_service, void * srv) :
	socket_(io_service),serverPtr(srv),fragment_count(0),
	isDisconnected(false),m_ulSessionID(0)
	{
		m_readBuffer.Allocate(RECV_BUFFER_SIZE);
		m_readBuffer.Remove(RECV_BUFFER_SIZE);
	}

	/*
	Sends given buffer to the connected endpoint.
	*/
	void Send(void * buf, size_t len);
private:
	/*
	Socket
	*/
	tcp::socket socket_;
	/*
	Ring buffer for read operation
	*/
	CircularBuffer m_readBuffer;
	/*
	A mutex for the read event.
	It is claimed by any thread that completes a read event.
	Other pending read event should wait until current thread completes
	its' job.
	*/
	boost::recursive_mutex readMutex;
	/*
		Last data receive time.
	*/
	boost::atomic<unsigned long long> m_aulLastReceiveTime;
	/*
		Total received data amount (bytes)
	*/
	boost::atomic<unsigned long long> m_aulReceiveCount;
	/*
		Previous total received data amount (bytes)
	*/
	boost::atomic<unsigned long long> m_aulPrevReceiveCount;
	/*
	Maximum count of fragmented packet(s).
	If a session exceeds the MAX_PACKET_FRAGMENT_COUNT value
	it will be disconnected eventually.
	*/
	boost::atomic<int> fragment_count;
	/*
	Indicates if client is disconnected.
	*/
	boost::atomic<bool> isDisconnected;
	/*
	Identifier of this session.
	It is assigned by owner server and it should not be
	altered anywhere.(otherwise things might go crazy.)
	*/
	unsigned long long m_ulSessionID;
	/* 
		Pointer to the owner server 
	*/
	void * serverPtr;
	/* 
		Variables to store IP addresses 
	*/
	std::string local_, remote_;
	/* 
		Variables to store port values 
	*/
	unsigned short localp_, remotep_;
	


	/* 
		Sets defined socket options for this session. 
	*/
	void SetSocketOptions();
	/* 
		Invoked when an asynchronous write operation is completed 
	*/
	void OnWriteCompleted(const boost::system::error_code&, size_t s);
	/* 
		Invoked when an asynchronous read operation is completed
	*/
	void OnReadCompleted(const boost::system::error_code& err, std::size_t bytes_transferred);
	/* 
		Dispatches the parse event.
	*/
	void DispatchParse(size_t rSize);
	/* 
		Issues another asynchronous read operation to fill the read buffer (if there's any data available.) 
	*/
	void DispatchRead();
};




