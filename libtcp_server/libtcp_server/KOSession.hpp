#pragma once
/*
A template inherited class.
*/
#include "Session.hpp"
#include "Packet.hpp"
#include "JvCryption.h"
#include <boost/asio/io_service.hpp>

// KO sockets time out after at least 30 seconds of inactivity.
#define KOSOCKET_TIMEOUT (30) 

// Allow up to 30 minutes for the player to create their character / the client to load.
#define KOSOCKET_LOADING_TIMEOUT (30 * 60)

class KOSession : public Session, public boost::enable_shared_from_this<KOSession>
{
public:

	typedef boost::shared_ptr<KOSession> pointer;

	static pointer create(boost::asio::io_service& io_service, void * srv)
	{
		pointer ptr = pointer(new KOSession(io_service, srv));
		return ptr;
	}

	//virtual bool HandlePacket(Packet & pkt) = 0;
	__inline bool isCryptoEnabled() { return m_usingCrypto; };
	__inline unsigned __int16 GetSocketID() { return m_socketID; };
	void OnDataReceived(char *, size_t) override;
	void OnConnected() override;
	void OnDisconnected() override;

	void EnableCrypto();
	void Send(Packet * pkt);
	KOSession(boost::asio::io_service& io_service, void * srv);
	~KOSession();
private:
	bool DecryptData(unsigned char * in_stream,size_t len, Packet & pkt);
	
	CJvCryption m_crypto;
	uint32 m_sequence;
	unsigned short m_socketID;
	bool m_usingCrypto;

};

