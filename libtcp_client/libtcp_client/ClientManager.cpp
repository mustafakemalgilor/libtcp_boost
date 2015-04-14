#include "ClientManager.hpp"
#include "DefaultClient.hpp"

ClientManager::ClientManager() : io_service_pool_(8)
{
	#if defined TEST_MODE
	for (int i = 0; i < TEST_CLIENT_COUNT; i++)
	{
		SpawnNewClient((std::string)"127.0.0.1", (std::string)"3333", 7, 3000);
		//::Sleep(150);
	}
	#endif
}


ClientManager::~ClientManager()
{
}

void ClientManager::Run()
{
	io_service_pool_.run();
}

void ClientManager::SpawnNewClient(std::string &ip, std::string& port, int reconnectAttempt, int reconnectInterval)
{
	{
		/* Acquire the session map mutex (as it can occur concurrently) */
		boost::recursive_mutex::scoped_lock lock(clientArrayLock);
		DefaultClient::pointer new_client = DefaultClient::create(io_service_pool_.get_io_service(), this, ip, port, reconnectAttempt, reconnectInterval);
		new_client->Connect();

		arrClients.insert(std::make_pair(new_client->GetSessionID(), new_client));
	}
}

void ClientManager::OnClientDisconnect(Client::pointer dConnection)
{
	{
		boost::recursive_mutex::scoped_lock lock(clientArrayLock);
		printf("> session_disconnect \n	(R) ip : %s, port : %hu, sid %llu\n<\n", dConnection->GetRemoteIPAddress().c_str(), dConnection->GetRemotePort(), dConnection->GetSessionID());
		arrClients.erase(dConnection->GetSessionID());
	}
}
