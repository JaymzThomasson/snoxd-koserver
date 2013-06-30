void CUser::Attack(Packet & pkt)
{
	Packet result;
	int16 sid = -1, tid = -1, damage, delaytime, distance;	
	uint8 bType, bResult = 0;	
	
	CUser* pTUser = nullptr;

	pkt >> bType >> bResult >> tid >> delaytime >> distance;

//	delaytime = delaytime / 100.0f;
//	distance = distance / 10.0f;

	if (isBlinking()
		|| isDead())
		return;

	if (m_bInvisibilityType != INVIS_NONE)
	{
		CMagicProcess::RemoveStealth(this, INVIS_DISPEL_ON_MOVE);
		CMagicProcess::RemoveStealth(this, INVIS_DISPEL_ON_ATTACK);
	}

	// If you're holding a weapon, do a client-based (ugh, do not trust!) delay check.
	_ITEM_TABLE *pTable = GetItemPrototype(RIGHTHAND);
	if (pTable != nullptr) 
	{
		if (delaytime < pTable->m_sDelay
			|| distance > pTable->m_sRange)
			return;	
	}
	// Empty handed.
	else if (delaytime < 100)
		return;			

	// We're attacking a player...
	if (tid < MAX_USER)
	{
		pTUser = g_pMain->GetUserPtr(tid);
 
		if (pTUser != nullptr
			&& CanAttack(pTUser)) 
		{
			damage = GetDamage(pTUser, nullptr);
			if (GetZoneID() == ZONE_SNOW_BATTLE && g_pMain->m_byBattleOpen == SNOW_BATTLE)
				damage = 0;		

			if (damage > 0)
			{
				// TO-DO: Move all this redundant code into appropriate event-based methods so that all the other cases don't have to copypasta (and forget stuff).
				pTUser->HpChange(-damage, this);
				if (pTUser->isDead())
					bResult = 2;

				ItemWoreOut(ATTACK, damage);
				pTUser->ItemWoreOut(DEFENCE, damage);
			}
		}
	}
	// We're attacking an NPC...
	else if (tid >= NPC_BAND)
	{
		// AI hasn't loaded yet
		if (g_pMain->m_bPointCheckFlag == false)	
			return;	

		CNpc *pNpc = g_pMain->m_arNpcArray.GetData(tid);		
		if (pNpc != nullptr 
			&& CanAttack(pNpc))
		{
			result.SetOpcode(AG_ATTACK_REQ);
			result	<< bType << bResult
					<< GetSocketID() << tid;
			Send_AIServer(&result);	
			return;
		}
	}

	result.SetOpcode(WIZ_ATTACK);
	result << bType << bResult << GetSocketID() << tid;
	SendToRegion(&result);

	if (tid < NPC_BAND
		&& bResult == 2 // 2 means a player died.
		&& pTUser) 
	{
		pTUser->Send(&result);
		TRACE("*** User Attack Dead, id=%s, result=%d, type=%d, HP=%d\n", pTUser->GetName().c_str(), bResult, pTUser->m_bResHpType, pTUser->m_sHp);
	}
}

