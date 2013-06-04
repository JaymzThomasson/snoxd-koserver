#include "stdafx.h"
#include "EbenezerDlg.h"
#include "User.h"
#include "Map.h"
#include "KnightsManager.h"
#include "MagicProcess.h"
#include "KingSystem.h"
#include "../shared/KOSocketMgr.h"
#include "../shared/tstring.h"

using namespace std;

ServerCommandTable CEbenezerDlg::s_commandTable;
ChatCommandTable CUser::s_commandTable;

// List of user packet handlers for compilation 
// in one combined unit.
// Doing this speeds up compilation times considerably.

#include "User.cpp"
#include "ArenaHandler.cpp"
#include "AttackHandler.cpp"
#include "CharacterMovementHandler.cpp"
#include "CharacterSelectionHandler.cpp"
#include "ChatHandler.cpp"
#include "FriendHandler.cpp"
#include "ItemHandler.cpp"
#include "LetterHandler.cpp"
#include "LoginHandler.cpp"
#include "MerchantHandler.cpp"
#include "NPCHandler.cpp"
#include "PartyHandler.cpp"
#include "QuestHandler.cpp"
#include "RentalHandler.cpp"
#include "ShoppingMallHandler.cpp"
#include "TradeHandler.cpp"

