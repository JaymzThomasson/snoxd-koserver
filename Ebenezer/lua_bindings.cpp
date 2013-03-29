#include "StdAfx.h"
#include "LuaEngine.h"
#include "User.h"
#include "Npc.h"

/* 
	Classes
*/

/**
 * Defining a class is simple: just #define LUA_CLASS to your class name, 
 * then call DEFINE_LUA_CLASS(), specifying each method to expose.
 *
 * Remember to #undef LUA_CLASS when you're done.
 *
 * Doing so sacrifices a little readability, however it
 * makes manually defining methods much quicker (less to type).
 *
 * Also, as our method doesn't support inheritance, you'll need
 * to redefine the same methods for each class.
 *
 * This is because of the way we dynamically pull instance pointers;
 * these are typedef'd within the class, so that we can refer to them 
 * to grab the class name from within. As we type check these with Lua,
 * they'll fail, so we won't be able to use them.
 *
 * I don't think this is such a big issue, as there's only a handful of
 * cases that actually require this behaviour.
 **/

#define LUA_CLASS CUser
DEFINE_LUA_CLASS
(
	// Getters
	MAKE_LUA_METHOD(GetName)
	MAKE_LUA_METHOD(GetAccountName)
	MAKE_LUA_METHOD(GetZoneID)
	MAKE_LUA_METHOD(GetX)
	MAKE_LUA_METHOD(GetY)
	MAKE_LUA_METHOD(GetZ)
	MAKE_LUA_METHOD(GetNation)
	MAKE_LUA_METHOD(GetLevel)
	MAKE_LUA_METHOD(GetClass)
	MAKE_LUA_METHOD(GetCoins)
	MAKE_LUA_METHOD(GetInnCoins)
	MAKE_LUA_METHOD(GetLoyalty)
	MAKE_LUA_METHOD(GetMonthlyLoyalty)
	MAKE_LUA_METHOD(GetManner)
	MAKE_LUA_METHOD(isWarrior)
	MAKE_LUA_METHOD(isRogue)
	MAKE_LUA_METHOD(isMage)
	MAKE_LUA_METHOD(isPriest)
	MAKE_LUA_METHOD(isInClan)
	MAKE_LUA_METHOD(isClanLeader)
	MAKE_LUA_METHOD(isInParty)
	MAKE_LUA_METHOD(isPartyLeader)

	// Shortcuts for lazy people
	MAKE_LUA_METHOD(hasCoins)
	MAKE_LUA_METHOD(hasInnCoins)
	MAKE_LUA_METHOD(hasLoyalty)
	MAKE_LUA_METHOD(hasMonthlyLoyalty)
	MAKE_LUA_METHOD(hasManner)

	// Here lie the useful methods.
	MAKE_LUA_METHOD(GiveItem)
	MAKE_LUA_METHOD(RobItem)
	MAKE_LUA_METHOD(CheckExistItem)
	MAKE_LUA_METHOD(GoldGain)
	MAKE_LUA_METHOD(GoldLose)
	// MAKE_LUA_METHOD(RequestReward)
	// MAKE_LUA_METHOD(RequestPersonalRankReward)
	MAKE_LUA_METHOD(SaveEvent)
	MAKE_LUA_METHOD(SearchQuest)
	MAKE_LUA_METHOD(ShowMap)
	MAKE_LUA_METHOD(SelectMsg) // menu
	MAKE_LUA_METHOD(NpcSay) // dialog
	MAKE_LUA_METHOD(NpcMsg) // new automated quest prompt (does whatever's needed, menu, quest prompt, etc)
	MAKE_LUA_METHOD(CheckWeight)
	MAKE_LUA_METHOD(CheckSkillPoint)
	// MAKE_LUA_METHOD(CheckStatPoint)
	MAKE_LUA_METHOD(isRoomForItem) // FindSlotForItem()
	// MAKE_LUA_METHOD(CountMonsterQuestSub) // CheckMonsterCount(2)
	// MAKE_LUA_METHOD(CountMonsterQuestMain) // CheckMonsterCount(3)
	MAKE_LUA_METHOD(CheckExchange)
	MAKE_LUA_METHOD(RunExchange)
	// MAKE_LUA_METHOD(SendNameChangeDialog)
	// MAKE_LUA_METHOD(SendRestatDialog) // these two are the same thing
	// MAKE_LUA_METHOD(SendReskillDialog)
	// MAKE_LUA_METHOD(ResetSkillPoints) // AllSkillPointChange(1)
	// MAKE_LUA_METHOD(ResetStatPoints) // AllStatPointChange(1)
	MAKE_LUA_METHOD(PromoteUserNovice)
	MAKE_LUA_METHOD(PromoteUser)
	// MAKE_LUA_METHOD(RobAllItemParty)
	// MAKE_LUA_METHOD(ZoneChangeParty)
	// MAKE_LUA_METHOD(ZoneChangeClan)
	// MAKE_LUA_METHOD(PromoteKnight)
	// MAKE_LUA_METHOD(SendEffect)
	MAKE_LUA_METHOD(KissUser)
);
#undef LUA_CLASS

#define LUA_CLASS CNpc
DEFINE_LUA_CLASS
(
	// Getters
	MAKE_LUA_METHOD(GetID)
	MAKE_LUA_METHOD(GetEntryID)
	MAKE_LUA_METHOD(GetName)
	MAKE_LUA_METHOD(GetNation)
	MAKE_LUA_METHOD(GetType)
	MAKE_LUA_METHOD(GetZoneID)
	MAKE_LUA_METHOD(GetX)
	MAKE_LUA_METHOD(GetY)
	MAKE_LUA_METHOD(GetZ)

	// Useful methods
	// MAKE_LUA_METHOD(CycleSpawn) // i.e. ChangePosition(), used to cycle a spawn through the various trap numbers (like 7 key quest NPCs)

);
#undef LUA_CLASS


/* 
	Global functions 
*/

LUA_FUNCTION(myrand)
{
	LUA_RETURN(myrand(LUA_ARG(int, 1), 
					  LUA_ARG(int, 2)));
}

LUA_FUNCTION(CheckPercent)
{
	LUA_RETURN(CheckPercent(LUA_ARG(int, 1)));
}