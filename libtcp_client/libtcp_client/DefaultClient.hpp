
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/io_service.hpp>
#include "Client.hpp"



class DefaultClient : public Client,public boost::enable_shared_from_this <DefaultClient>
{
public:
	typedef boost::shared_ptr<DefaultClient> pointer;
	static pointer create(boost::asio::io_service& io_service,
		void * mgr,
		std::string& ip,
		std::string& sPort,
		int ra = DEFAULT_RECONNECT_ATTEMPT,
		int ri = DEFAULT_RECONNECT_INTERVAL)
	{
		pointer ptr = pointer(new DefaultClient(io_service, mgr,ip,sPort,ra,ri));
		return ptr;
	}

	void OnDataReceived(char *, size_t) override;
	void OnConnected()    override;
	void OnDisconnected() override;

private:
	DefaultClient(boost::asio::io_service& io_service,
		void * mgr,
		std::string& ip,
		std::string& sPort,
		int ra,
		int ri) : Client(io_service, mgr,ip,sPort,ra,ri)
	{

	}
	
};

void DefaultClient::OnDataReceived(char* buf, size_t size)
{

}

void DefaultClient::OnConnected()   {}
void DefaultClient::OnDisconnected(){}