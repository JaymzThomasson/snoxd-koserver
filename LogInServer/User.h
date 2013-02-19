#pragma once

#include "../shared/KOSocket.h"
#include "Define.h"

class CVersionManagerDlg;
class LoginSession : public KOSocket
{
public:
	LoginSession(uint16 socketID, SocketMgr *mgr); 

	virtual bool HandlePacket(Packet & pkt);
	void HandleVersion(Packet & pkt);
	void HandlePatches(Packet & pkt);
	void HandleLogin(Packet & pkt);
	void HandleServerlist(Packet & pkt);
	void HandleNews(Packet & pkt);
#if __VERSION >= 1453
	void HandleSetEncryptionPublicKey(Packet & pkt);
	void HandleUnkF7(Packet & pkt);
#endif
};

void InitPacketHandlers(void);
typedef void (LoginSession::*LSPacketHandler)(Packet &);