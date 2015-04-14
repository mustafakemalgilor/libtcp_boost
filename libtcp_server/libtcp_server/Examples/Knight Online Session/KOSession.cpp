#include "../../Headers/stdafx.h"
#include "KOSession.hpp"


KOSession::KOSession(boost::asio::io_service& io_service, void * srv) : Session(io_service, srv), m_usingCrypto(false)
{
	
}


KOSession::~KOSession()
{
}

void KOSession::OnDataReceived(char * buf, size_t len)
{
	Packet pkt;
	if (DecryptData((unsigned char*)buf, len, pkt))
	{
		printf("< kosocket::on_data_received\n	received opcode : %x\n>\n", pkt.GetOpcode());
		// Send to process..
	}
}


void KOSession::OnConnected()    {}
void KOSession::OnDisconnected() {}

void KOSession::EnableCrypto()
{
	m_crypto.Init();
	m_usingCrypto = true;
}

bool KOSession::DecryptData(unsigned char *in_stream,size_t m_remaining, Packet & pkt)
{
	uint8* final_packet = nullptr;
	uint32 seqid = 0;
	
	if (isCryptoEnabled())
	{
		bool decryptState = m_crypto.JvDecryptionWithCRC32((int)m_remaining, in_stream, in_stream) > -1;
		if (m_remaining < 4 || !decryptState || ++m_sequence != *(uint32 *)(in_stream)) return false;

		m_remaining -= 8; // remove the sequence ID & CRC checksum
		final_packet = &in_stream[4];
	}
	else
		final_packet = in_stream;

	/* Discard opcode */
	m_remaining--;
	pkt = Packet(final_packet[0], (size_t)m_remaining);
	/* Append remaining */
	if (m_remaining > 0)
	{
		pkt.resize(m_remaining);
		memcpy((void*)pkt.contents(), &final_packet[1], m_remaining);
	}
	/* Else, the packet was only containing the opcode. */

	return true;
}

void KOSession::Send(Packet * pkt)
{
	unsigned char opcode = pkt->GetOpcode();
	unsigned char * out_stream = nullptr;
	unsigned __int16 len = (unsigned __int16)(pkt->size() + 1);

	switch (isCryptoEnabled())
	{
		case true:
		{
			/* aa55 + 2 + 55aa*/
			len += 5;
			out_stream = new unsigned char[len + 6];
			*(unsigned __int16 *)&out_stream[0] = 0xAA55;
			*(unsigned __int16 *)&out_stream[2] = len;

			/* CRC & Sequence stuff*/
			*(unsigned __int16 *)&out_stream[4] = 0x1efc;
			*(unsigned __int16 *)&out_stream[6] = (unsigned __int16)(m_sequence); // this isn't actually incremented here
			out_stream[8] = 0;

			out_stream[9] = pkt->GetOpcode();

			if (pkt->size() > 0)
				memcpy(&out_stream[10], pkt->contents(), pkt->size());
			*(unsigned __int16 *)&out_stream[len + 2] = 0x55AA;

			m_crypto.JvEncryptionFast(len, out_stream, out_stream);
		}
		break;
		case false:
		{
			out_stream = new unsigned char[len + 6];

			*(unsigned __int16 *)&out_stream[0] = 0xAA55;
			*(unsigned __int16 *)&out_stream[2] = len;

			out_stream[5] = pkt->GetOpcode();
			if (pkt->size() > 0)
				memcpy(&out_stream[6], pkt->contents(), pkt->size());

			*(unsigned __int16 *)&out_stream[len + 2] = 0x55AA;
		}
		break;
	}


	/*
	r = BurstSend((const uint8*)"\xaa\x55", 2);
	if (r) r = BurstSend((const uint8*)&len, 2);
	if (r) r = BurstSend((const uint8*)out_stream, len);
	if (r) r = BurstSend((const uint8*)"\x55\xaa", 2);
	if (r) BurstPush();
	
	*/
	Session::Send(out_stream, len);
	delete[] out_stream;
}
