#pragma once
#include "CircularBuffer.hpp"

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/atomic.hpp>




using boost::asio::ip::tcp;

#define HEADER						"\xAA\x55"		/* AA55 */
#define TAIL						"\x55\xAA"		/* 55AA*/

#define RECV_BUFFER_SIZE			16384	/* bytes */
#define MAX_TRANSMISSION_AMOUNT	    16384	/* bytes */
#define MAX_INDIVIDUAL_PACKET_SIZE  16384	/* bytes */
#define MIN_RECV_TRANSFER_AMOUNT    7		/* bytes */
#define RECEIVE_POOL_AMOUNT         32767   /* bytes */

#define USE_KEEP_ALIVE
#define USE_TCP_NODELAY


#define INTERNAL_SEND_BUFFER_SIZE 16384
#define INTERNAL_RECV_BUFFER_SIZE 16384

#if defined TEST_MODE
	#define FLOOD_PACKET_SIZE 16384
	#define FLOOD_WAIT        1 /* milliseconds */
#endif


class Client : public boost::enable_shared_from_this<Client>
{
public:
	typedef boost::shared_ptr<Client> pointer;
	~Client();
	static pointer create(boost::asio::io_service& io_service, void * mgr,std::string& ip,std::string& sPort,bool connectImmediately = true,int reconnectAttempt = 0, int reconnectInterval = 5000)
	{
		pointer ptr = pointer(new Client(io_service));
		ptr->m_readBuffer.Allocate(RECV_BUFFER_SIZE);
		ptr->m_readBuffer.Remove(RECV_BUFFER_SIZE);
		ptr->mgrPtr = mgr;
		ptr->m_ulSessionID = 0;
		ptr->szServerIP = ip;
		ptr->serverp_ = sPort;
		ptr->isDisconnected = true;
		ptr->reconnectAttempt = reconnectAttempt;
		ptr->reconnectInterval = reconnectInterval;
		
		if (connectImmediately)
			ptr->Connect();
		return ptr;
	}

	void Connect();
	void OnConnect(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator);
	void Disconnect();
	void Initialize();


	/* Returns client's IP address */
	std::string GetLocalIPAddress() const { return local_; }
	/* Returns IP address of connected server */
	std::string GetRemoteIPAddress() const { return remote_; }
	/* Returns client's port */
	unsigned short GetLocalPort() const { return localp_; }
	/* Returns connected server's port */
	unsigned short GetRemotePort() const { return remotep_; }
	unsigned long long GetSessionID() const { return m_ulSessionID; }
	tcp::socket& socket() { return socket_; }
	void * Manager(){ return mgrPtr; }
private:
	/*
		Socket.
	*/
	tcp::socket socket_;
	tcp::resolver resolver_;
	boost::asio::deadline_timer t_flood , t_reconnect;

	boost::atomic<int> reconnectAttempt;
	int reconnectInterval;

	/*
	Identifier of this session.
	It is assigned by owner server and it should not be
	altered anywhere.(otherwise things might go crazy.)
	*/
	unsigned long long m_ulSessionID;

	std::string szServerIP;
	std::string serverp_;
	/* Variables to store IP addresses */
	std::string local_, remote_;
	/* Variables to store port values */
	unsigned short localp_, remotep_;

	void * mgrPtr;

	/* Indicates if client is disconnected. */
	boost::atomic<bool> isDisconnected;

	/*
	A mutex for the read event.
	It is claimed by any thread that completes a read event.
	Other pending read event should wait until current thread completes
	its' job.
	*/
	boost::recursive_mutex readMutex;

	/* Ring buffer for read operation */
	CircularBuffer m_readBuffer;

	/* Constructor */
	Client(boost::asio::io_service& io_service) : socket_(io_service),resolver_(io_service),t_flood(io_service),t_reconnect(io_service) {}
	Client();

	void OnResolve(const boost::system::error_code& err, tcp::resolver::iterator endpoint_iterator);
	/* Invoked when an asynchronous write operation is completed */
	void OnWriteCompleted(const boost::system::error_code&, size_t s);
	/* Invoked when an asynchronous read operation is completed */
	void OnReadCompleted(const boost::system::error_code& err, std::size_t bytes_transferred);

	/* Dispatches the parse event. */
	void DispatchParse(size_t rSize);
	/* Issues another asynchronous read operation to fill the read buffer (if there's any data available.) */
	void DispatchRead();

	void SetSocketOptions();

	void Send(void * buf, size_t len);

	void ReconnectWait();
	void OnReconnectWaitEnd();

	#if defined TEST_MODE
		void DispatchFlood();
		void FloodWait();
		void OnFloodWaitEnd();
	#endif

};

