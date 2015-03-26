#pragma once

/*

File        : Server.hpp
Author      : Mustafa K. GILOR
Description :

| ASIO Server Wrapper |

A simple wrapper for Boost ASIO library to handle asynchronous connections.
Currently only supports TCP connections.

Usage :

boost::asio::io_service io;
Server server(io,3333);
io.run();

* Replace '3333' with desired listen port.

Author		: Mustafa K. GILOR
Last rev.	: 14/03/2015

*/

#pragma region Includes 


#include "Session.hpp"
#include "IOServicePool.hpp"
#include "Server_Configuration.hpp"

#ifdef REPORT_MEMORY_USAGE
	#include "../Misc/MemUsage.h"
#endif

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost\asio.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/noncopyable.hpp>

#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/mem_fn.hpp>
#include <boost/foreach.hpp>
#include <boost/atomic.hpp>

#include <map>
#include <set>

using boost::asio::ip::tcp;
using std::map;
using std::set;


typedef boost::atomic<unsigned long long> atomic_ulonglong;





#pragma endregion


#pragma region Declaration

template <class T>
class Server : boost::noncopyable
{
public:
	typedef std::map<unsigned long long, boost::shared_ptr<T>> SessionMap;
	typedef std::set<boost::shared_ptr<T>> SessionSet;
	/* 
		Constructor 
	*/
	Server(const unsigned short sPort);
	/*
		Generic destructor 
	*/
	~Server();
	/*
		Start the server.
	*/
	void Run();
	/*
		Removes the given session from the session list.
		This is intended to be invoked by individual session.
	*/
	void OnSessionDisconnect(boost::shared_ptr<T> dConnection);
	/*
		Declared for keep alive service usage.
	*/
	void OnBrokenSession(boost::shared_ptr<T> dConnection);
	/*
		Updates the amount of total received bytes by given amount.
	*/
	void OnReceive(const size_t amount) 
	{
		aulTotalRecv.store(aulTotalRecv.load() + amount);
		if (GetTickCount64() - ul_recvtime >= 1000)
		{
			ul_recvtime = GetTickCount64();
			fl_downrate.store((double)(aulTotalRecv - aulPrevTotalRecv));
			aulPrevTotalRecv.store(aulTotalRecv.load());
		}
		
	};
	/*
		Updates the amount of total sent bytes by given amount.
	*/
	void OnSend(const size_t amount)
	{
		aulTotalSend += amount;

		if (GetTickCount64() - ul_sendtime >= 1000)
		{
			ul_sendtime = GetTickCount64();
			fl_uprate.store((double)(aulTotalSend - aulPrevTotalSend));
			aulPrevTotalSend.store(aulTotalSend.load());
		}
	};


	/*
		The number of active session(s).
	*/
	size_t GetSessionCount() const { return arrSessions.size(); }

	/* 
		Returns receive amount in bytes-per-second form.
	*/
	double GetCurrentReceiveAmount() const { return fl_downrate; }
	/*
		Returns send amount in bytes-per-second form.
	*/
	double GetCurrentSendAmount()    const{ return fl_uprate; }

private:


	/* 
		The IO service pool for server.
		Default initialization value depends on CPU core amount.
	*/
	io_service_pool io_service_pool_;

	/* 
		The listening port.
	*/
	unsigned __int16 m_sPort;

	/* 
		The timer(s) used to check the socket keep alive and update internal server stats.
	*/
	boost::asio::deadline_timer t_keepalive, t_statupdate;

	/*
		The last receive / send update time. (for internal stats)
	*/
	unsigned long long ul_recvtime, ul_sendtime;

	/*
		Total received bytes since server started.
		(prev is for stat calculation)
	*/
	atomic_ulonglong aulTotalRecv, aulPrevTotalRecv;

	/*
		Total received sent since server started.
		(prev is for stat calculation)
	*/
	atomic_ulonglong aulTotalSend, aulPrevTotalSend;

	/* 
		Amount of bytes being received & sent by server (per second) 
	*/
	boost::atomic<double> fl_downrate, fl_uprate;

	/* 
		The session map.
		Map key is session key (which is generated according to IP & Port
		Map value is session.
	*/
	SessionMap arrSessions;

	/*
		Session map mutex.

		This is here for preventing any race conditions
		between threads.

		NOTE : It must be acquired first before make any read / write operations
		on session map. Otherwise session map might be get corrupt.
	*/
	mutable boost::recursive_mutex sessionArrayLock;

	/*
		The temporary storage declaration for broken sessions that
		are detected by keep alive service.

		The session(s) that exist in this set will be automatically
		removed from the session map on next iteration.
	*/
	SessionSet arrSessionCleanup;
	
