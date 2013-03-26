#include "stdafx.h"
#include "LuaEngine.h"
#include "../shared/RWLock.h"

#include "User.h"
#include "Npc.h"

// define global functions to be called from Lua (e.g. myrand())
DEFINE_LUA_FUNCTION_TABLE(g_globalFunctions, 
	MAKE_LUA_FUNCTION(myrand)
);

CLuaEngine::CLuaEngine() : m_lock(new RWLock())
{
}

CLuaScript::CLuaScript() : m_luaState(NULL), m_lock(new FastMutex())
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

// TO-DO: Pull an available script for use
CLuaScript * CLuaEngine::SelectAvailableScript()
{
	return &m_luaScript;
}

bool CLuaEngine::ExecuteScript(CUser * pUser, CNpc * pNpc, int32 nEventID, const char * filename)
{
	ScriptBytecodeMap::iterator itr;
	bool result = false;

	m_lock->AcquireReadLock();
	itr = m_scriptMap.find(filename);
	if (itr == m_scriptMap.end())
	{
		// Build full path to script
		char szPath[_MAX_PATH];
		sprintf_s(szPath, sizeof(szPath), LUA_SCRIPT_DIRECTORY "%s", filename);

		// Release the read lock (we're not reading anymore)
		m_lock->ReleaseReadLock();

		// Attempt to comppile 
		BytecodeBuffer bytecode;
		bytecode.reserve(LUA_SCRIPT_BUFFER_SIZE);
		if (!SelectAvailableScript()->CompileScript(szPath, bytecode))
		{
			printf("ERROR: Could not compile Lua script `%s`.\n", szPath);
			return false;
		}

		// Acquire the write lock (we're adding the compiled script)
		m_lock->AcquireWriteLock();

#if !defined(LUA_SCRIPT_CACHE_DISABLED)
		// Add the script to our map
		m_scriptMap[filename] = bytecode;
#endif

		// Now that we have the bytecode, we can use it.
		result = SelectAvailableScript()->ExecuteScript(pUser, pNpc, nEventID, filename, bytecode);

		// Done using the lock.
		m_lock->ReleaseWriteLock();
	}
	else
	{
		// Already have the bytecode, so now we need to use it.
		result = SelectAvailableScript()->ExecuteScript(pUser, pNpc, nEventID, filename, itr->second);

		// Done using the lock.
		m_lock->ReleaseReadLock();
	}

	return result;
}

bool CLuaScript::CompileScript(const char * filename, BytecodeBuffer & buffer)
{
	// ensure that we wait until the last user's done executing their script.
	FastGuard lock(m_lock);

	/* Attempt to load the file */
	int err = luaL_loadfile(m_luaState, filename);

	// If something bad happened, try to find an error.
	if (err != LUA_OK)
	{
		RetrieveLoadError(err, filename);
		return false;
	}

	// Everything's OK so far, the script has been loaded, now we need to start dumping it to bytecode.
	err = lua_dump(m_luaState, (lua_Writer)LoadBytecodeChunk, &buffer);
	if (err
		|| buffer.empty())
	{
		printf("ERROR: Failed to dump the Lua script `%s` to bytecode.\n", filename);
		return false;
	}

	// Load up the script & revert the stack.
	// This step's only here for cleanup purposes.
	err = lua_pcall(m_luaState, 0, LUA_MULTRET, 0);
	if (err != LUA_OK)
	{
		RetrieveLoadError(err, filename);
		return false;
	}

	// Compiled!
	return true;
}

// Callback for lua_dump() to read in each chunk of bytecode.
int CLuaScript::LoadBytecodeChunk(lua_State * L, uint8 * bytes, size_t len, BytecodeBuffer * buffer)
{
	for (size_t i = 0; i < len; i++)
		buffer->push_back(bytes[i]);

	return 0;
}

bool CLuaScript::ExecuteScript(CUser * pUser, CNpc * pNpc, int32 nEventID, const char * filename, BytecodeBuffer & bytecode)
{
	// Ensure that we wait until the last user's done executing their script.
	FastGuard lock(m_lock);

	/* Attempt to run the script. */

	// Load the buffer with our bytecode.
	int err = luaL_loadbuffer(m_luaState, reinterpret_cast<const char *>(&bytecode[0]), bytecode.size(), NULL);
	if (err != LUA_OK)
	{
		RetrieveLoadError(err, filename);
		return false;
	}

	// Entry point requires 1 arguments: the event ID.
	// The user & NPC instances are globals.

	lua_tsetglobal(m_luaState, LUA_SCRIPT_GLOBAL_USER, pUser);
	lua_tsetglobal(m_luaState, LUA_SCRIPT_GLOBAL_NPC, pNpc);

	// Find & assign script's entry point to the stack
	lua_getglobal(m_luaState, LUA_SCRIPT_ENTRY_POINT);

	lua_tpush(m_luaState, nEventID);

	// Try calling the script's entry point (Main()).
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

void CLuaScript::RetrieveLoadError(int err, const char * filename)
{
	switch (err)
	{
	case LUA_ERRFILE:
		printf("ERROR: Unable to load Lua script `%s`.\n", filename);
		break;

	case LUA_ERRSYNTAX:
		printf("ERROR: There was a error with the syntax of Lua script `%s`.\n", filename);
		break;

	case LUA_ERRMEM:
		printf("ERROR: Unable to allocate memory for Lua script `%s`.\n", filename);
		break;

	default:
		printf("ERROR: An unknown error occurred while loading Lua script `%s`.\n", filename);
		break;
	}

	// Is there an error set? That can be more useful than our generic error.
	if (lua_isstring(m_luaState, -1))
	{
		printf("ERROR: [%s] The following error was provided:\n%s\n",
			filename, lua_to<const char *>(m_luaState, -1));
	}
}

CLuaScript::~CLuaScript()
{
	m_lock->Acquire();
	if (m_luaState != NULL)
		lua_close(m_luaState);
	m_lock->Release();
	delete m_lock;
}


CLuaEngine::~CLuaEngine()
{
	delete m_lock;
}