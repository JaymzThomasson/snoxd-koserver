#pragma once

#define LUA_SCRIPT_DIRECTORY	"./quests/"
#define LUA_SCRIPT_ENTRY_POINT	"Main"
#define LUA_SCRIPT_GLOBAL_USER	"pUser"
#define LUA_SCRIPT_GLOBAL_NPC	"pNpc"

extern "C" {
#	include "../scripting/Lua/lua.h"
#	include "../scripting/Lua/lualib.h"
#	include "../scripting/Lua/lauxlib.h"
}

#include "../scripting/lua_helpers.h"
#include "lua_bindings.h"

class CUser;
class CNpc;
class CLuaScript
{
public:
	CLuaScript();
	bool Initialise();
	bool ExecuteScript(CUser * pUser, CNpc * pNpc, int32 nEventID, const char * filename);
	~CLuaScript();

private:
	lua_State * m_luaState;
	FastMutex m_lock;
};

class CLuaEngine
{
public:
	CLuaEngine();
	bool Initialise();
	bool ExecuteScript(CUser * pUser, CNpc * pNpc, int32 nEventID, const char * filename);
	~CLuaEngine();

private:
	// For now, we'll only use a single instance for such.
	// In the future, however, it would be wise to spread the load across 
	// multiple script instances (which have been completely thread-safe since Lua 5.1)
	CLuaScript m_luaScript;
};