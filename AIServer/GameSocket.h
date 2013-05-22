#pragma once

#include "../shared/KOSocket.h"
#include "Party.h"

class CUser;
class MAP;

class CGameSocket : public KOSocket
{
public:
	CParty		m_Party;

	CGameSocket(uint16 socketID, SocketMgr *mgr) : KOSocket(socketID, mgr, -1, 65536, 65536) {}

	virtual void OnConnect();

	void Initialize();
	bool HandlePacket(Packet & pkt);

	virtual void OnDisconnect();
	void RecvServerConnect(Packet & pkt);
	void RecvUserInfo(Packet & pkt);
	void RecvUserInOut(Packet & pkt);
	void RecvUserMove(Packet & pkt);
	void RecvUserMoveEdge(Packet & pkt);
	void RecvUserLogOut(Packet & pkt);
	void RecvUserRegene(Packet & pkt);
	void RecvUserSetHP(Packet & pkt);
	void RecvNpcSetHP(Packet & pkt);
	void RecvAttackReq(Packet & pkt);
	void RecvUserUpdate(Packet & pkt);
	void RecvZoneChange(Packet & pkt);
	void RecvUserInfoAllData(Packet & pkt);
	void RecvPartyInfoAllData(Packet & pkt);
	void RecvGateOpen(Packet & pkt);
	void RecvUserVisibility(Packet & pkt);
	void RecvCheckAlive(Packet & pkt);
	void RecvHealMagic(Packet & pkt);
	void RecvTimeAndWeather(Packet & pkt);
	void RecvUserFail(Packet & pkt);
	void Send_UserError(short uid, short tid = 10000);
	void RecvBattleEvent(Packet & pkt);

	bool SetUid(float x, float z, int id, int speed);

	virtual ~CGameSocket();
};
