/*
 * =====================================================================================
 *
 *  @file  lua_log.cpp 
 *
 *  @brief  处理lua打印日志
 *
 *  compiler  gcc4.4.7 
 *	
 *  platform  Linux
 *
 * copyright:  kings, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#include "player.hpp"
#include "log_thread.hpp"


static int
lua_T_DEBUG_LOG(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);

	T_DEBUG_LOG("from lua--%s", msg);

	return 0;
}

static int
lua_T_KDEBUG_LOG(lua_State *L)
{
	Player *p = (Player *)lua_touserdata(L, 1);
	const char *msg = luaL_checkstring(L, 2);

	T_KDEBUG_LOG(p->user_id, "from lua--%s", msg);

	return 0;
}

static int
lua_T_ERROR_LOG(lua_State *L)
{
	const char *msg = luaL_checkstring(L, 1);

	T_ERROR_LOG("from lua--%s", msg);

	return 0;
}

static int
lua_T_KERROR_LOG(lua_State *L)
{
	Player *p = (Player *)lua_touserdata(L, 1);
	const char *msg = luaL_checkstring(L, 2);

	T_KERROR_LOG(p->user_id, "from lua--%s", msg);

	return 0;
}


int luaopen_log(lua_State *L)
{
	luaL_checkversion(L);

	luaL_Reg l[] = {
		{"T_DEBUG_LOG", lua_T_DEBUG_LOG},
		{"T_KDEBUG_LOG", lua_T_KDEBUG_LOG},
		{"T_ERROR_LOG", lua_T_ERROR_LOG},
		{"T_KERROR_LOG", lua_T_KERROR_LOG},
		{NULL, NULL}
	};

	luaL_newlib(L, l);

/*
	lua_register(L, "T_DEBUG_LOG", lua_T_DEBUG_LOG);
	lua_register(L, "T_KDEBUG_LOG", lua_T_KDEBUG_LOG);
	lua_register(L, "T_ERROR_LOG", lua_T_ERROR_LOG);
	lua_register(L, "T_KERROR_LOG", lua_T_KERROR_LOG);
*/
	return 1;
}
