#pragma once

#include "../shared/KOSocket.h"
#include "MagicProcess.h"

class CAISocket : public KOSocket 
{
public:
	CAISocket(uint16 socketID, SocketMgr * mgr) : KOSocket(socketID, mgr, 0, 65536, 65536), m_bHasConnected(false) {}

	__forceinline bool IsReconnecting() { return m_bHasConnected; }

	virtual void OnConnect();
	virtual bool HandlePacket(Packet & pkt);

	void Initialize();

	void InitEventMonster(int index);

	void LoginProcess(Packet & pkt);
	void RecvCheckAlive(Packet & pkt);
	void RecvServerInfo(Packet & pkt);
	void RecvNpcInfoAll(Packet & pkt);
	void RecvNpcMoveResult(Packet & pkt);
	void RecvNpcAttack(Packet & pkt);
	void RecvMagicAttackResult(Packet & pkt);
	void RecvNpcInfo(Packet & pkt);
	void RecvUserHP(Packet & pkt);
	void RecvUserExp(Packet & pkt);
	void RecvSystemMsg(Packet & pkt);
	void RecvNpcGiveItem(Packet & pkt);
	void RecvUserFail(Packet & pkt);
	void RecvGateDestory(Packet & pkt);
	void RecvNpcDead(Packet & pkt);
	void RecvNpcInOut(Packet & pkt);
	void RecvBattleEvent(Packet & pkt);
	void RecvNpcEventItem(Packet & pkt);
	void RecvGateOpen(Packet & pkt);
	void RecvCompressed(Packet & pkt);

	virtual void OnDisconnect();
	virtual ~CAISocket() {}

private:
	bool m_bAllNpcInfoRecv;
	CMagicProcess m_MagicProcess;
	bool m_bHasConnected;
};
