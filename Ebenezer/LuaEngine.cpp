#include "stdafx.h"

// define global functions to be called from Lua (e.g. myrand())
DEFINE_LUA_FUNCTION_TABLE(g_globalFunctions, 
	MAKE_LUA_FUNCTION(myrand)
);

CLuaEngine::CLuaEngine()
{
}

CLuaScript::CLuaScript() : m_luaState(NULL)
{
}

// Initialise Lua scripts.
bool CLuaEngine::Initialise()
{
	// TO-DO: Initialise a pool of scripts (enough for 1 per worker thread).
	return m_luaScript.Initialise();
}

bool CLuaScript::Initialise()
{
	FastGuard lock(m_lock);

	// Lua already initialised?
	if (m_luaState != NULL)
	{
		printf("ERROR: Lua script already initialised. Cannot reinitialise.\n");
		return false;
	}

	// Create a new state.
	m_luaState = luaL_newstate();
	if (m_luaState == NULL)
	{
		printf("ERROR: Failed to initialise Lua script. Not enough memory.\n");
		return false;
	}

	// Expose scripts to Lua libraries
	// May be preferable to limit these, but for now we won't stress too much.
	luaL_openlibs(m_luaState);

	/* globals */

	// push the global table onto the stack so we can set our globals
	lua_pushglobaltable(m_luaState);

	// setup our global funcs...
	luaL_setfuncs(m_luaState, g_globalFunctions, 0);

	/* objects */

	// bind our classes
	lua_bindclass(m_luaState, CUser);
	lua_bindclass(m_luaState, CNpc);

	return true;
}

bool CLuaEngine::ExecuteScript(CUser * pUser, CNpc * pNpc, int32 nEventID, const char * filename)
{
	// TO-DO: Pull an available script for use
	return m_luaScript.ExecuteScript(pUser, pNpc, nEventID, filename);
}

bool CLuaScript::ExecuteScript(CUser * pUser, CNpc * pNpc, int32 nEventID, const char * filename)
{
	// ensure that we wait until the last user's done executing their script.
	FastGuard lock(m_lock);

	// This is really poor behaviour, but for now let's just load the script up each time (from disk).
	// Note that this will overwrite existing tables, so it is technically possible to refer to previously loaded 
	// scripts' functions/variables. There's no real concern so long as this behaviour isn't relied upon in scripts --
	// any duplicate declarations will be overwritten with that from the new file.

	char szPath[_MAX_PATH];
	sprintf_s(szPath, sizeof(szPath), LUA_SCRIPT_DIRECTORY "%s", filename);

	/* Attempt to load the file */
	int err = luaL_dofile(m_luaState, szPath);

	// If something bad happened, try to find an error.
	if (err != LUA_OK)
	{
		switch (err)
		{
		case LUA_ERRFILE:
			printf("ERROR: Unable to load Lua script `%s`.\n", szPath);
			break;

		case LUA_ERRSYNTAX:
			printf("ERROR: There was a error with the syntax of Lua script `%s`.\n", szPath);
			break;

		case LUA_ERRMEM:
			printf("ERROR: Unable to allocate memory for Lua script `%s`.\n", szPath);
			break;

		default:
			printf("ERROR: An unknown error occurred while loading Lua script `%s`.\n", szPath);
			break;
		}

		// Is there an error set? That can be more useful than our generic error.
		if (lua_isstring(m_luaState, -1))
		{
			printf("ERROR: [%s] The following error was provided:\n%s\n",
				filename, lua_to<const char *>(m_luaState, -1));
		}

		return false;
	}

	/* Attempt to run the script. */

	// Entry point requires 1 arguments: the event ID.
	// The user & NPC instances are globals.

	lua_tsetglobal(m_luaState, LUA_SCRIPT_GLOBAL_USER, pUser);
	lua_tsetglobal(m_luaState, LUA_SCRIPT_GLOBAL_NPC, pNpc);

	// Find & assign script's entry point to the stack
	lua_getglobal(m_luaState, LUA_SCRIPT_ENTRY_POINT);

	lua_tpush(m_luaState, nEventID);

	// Try calling the script.
	err = lua_pcall(m_luaState, 
		1,	// 1 arguments
		0,	// 0 returned values
		NULL); // no error handler

	// Nothing returned, so we can finish up here.
	if (err == LUA_OK)
		return true;

	// Attempt to provide somewhat informative errors to help the user figure out what's wrong.
	switch (err)
	{
	case LUA_ERRRUN:
		printf("ERROR: A runtime error occurred within Lua script `%s`.\n", filename);
		break;

	case LUA_ERRMEM:
		printf("ERROR: Unable to allocate memory during execution of Lua script `%s`.\n", filename);
		break;

	case LUA_ERRERR:
		printf("ERROR: An error occurred during Lua script `%s`. Error handler failed.\n", filename);
		break;

	default:
		printf("ERROR: An unknown error occurred in Lua script `%s`.\n", filename);
		break;
	}

	// Is there an error set? That can be more useful than our generic error.
	if (lua_isstring(m_luaState, -1))
	{
		printf("ERROR: [%s] The following error was provided:\n%s\n",
			filename, lua_to<const char *>(m_luaState, -1));
	}

	return false;
}

CLuaScript::~CLuaScript()
{
	FastGuard lock(m_lock);
	if (m_luaState != NULL)
		lua_close(m_luaState);
}

CLuaEngine::~CLuaEngine()
{
}