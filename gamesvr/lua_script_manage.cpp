/*
 * =====================================================================================
 *
 *  @file  lua_scipt_manage.hpp 
 *
 *  @brief  处理lua管理
 *
 *  compiler  gcc4.4.7 
 *	
 *  platform  Linux
 *
 * copyright:  kings, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
*/
#include "lua_script_manage.hpp"
#include "lua_log.hpp"
#include "restriction.hpp"
#include "player.hpp"

LuaScriptManage lua_script_mgr;
LuaScriptManage::LuaScriptManage()
{
	l_state = luaL_newstate();
	luaL_openlibs(l_state);

	if (!LoadLuaFile()) {
		lua_close(l_state);
		l_state = NULL;
	}
}

LuaScriptManage::~LuaScriptManage()
{
	if (l_state)
	{
		lua_close(l_state);
		l_state = NULL;
	}
}

bool LuaScriptManage::LoadLuaFile()
{
	luaopen_restriction(l_state);

	luaL_requiref(l_state, "log", luaopen_log, 0);
	if (luaL_dofile(l_state, "./lualib/test.lua")) {
		ERROR_LOG("load lua file error!");
		return false;
	}
	DEBUG_LOG("load lua file ok!");
	//lua_pcall(l_state, 0, LUA_MULTRET, 0);

	return true;
}

int LuaScriptManage::DoTest(Player* p)
{
 	lua_getglobal(l_state, "test");
	lua_pushlightuserdata(l_state, p);
	lua_call(l_state, 1, 1);

	int ret = (int)lua_tonumber(l_state, -1);
	lua_pop(l_state, 1);
	DEBUG_LOG("lua test ret=%d", ret);
 	
	/*lua_getglobal(l_state, "testSimple");
	lua_pushnumber(l_state, 1);
	lua_pushnumber(l_state, 2);
	lua_call(l_state, 2, 1);

	int ret = (int)lua_tonumber(l_state, -1);
	lua_pop(l_state, 1);
	DEBUG_LOG("lua test ret=%d", ret);
	*/
}





