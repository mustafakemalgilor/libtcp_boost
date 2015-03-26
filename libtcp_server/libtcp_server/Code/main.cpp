// BoostASIOCP_Server.cpp : Defines the entry point for the console application.
//

#include "../Headers/stdafx.h"
#include "../Headers/Server.hpp"
#include "../Headers/Session.hpp"
#include "../Examples/Generic Session/DefaultSession.h"
#include "../Examples/Knight Online Session/KOSession.hpp"

int _tmain(int argc, _TCHAR* argv[])
{
	SetConsoleTitleW(L"[initializing]");
	//printf("test server application using boost library\n");
	printf("< _main \n	Binding port..\n");
	try
	{
		
		Server<DefaultSession> server(3333);
	
		printf("	Succeeded listening on port (%hu)..\n", 3333);
		printf("	Ready.\n>\n");
		server.Run();
	}
	catch (...)
	{
		printf("	Could not bind accept socket on port (%hu).\n", 3333);
		printf("	Probably the specified port is already bound to another application.\n>\n");
		printf("< press any key to exit.> \n");
		getchar();
	}


	return 0;
}



