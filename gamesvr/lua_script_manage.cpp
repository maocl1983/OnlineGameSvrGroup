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
#include <dirent.h>
#include <vector>
#include <unistd.h>
#include "lua_script_manage.hpp"
#include "lua_log.hpp"
#include "restriction.hpp"
#include "utils.hpp"
#include "player.hpp"
#include "global_data.hpp"

using namespace std;
#define pushuserdata(_p, lua_state, metaname) \
	do { \
		*(typeof(_p)*)lua_newuserdata(lua_state, sizeof(_p)) = _p; \
		luaL_getmetatable(lua_state, metaname); \
		lua_setmetatable(lua_state, -2); \
	}while (0)

//LuaScriptManage lua_script_mgr;
static int tracelv = 1;
static int traceback(lua_State* l)
{
	//DEBUG_LOG("trace:get state top=%d", lua_gettop(l));
	const char *msg = lua_tostring(l, 1);
	if (msg) {
		luaL_traceback(l, l, msg, 1);
	} else {
		lua_pushliteral(l, "(no error message)");
	}
	return 1;
}

LuaScriptManage::LuaScriptManage()
{
	l_state = luaL_newstate();
	luaL_openlibs(l_state);
	
	/*if (!LoadLuaFile()) {
		lua_close(l_state);
		l_state = NULL;
	}*/
}

LuaScriptManage::~LuaScriptManage()
{
	if (l_state)
	{
		lua_close(l_state);
		l_state = NULL;
	}
}

bool LuaScriptManage::LoadLuaFile(const char* path)
{
	DIR *pDir;
	struct dirent *file;
	//DEBUG_LOG("1:get state top=%d", lua_gettop(l_state));
	luaL_requiref(l_state, "log", luaopen_log, 0);
	luaL_requiref(l_state, "utils", luaopen_utils, 0);
	luaL_requiref(l_state, "player", luaopen_player, 0);
	lua_settop(l_state, 0);

	vector<string> lua_file_vec;
	if (!(pDir = opendir(path))) {
		ERROR_LOG("load lua file path[%s] error!", path);
		return false;
	}
	chdir(path);
	while ((file = readdir(pDir)) != NULL) {
		if (strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0)
			continue;
		string lua_file_name(path);
		lua_file_name.append(file->d_name);
		lua_file_vec.push_back(lua_file_name);
	}
	chdir("..");
	closedir(pDir);

	for (size_t i = 0; i < lua_file_vec.size(); i++) {
		if (luaL_dofile(l_state, lua_file_vec[i].c_str())) {
			ERROR_LOG("load lua file error![%s]", lua_file_vec[i].c_str());
			return false;
		}
		DEBUG_LOG("load lua file [%s]", lua_file_vec[i].c_str());
	}
	DEBUG_LOG("load lua file ok!");
	
	int top = lua_gettop(l_state);
	if (top == tracelv-1) {
		lua_pushcfunction(l_state, traceback);
	}

	return true;
}

int LuaScriptManage::DoTest(Player* p)
{
	//test1
 	lua_getglobal(l_state, "test");
	//lua_pushlightuserdata(l_state, p);
	
	/*Player **lua_player = (Player **)lua_newuserdata(l_state, sizeof(Player*));
 	*lua_player	= p;
	luaL_getmetatable(l_state, PLAYERMETA);	
	lua_setmetatable(l_state, -2);*/
	pushuserdata(p, l_state, PLAYERMETA);

	if (lua_pcall(l_state, 1, 1, tracelv) != LUA_OK)
	{
		ERROR_LOG("lua function test error:%s", lua_tostring(l_state, -1));
		lua_pop(l_state, 1);
		return -1;
	}

	int ret = (int)lua_tonumber(l_state, -1);
	lua_pop(l_state, 1);
	DEBUG_LOG("lua test ret=%d", ret);

	return 0;
}

int LuaScriptManage::daliy_checkin(Player* p)
{
 	lua_getglobal(l_state, "daliy_checkin");
	pushuserdata(p, l_state, PLAYERMETA);

	if (lua_pcall(l_state, 1, 1, tracelv) != LUA_OK)
	{
		ERROR_LOG("lua function test error:%s", lua_tostring(l_state, -1));
		lua_pop(l_state, 1);
		return -1;
	}

	int ret = (int)lua_tonumber(l_state, -1);
	lua_pop(l_state, 1);

	return ret;
}






