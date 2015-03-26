#pragma once

#include "Client.hpp"
#include "IOServicePool.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/atomic.hpp>

#include <map>
#include <set>

using boost::asio::ip::tcp;
using std::map;
using std::set;

typedef std::map<unsigned long long, boost::shared_ptr<Client>> ClientMap;
typedef std::set<boost::shared_ptr<Client>> ClientSet;
typedef boost::atomic<unsigned long long> Atomic_ULL;

#if defined TEST_MODE
	#define TEST_CLIENT_COUNT 1000
#endif

class ClientManager
{
public:
	ClientManager();
	~ClientManager();
	void Run();

	void OnClientDisconnect(Client::pointer ptr);
	void SpawnNewClient(std::string& ip, std::string& port, int reconnectAttempt = 0, int reconnectInterval = 5000);
	size_t GetClientCount() const { return arrClients.size(); }
private:
	/*
	The IO service pool for server.
	Default initialization value depends on CPU core amount.
	*/
	io_service_pool io_service_pool_;

	mutable boost::recursive_mutex clientArrayLock;
	ClientMap arrClients;
};

