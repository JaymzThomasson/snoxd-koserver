#pragma once

#include "Extern.h"

#include "../shared/STLMap.h"
#include "../Ebenezer/Unit.h"

class MAP;

class CUser : public Unit
{
public:
	INLINE bool isGM() { return m_byIsOP == AUTHORITY_GAME_MASTER; }

	virtual uint16 GetID() { return m_iUserId; }
	virtual std::string & GetName() { return m_strUserID; }

	virtual int32 GetHealth() { return m_sHP; }
	virtual int32 GetMaxHealth() { return m_sMaxHP; }
	virtual int32 GetMana() { return m_sMP; }
	virtual int32 GetMaxMana() { return m_sMaxMP; }

	virtual void GetInOut(Packet &, uint8) {}
	virtual void AddToRegion(int16 sRegionX, int16 sRegionZ) {}

	virtual void HpChange(int amount, Unit *pAttacker = nullptr, bool bSendToAI = true) {}
	virtual void MSpChange(int amount) {}

	virtual bool isDead() { return m_bLive == AI_USER_DEAD || GetHealth() <= 0; }

	INLINE bool isInParty() { return m_byNowParty != 0; }
	INLINE uint16 GetPartyID() { return m_sPartyNumber; }

	std::string m_strUserID;
	short	m_iUserId;					// User의 번호
	uint8	m_bLive;					// 죽었니? 살았니?

	float			m_fWill_x;			// 다음 X 좌표
	float			m_fWill_y;			// 다음 Y 좌표
	float			m_fWill_z;			// 다음 Z 좌표
	short			m_sSpeed;			// 유저의 스피드	

	short	m_sHP;							// HP
	short	m_sMP;							// MP
	short	m_sMaxHP;						// HP
	short	m_sMaxMP;						// MP

	uint8	m_state;						// User의 상태

	short	m_sOldRegionX;					// 이전 영역 X 좌표
	short	m_sOldRegionZ;					// 이전 영역 Z 좌표

	uint8	m_bResHp;						// 회복량
	uint8	m_bResMp;
	uint8	m_bResSta;

	uint8    m_byNowParty;				// 파티중이면 1, 부대중이면 2, 둘다 아니면 0
	uint8	m_byPartyTotalMan;			// 파티 맺은 총 구성 인원수 
	short   m_sPartyTotalLevel;			// 파티 맺은 사람의 총 레벨
	short	m_sPartyNumber;				// 파티 번호

	short   m_sItemAc;                  // 아이템 방어률

	short  m_sSurroundNpcNumber[8];		// Npc 다굴~

	uint8   m_byIsOP;
	uint8	m_bInvisibilityType;

public:
	void Initialize();
	void InitNpcAttack();
	void Attack(int sid, int tid);	// ATTACK
	bool SetDamage(int damage, int attackerID);
	void Dead(int tid, int nDamage);					// user dead
	void SetExp(int iNpcExp, int iLoyalty, int iLevel);		// user exp
	void SetPartyExp(int iNpcExp, int iLoyalty, int iPartyLevel, int iMan);		// user exp
	int IsSurroundCheck(float fX, float fY, float fZ, int NpcID);
	void HealMagic();
	void HealAreaCheck(int rx, int rz);

	void SendAttackSuccess(short tid, uint8 result, short sDamage, int nHP=0, short sAttack_type=1, uint8 type = 1, short sid = -1);
	void SendHP();												// user의 HP
	void SendExp(int32 iExp, int32 iLoyalty, int tType = 1);

	short GetDamage(Unit *pTarget, _MAGIC_TABLE *pSkill = nullptr, bool bPreviewOnly = false);

	CUser();
	virtual ~CUser();

	// Placeholders, for the magic system.
	// These should really be using the same base class.
	INLINE bool isInClan() { return false; }
	INLINE uint16 GetClanID() { return 0; }
	INLINE uint8 GetStat(StatType type) { return 0; }
	INLINE void SetStatBuff(StatType type, uint8 val) {}
		
	void RemoveSavedMagic(uint32 nSkillID) {}
	void SendUserStatusUpdate(UserStatus type, UserStatusBehaviour status) {}
	void SetUserAbility(bool bSendPacket = true) {}
	void Send(Packet * pkt) {}

	time_t	m_tLastRegeneTime;
	uint32	m_nOldAbnormalType;
	uint16	m_sExpGainAmount;
	uint8	m_bMaxWeightAmount, m_bNPGainAmount, m_bNoahGainAmount, 
			m_bPlayerAttackAmount, m_bSkillNPBonus;
	bool	m_bPremiumMerchant;
};
