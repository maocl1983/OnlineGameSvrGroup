/**
  * ============================================================
  *
  *  @file      restriction.cpp
  *
  *  @brief     处理限制的相关信息
  * 
  *  compiler   gcc4.1.2
  *
  *  platform   Linux
  *
  *  copyright:  kings, Inc. ShangHai CN. All rights reserved.
  *
  * ============================================================
  */

#include "./proto/xseer_online.hpp" 
#include "./proto/xseer_online_enum.hpp" 
#include "./proto/xseer_db.hpp" 
#include "./proto/xseer_db_enum.hpp" 

#include "restriction.hpp"
#include "player.hpp"
#include "dbroute.hpp"
#include "utils.hpp"
#include "log_thread.hpp"

using namespace std;
using namespace project;

ResXmlManage res_xml_mgr;

/************************************************************************/
/*                       Restriction class                               */ 
/************************************************************************/
Restriction::Restriction(Player *p) : owner(p)
{
	res_map.clear();
}

Restriction::~Restriction()
{

}

const res_cache_info_t*
Restriction::get_res_info(uint32_t type)
{
	ResCacheMap::iterator it = res_map.find(type);
	if (it != res_map.end()) {
		return &it->second;
	}
	return NULL;
}

void
Restriction::init_res_value_info(std::vector<db_res_info_t> *p_vec)
{
	db_clear_overdue_res_in db_in;
	for (uint32_t i = 0; i < p_vec->size(); i++) {
		uint32_t type = (*p_vec)[i].type;
		uint32_t value = (*p_vec)[i].value;
		uint32_t expire_tm = (*p_vec)[i].expire_tm;
		//先过滤掉过期的带期限限制
		if (type >= 50000) {//带期限限制
			const res_xml_info_t *p_info = res_xml_mgr.get_res_xml_info(type);
			if (!p_info || expire_tm < p_info->expire_tm) {//过期了，由玩家触发清除数据
				db_in.expire_vec.push_back(type);
			} else {
				res_map[type].value = value;
				res_map[type].expire_tm = expire_tm;
			}
		} else {
			res_map[type].value = value;
		}

		TRACE_LOG("init res info[%d %d %u %d]", type, value, expire_tm, (int)res_map.size());
	}

	if (db_in.expire_vec.size() > 0) {
		send_msg_to_dbroute(owner, db_clear_overdue_res_cmd, &db_in, owner->user_id);
	}

	return;
}


uint32_t
Restriction::get_res_value(uint32_t type)
{
	uint32_t value = 0;
	if (type < 50000) {
		value = res_map[type].value;
	} else {
		value = get_res_value_with_expire(type);
	}

	return value;
}

void
Restriction::set_res_value(uint32_t type, uint32_t value, uint32_t expire_tm)
{
	if (type < 50000) {
		res_map[type].value = value;

		db_set_res_value_in db_in;
		db_in.type = type;
		db_in.value = value;
		send_msg_to_dbroute(0, db_set_res_value_cmd, &db_in, owner->user_id);
	} else {
		set_res_value_with_expire(type, value, expire_tm);
	}

	KTRACE_LOG(owner->user_id, "set res info[%d %d]", type, value);

	return;
}

uint32_t
Restriction::get_res_value_with_expire(uint32_t type)
{
	uint32_t value = res_map[type].value;
	if (value != 0) {
		if (res_map[type].expire_tm < get_now_tv()->tv_sec) {
			//过期了,数据库中不用更新，每次登录自然会清
			res_map.erase(type);
			value = 0;
		}
	}

	return value;
}

void
Restriction::set_res_value_with_expire(uint32_t type, uint32_t value, uint32_t expire_tm)
{
	if (expire_tm == 0) {
		const res_xml_info_t *p_info = res_xml_mgr.get_res_xml_info(type);
		if (p_info) {
			uint32_t year = p_info->year;
			uint32_t month = p_info->month;
			uint32_t day = p_info->day;
			uint32_t hour = p_info->hour;
			uint32_t min = p_info->min;
			uint32_t sec = p_info->sec;
			expire_tm = utils_mgr.mk_tm(year, month, day, hour, min, sec);
		}
	}
	res_map[type].value = value;
	if (expire_tm != 0) {//修改过期时间
		res_map[type].expire_tm = expire_tm;
	}

	db_set_res_value_in db_in;
	db_in.type = type;
	db_in.value = value;
	db_in.expire_tm = res_map[type].expire_tm;
	send_msg_to_dbroute(0, db_set_res_value_cmd, &db_in, owner->user_id);
	KTRACE_LOG(owner->user_id, "set expire res info[%d %d %u]", type, value, expire_tm);

	return;
}


/************************************************************************/
/*                        ResXmlManage class                            */ 
/************************************************************************/
ResXmlManage::ResXmlManage()
{
	res_xml_map.clear();
}

ResXmlManage::~ResXmlManage()
{

}