	/* 
		Acceptor for incoming connections 
	*/
	tcp::acceptor acceptor_;

	#ifdef REPORT_MEMORY_USAGE
		MemUsage mem_usage;
	#endif

	/* 
		Server starts accepting connection(s) when called 
	*/
	void StartAccepting();

	/* 
		Invoked when an accept operation is completed 
	*/
	void OnSessionConnect(boost::shared_ptr<T>, const boost::system::error_code&);

	/* 
		Dispatches the keep alive service 
	*/
	void DispatchKeepAlive();
	void OnKeepAlive();

	/*
		Dispatches the stat update service
	*/
	void DispatchStatUpdate();
	void OnStatUpdate();

};

#pragma endregion

#pragma region Implementation

/*
CTOR
*/
template <class T>
Server<T>::Server(unsigned short sPort)
#pragma region Initialize

try :
io_service_pool_(8),
acceptor_(io_service_pool_.get_io_service(), tcp::endpoint(tcp::v4(), sPort), false),
t_keepalive(io_service_pool_.get_io_service()),
t_statupdate(io_service_pool_.get_io_service()),
m_sPort(sPort),
ul_recvtime(0), ul_sendtime(0),
fl_downrate(0.0), fl_uprate(0.0)

#pragma endregion
{
	printf("libtcp_win ver 0.1\n");
	#pragma region Print debug information 
	#ifdef _DEBUG 
		printf("server_config_report (debug)\n");
		printf("	KEEP_ALIVE_INTERVAL		[%d]\n", KEEP_ALIVE_INTERVAL);
		printf("	STAT_UPDATE_INTERVAL		[%d]\n", STAT_UPDATE_INTERVAL);
		printf("	MAXIMUM_PENDING_CONNECTIONS	[%d]\n", MAXIMUM_PENDING_CONNECTIONS);
		printf("	MAXIMUM_ALLOWED_CONNECTION	[%d]\n", MAXIMUM_ALLOWED_CONNECTION);
	#endif
	#pragma endregion

	StartAccepting();
	/*
		Dispatch the keep alive service in order to
		track disconnected or somehow broken session(s)
		*/
	DispatchKeepAlive();
	/*
		Stat update service, which keeps track of
		internal counters for send/recv operations.
		*/
	DispatchStatUpdate();
} catch (...) { throw; }

/*
DTOR
*/
template <class T>
Server<T>::~Server()
{
	/* Stop accepting new connections */
	acceptor_.close();

	{
		boost::recursive_mutex::scoped_lock lock(sessionArrayLock);
		// Disconnect all session(s)
		BOOST_FOREACH(SessionMap::value_type &ptr, arrSessions)
		{
			ptr.second->Disconnect();
		}

		arrSessions.clear();
	}
	/* Stop running io service(s) */
	io_service_pool_.stop();
}


template <class T>
void Server<T>::Run()
{
	io_service_pool_.run();
}

template <class T>
void Server<T>::StartAccepting()
{
	boost::shared_ptr<T> new_connection = T::create(io_service_pool_.get_io_service(), this);
	acceptor_.async_accept
		(
		/* Socket to copy connected client */
		new_connection->socket(),
		/* After-connection stuff */
		boost::bind
		(
		/* Function to call after connection */
		&Server::OnSessionConnect,
		/* Server */
		this,
		/* Client */
		new_connection,
		/* */
		boost::asio::placeholders::error
		)
		);
}


#pragma region Session connect / disconnect handlers

template <class T>
void Server<T>::OnSessionConnect(boost::shared_ptr<T> new_connection, const boost::system::error_code& error)
{
	/* As we're using shared pointers for Sessions,
	we don't need to worry about dropped connections
	or connection errors.

	(broken session will be automatically deleted
	when this function goes out of the scope
	as the last reference exist in this function).
	*/
	if (error)
		/* TODO : Determine the error and print a detailed output message to log file. */
		printf("> session_connect  \n	failed.\n<\n");
	else
	{
		/* Initialize the new connection */
		new_connection->Initialize();
		printf("> session_connect  \n	(S) ip : %s, port : %hu \n	(R) ip : %s, port : %hu, sid : %llu \n<\n", new_connection->GetLocalIPAddress().c_str(), new_connection->GetLocalPort(), new_connection->GetRemoteIPAddress().c_str(), new_connection->GetRemotePort(), new_connection->GetSessionID());
		/* Inform session that it's connected properly */
		new_connection->OnConnect();
		{
			/* Acquire the session map mutex */
			boost::recursive_mutex::scoped_lock lock(sessionArrayLock);
			if (new_connection->GetSessionID() == 0)
			{
				/* NOTE: This should never happen.
				If it happens, we need to investigate why.
				*/
				ASSERT(1);
				return;
			}
			/* Insert the session to the session map */
			arrSessions.insert(std::make_pair(new_connection->GetSessionID(), new_connection));
		}

	}

	/* Re-invoke accept operation */
	StartAccepting();
}

