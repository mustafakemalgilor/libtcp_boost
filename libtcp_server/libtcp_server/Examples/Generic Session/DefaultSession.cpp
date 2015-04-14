#include "../../Headers/stdafx.h"
#include "DefaultSession.h"


DefaultSession::DefaultSession(boost::asio::io_service& io_service, void * srv) : Session(io_service, srv)
{
}


DefaultSession::~DefaultSession()							{}

void DefaultSession::OnDataReceived(char * buf, size_t len)	{}
void DefaultSession::OnConnected()							{}
void DefaultSession::OnDisconnected()						{}
