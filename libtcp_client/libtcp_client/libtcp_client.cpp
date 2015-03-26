// libtcp_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ClientManager.hpp"
#include <boost/asio/io_service.hpp>
#include <vector>

std::vector<Client::pointer> s;
int _tmain(int argc, _TCHAR* argv[])
{
	static const char * szFormat = "libtcp_win::client(%d) <-> ul  [0 kb/s]   /   dn  [0 kb/s]";
	char buf[1024];
	
	ClientManager mgr;
	sprintf_s(buf, szFormat,mgr.GetClientCount());
	SetConsoleTitleA(buf);
	mgr.Run();
	getchar();
	return 0;
}