const res_xml_info_t*
ResXmlManage::get_res_xml_info(uint32_t res_id)
{
    const ResXmlMap *p_map = &res_xml_map;
    ResXmlMap::const_iterator it = p_map->find(res_id);
    if (it == p_map->end()) {
        return 0;
    }
    return &(it->second);
}

int
ResXmlManage::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

    int ret = load_res_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);
	return ret;
}

int
ResXmlManage::load_res_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while(cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("ResType"))) {
			ResXmlMap *p_map = &res_xml_map;
			uint32_t res_id = 0;
			get_xml_prop(res_id, cur, "id");
            ResXmlMap::iterator it = p_map->find(res_id);
            if (it != p_map->end()) {
                KERROR_LOG(0, "res id existed! res=%u", res_id);
                return -1;
            }

            res_xml_info_t info = { 0 };
            info.id = res_id;

			char expire_day[64];
			get_xml_prop_raw_str(expire_day, cur, "expireDay");
			init_res_day_info(info, expire_day);
			
            KTRACE_LOG(0,"res xml info id=%u,year=%u,month=%u,day=%u,hour=%u,min=%u,sec=%u",
					info.id,info.year,info.month,info.day,info.hour,info.min,info.sec);
            
			p_map->insert(ResXmlMap::value_type(res_id, info));
		}
		cur = cur->next;
	}

    return 0;
}

int
ResXmlManage::init_res_day_info(res_xml_info_t &info, const char *day_str)
{
	int year, month, day, hour, min, sec;
    sscanf(day_str, "%4d-%d-%d %2d:%2d:%2d", &year, &month, &day, &hour, &min, &sec);

	info.year = year;
	info.month = month;
	info.day = day;
	info.hour = hour;
	info.min = min;
	info.sec = sec;

	return 0;
}



/************************************************************************/
/*                       Client  Request                                */
/************************************************************************/

/* @brief 获得每日限制信息 
 */
int cli_get_res_value(Player *p, Cmessage *c_in)
{
    cli_get_res_value_in *p_in = P_IN;
    cli_get_res_value_out cli_out;

	uint32_t cur_value = p->res_mgr->get_res_value(p_in->type);

	cli_out.type = p_in->type;
	cli_out.value = cur_value;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 批量获得每日限制信息 
 */
int cli_get_res_value_list(Player *p, Cmessage *c_in)
{
    cli_get_res_value_list_in *p_in = P_IN;
    cli_get_res_value_list_out cli_out;

	uint32_t type_num = p_in->type_list.size();
	if (type_num > 100) {
		KERROR_LOG(p->user_id, "too many res type! type_num=%u", type_num);
		return -1;
	}

	for (uint32_t i = 0; i < type_num; i++) {
		cli_res_info_t info;
		info.type = p_in->type_list[i];
		info.value = p->res_mgr->get_res_value(info.type);
		cli_out.value_list.push_back(info);
	}

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}


/************************************************************************/
/*                       dbser return   Request                         */
/************************************************************************/
int db_get_res_value_info(Player *p, Cmessage* c_in, uint32_t ret)
{
    CHECK_DB_ERR(p,ret);

	db_get_res_value_info_out* p_in = P_IN;

    if (p->wait_cmd == cli_proto_login_cmd) {
		p->res_mgr->init_res_value_info(&(p_in->res_value));

		p->login_step++;
		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u GET RES LIST", p->login_step);

		p->deal_something_when_login();

		cli_proto_login_out cli_out;
		p->pack_player_login_info(cli_out);
		p->send_to_self(p->wait_cmd, &cli_out, 0); 

		//拉取物品
		return send_msg_to_dbroute(p, db_get_player_items_info_cmd, 0, p->user_id);
	}

    return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}

/************************************************************************/
/*                       		Lua Interface        	                */
/************************************************************************/
static int 
lua_get_res_value(lua_State *L)
{
	Player *p = (Player *)lua_touserdata(L, 1);
	uint32_t res_type = luaL_checkinteger(L, 2);

	uint32_t res_value = p->res_mgr->get_res_value(res_type);
	lua_pushinteger(L, res_value);

	return 1;
}

static int 
lua_set_res_value(lua_State *L)
{
	Player *p = (Player *)lua_touserdata(L, 1);
	uint32_t res_type = luaL_checkinteger(L, 2);
	uint32_t res_value = luaL_checkinteger(L, 3);

	p->res_mgr->set_res_value(res_type, res_value);

	return 0;
}

int
luaopen_restriction(lua_State *L)
{
	/*luaL_checkversion(L);

	luaL_Reg l[] = {
		{"get_res_value", lua_get_res_value},
		{"set_res_value", lua_set_res_value},
		{NULL, NULL},
	};


	luaL_newlib(L, l);*/
	
	lua_register(L, "get_res_value", lua_get_res_value);
	lua_register(L, "set_res_value", lua_set_res_value);

	return 1;
}