template <class T>
void Server<T>::OnSessionDisconnect(boost::shared_ptr<T> dConnection)
{
	printf("> session_disconnect \n	(R) ip : %s, port : %hu, sid %llu\n<\n", dConnection->GetRemoteIPAddress().c_str(), dConnection->GetRemotePort(), dConnection->GetSessionID());
	{
		/* Acquire the session map mutex (as it can occur concurrently) */
		boost::recursive_mutex::scoped_lock lock(sessionArrayLock);

		/* Check if we have a session exist with that session key in the session map */
		if (arrSessions.find(dConnection->GetSessionID()) != arrSessions.end())
		{
			/*
			If it exist, we should erase it
			Otherwise, either it's deleted before or never existed.
			(both cases should not occur normally)
			*/
			arrSessions.erase(dConnection->GetSessionID());
		}
		else
			printf("> session_disconnect \n	(R) ip : %s, port : %hu, sid %llu\n	failed,key does not exist.\n<\n", dConnection->GetRemoteIPAddress().c_str(), dConnection->GetRemotePort(), dConnection->GetSessionID());

	}
}

template <class T>
void Server<T>::OnBrokenSession(boost::shared_ptr<T> dConnection)
{
	arrSessionCleanup.insert(dConnection);
}

#pragma endregion

#pragma region Deadline timer dispatchers
template <class T>
void Server<T>::DispatchKeepAlive()
{
	/* Set timer expiration time */
	t_keepalive.expires_from_now(boost::posix_time::seconds(KEEP_ALIVE_INTERVAL));
	/* Wait until time expires (asynchronously), then call Server::OnKeepAlive */
	t_keepalive.async_wait(boost::bind(&Server::OnKeepAlive, this));
}

template <class T>
void Server<T>::DispatchStatUpdate()
{
	/* Set timer expiration time */
	t_statupdate.expires_from_now(boost::posix_time::seconds(STAT_UPDATE_INTERVAL));
	/* Wait until time expires (asynchronously), then call Server::OnStatUpdate */
	t_statupdate.async_wait(boost::bind(&Server::OnStatUpdate, this));
}

#pragma endregion

#pragma region Deadline timer callbacks

template <class T>
void Server<T>::OnKeepAlive()
{
	printf("< keep_alive\n	checking for dead connections..\n");
	/* Acquire the session map mutex (to avoid concurrent modifications) */
	{
		boost::recursive_mutex::scoped_lock lock(sessionArrayLock);
		boost::for_each(arrSessions | boost::adaptors::map_values, boost::mem_fn(&T::CheckAlive));
		{
			BOOST_FOREACH(boost::shared_ptr<T> ptr, arrSessionCleanup)
			{
				arrSessions.erase(ptr->GetSessionID());
			}
		}
	}
	printf("	removed [%d] idle session(s)\n<\n", arrSessionCleanup.size());
	arrSessionCleanup.clear();
	DispatchKeepAlive();
}
template <class T>
void Server<T>::OnStatUpdate()
{
	static const int gbyte_limit = 1074790400;
	static const int mbyte_limit = 1049600;
	static const int kbyte_limit = 1024;
	static const char * szFormat = "service::{%d}::sess(%d) <-> ul[%.2f kb/s] / dn[%.2f %s/s] / mem [%.2f MB]";
	char buf[1024];
	std::string ind = "bytes";
	double dr = fl_downrate.load();


	if (fl_downrate > gbyte_limit)
	{
		ind = "GB";
		dr /= gbyte_limit;
	}
	else if (fl_downrate > mbyte_limit)
	{
		ind = "MB";
		dr /= mbyte_limit;
	}
	else if (fl_downrate > kbyte_limit)
	{

		ind = "KB";
		dr /= kbyte_limit;
	}
#ifdef REPORT_MEMORY_USAGE
	sprintf_s(buf, szFormat, m_sPort, GetSessionCount(), fl_uprate.load(), dr, ind.c_str(), (double)(mem_usage.getCurrentRSS() / mbyte_limit));
#else
	sprintf_s(buf, szFormat, m_sPort, GetSessionCount(), fl_uprate.load(), dr, ind.c_str(), 0);
#endif
	SetConsoleTitleA(buf);
	DispatchStatUpdate();
}

#pragma endregion


#pragma endregion