void CUser::Regene(uint8 regene_type, uint32 magicid /*= 0*/)
{
	ASSERT(GetMap() != nullptr);

	_OBJECT_EVENT* pEvent = nullptr;
	_HOME_INFO* pHomeInfo = nullptr;
	float x = 0.0f, z = 0.0f;

	if (!isDead())
		return;

	if (regene_type != 1 && regene_type != 2)
		regene_type = 1;

	if (regene_type == 2) 
	{
		magicid = 490041;	// The Stone of Ressurection magic ID

		// Is our level high enough to be able to resurrect using this skill?
 		if (GetLevel() <= 5
			// Do we have enough resurrection stones?
			|| !RobItem(379006000, 3 * GetLevel()))
			return;
	}

	// If we're in a home zone, we'll want the coordinates from there. Otherwise, assume our own home zone.
	pHomeInfo = g_pMain->m_HomeArray.GetData(GetZoneID() <= ZONE_ELMORAD ? GetZoneID() : GetNation());
	if (pHomeInfo == nullptr)
		return;

	UserInOut(INOUT_OUT);

	pEvent = GetMap()->GetObjectEvent(m_sBind);	

	// If we're not using a spell to resurrect.
	if (magicid == 0) 
	{
		// Resurrect at a bind/respawn point
		if (pEvent != nullptr && pEvent->byLife == 1)
		{
			SetPosition(pEvent->fPosX + x, 0.0f, pEvent->fPosZ + z);
		}
		// Are we trying to respawn in a home zone?
		else if (GetZoneID() <= ZONE_ELMORAD) 
		{
			// Use the proper respawn area for our nation, as the opposite nation can
			// enter this zone at a war's invasion stage.
			if (GetNation() == KARUS) 
			{
				x = (float)(pHomeInfo->KarusZoneX + myrand(0, pHomeInfo->KarusZoneLX));
				z = (float)(pHomeInfo->KarusZoneZ + myrand(0, pHomeInfo->KarusZoneLZ));			
			}
			else 
			{
				x = (float)(pHomeInfo->ElmoZoneX + myrand(0, pHomeInfo->ElmoZoneLX));
				z = (float)(pHomeInfo->ElmoZoneZ + myrand(0, pHomeInfo->ElmoZoneLZ));
			}		
		}
		else
		{
			// If we're in a war zone (aside from snow wars, which apparently use different coords), use BattleZone coordinates.
			if (GetZoneID() != ZONE_SNOW_BATTLE 
				&& GetZoneID() == (100 + g_pMain->m_byBattleZone))
			{
				x = (float)(pHomeInfo->BattleZoneX + myrand(0, pHomeInfo->BattleZoneLX));
				z = (float)(pHomeInfo->BattleZoneZ + myrand(0, pHomeInfo->BattleZoneLZ));
			}
			// If we're in a zone that can attack other-nation players (includes snow wars), use FreeZone coordinates instead.
			else if (GetMap()->canAttackOtherNation())
			{
				x = (float)(pHomeInfo->FreeZoneX + myrand(0, pHomeInfo->FreeZoneLX));
				z = (float)(pHomeInfo->FreeZoneZ + myrand(0, pHomeInfo->FreeZoneLZ));
			}
			// For all else, just grab the start position (/town coordinates) from the START_POSITION table.
			else
			{
				short sx, sz;
				GetStartPosition(sx, sz);

				x = sx;
				z = sz;
			}
		}

		SetPosition(x, 0.0f, z);

		m_bAbnormalType = ABNORMAL_BLINKING;
		m_bResHpType = USER_STANDING;	
		m_bRegeneType = REGENE_NORMAL;
	}
	else // we're respawning using a resurrect skill.
	{
		_MAGIC_TYPE5 * pType = g_pMain->m_Magictype5Array.GetData(magicid);     
		if (pType == nullptr)
			return;

		MSpChange(-m_iMaxMp); // reset us to 0 MP. 

		if (m_sWhoKilledMe == -1) 
			ExpChange((m_iLostExp * pType->bExpRecover) / 100); // Restore 

		m_bResHpType = USER_STANDING;
		m_bRegeneType = REGENE_MAGIC;
	}

	Packet result(WIZ_REGENE);
	result << GetSPosX() << GetSPosZ() << GetSPosY();
	Send(&result);

	HpChange(m_iMaxHp);

	m_tLastRegeneTime = UNIXTIME;
	m_sWhoKilledMe = -1;
	m_iLostExp = 0;

	if (!isBlinking())
	{
		result.Initialize(AG_USER_REGENE);
		result << GetSocketID() << m_sHp;
		Send_AIServer(&result);
	}

	SetRegion(GetNewRegionX(), GetNewRegionZ());

	UserInOut(INOUT_RESPAWN);		

	g_pMain->RegionUserInOutForMe(this);
	g_pMain->RegionNpcInfoForMe(this);

	InitializeStealth();
	SendUserStatusUpdate(USER_STATUS_DOT, USER_STATUS_CURE);
	SendUserStatusUpdate(USER_STATUS_POISON, USER_STATUS_CURE);

	if (isInArena())
		SendUserStatusUpdate(USER_STATUS_SPEED, USER_STATUS_CURE);

	BlinkStart();
	RecastSavedMagic();
}