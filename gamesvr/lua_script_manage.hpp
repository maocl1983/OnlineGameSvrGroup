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
#ifndef LUA_SCIPT_MANAGE_HPP_
#define LUA_SCIPT_MANAGE_HPP_

#include "common_def.hpp"

class Player;
class LuaScriptManage {
public:
	LuaScriptManage();
	~LuaScriptManage();

	bool LoadLuaFile();
	int DoTest(Player* p);
private:
	lua_State *l_state;
};

extern LuaScriptManage lua_script_mgr;
#endif
