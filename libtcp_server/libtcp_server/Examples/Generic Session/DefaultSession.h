#pragma once
/*
	A template inherited class.
*/
#include "../../Headers/Session.hpp"
#include <boost/asio/io_service.hpp>

class DefaultSession : public Session, public boost::enable_shared_from_this<DefaultSession>
{
public:

	typedef boost::shared_ptr<DefaultSession> pointer;

	static pointer create(boost::asio::io_service& io_service, void * srv)
	{
		pointer ptr = pointer(new DefaultSession(io_service, srv));
		return ptr;
	}
	void OnDataReceived(char *, size_t) override;
	void OnConnected() override;
	void OnDisconnected() override;
	DefaultSession(boost::asio::io_service& io_service, void * srv);
	~DefaultSession();
};

