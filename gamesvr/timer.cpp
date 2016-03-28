/**
  * ============================================================
  *
  *  @file      timer.cpp
  *
  *  @brief     处理定时器/时间相关信息
  * 
  *  compiler   gcc4.1.2
  *
  *  platform   Linux
  *
  *  copyright:  kings, Inc. ShangHai CN. All rights reserved.
  *
  * ============================================================
  */
/*
#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"

#include "global_data.hpp"
#include "timer.hpp"
#include "player.hpp"
#include "utils.hpp"
#include "wheel_timer.h"
#include "arena.hpp"
#include "guild.hpp"
*/

#include "stdafx.hpp"
using namespace std;
using namespace project;

//TimeStampXmlManage time_stamp_xml_info;
//Timer timer_mgr;

/************************************************************************/
/*                       Timer  Function								*/
/************************************************************************/
/* @brief 初始化全局定时器，指定1号服务器作为定时器触发服务器
 */
int init_global_timer()
{
	wheel_init_timer();
	
	//拉取公会个数
	set_timeout(check_guild_count, 0, 0, 5 * 1000);

	//拉取竞技场人数
	set_timeout(check_arena_count, 0, 0, 10000);


	//启动每天晚上九点竞技场奖励定时器
	uint32_t now_tm = get_now_tv()->tv_sec;
	uint32_t nine_tm = utils_mgr->get_today_zero_tm() + 21 * 3600;
	if (now_tm >= nine_tm) {
		nine_tm += 24 * 3600;
	}
	set_timeout(give_arena_daily_ranking_bonus, 0, 0, (nine_tm - now_tm) * 1000);

	return 0;
}


/* @brief 心跳包，同步时间 
 */
int keep_alive_noti(void *owner, void *data)
{
	Player *p = reinterpret_cast<Player*>(owner);

	cli_keep_alive_out cli_out;
	cli_out.time = (uint32_t)time(NULL);
	//p->send_to_self(cli_keep_alive_cmd, &cli_out, 0);

	p->keep_alive_tm = add_timer_event(0, tm_keep_alive_noti_index, p, 0, 10000);
	return 0;
}


/* @brief 定时检测活跃用户
 */
int check_active_player(void *owner, void *data)
{
	Player *p = (Player *)owner;

	g_player_mgr->del_expire_player(p);

	return 0;
}

/* @brief 检测竞技场是否为空
 */
int check_arena_count(void *owner, void *data)
{
	return arena_mgr->get_arena_count();
}

/* @brief 检测公会个数是否为0
 */
int check_guild_count(void *owner, void *data)
{
	return guild_mgr->get_guild_count();
}

/* @brief 竞技场每日奖励发放定时器
 */
int give_arena_daily_ranking_bonus(void *owner, void *data)
{
	arena_mgr->give_arena_daily_ranking_bonus();

	set_timeout(give_arena_daily_ranking_bonus, 0, 0, 24 * 3600 * 1000);

	return 0;
}

/* @brief 技能点增加定时器
 */
int add_player_skill_point(void *owner, void *data)
{
	Player *p = (Player *)owner;

	p->add_skill_point_tm();

	//p->skill_point_tm = add_timer_event(0, add_player_skill_point, p, 0, SKILL_POINT_PER_SEC * 1000);
	p->skill_point_tm = add_timer_event(0, tm_add_player_skill_point_index, p, 0, 3 * 1000);

	return 0;	
}

/* @brief 小兵训练点增加定时器
 */
int add_player_soldier_train_point(void *owner, void *data)
{
	Player *p = (Player *)owner;

	p->add_soldier_train_point_tm();

	p->soldier_train_point_tm = add_timer_event(0, tm_add_player_soldier_train_point_index, p, 0, SOLDIER_TRAIN_POINT_PER_SEC * 1000);

	return 0;	
}

/* @brief 体力恢复定时器
 */
int add_player_energy(void *owner, void *data)
{
	Player *p = (Player *)owner;

	p->add_energy_tm();

	p->energy_tm = add_timer_event(0, tm_add_player_energy_index, p, 0, ENERGY_PER_SEC * 1000);

	return 0;	
}

/* @brief 耐力恢复定时器
 */
int add_player_endurance(void *owner, void *data)
{
	Player *p = (Player *)owner;

	p->add_endurance_tm();

	p->endurance_tm = add_timer_event(0, tm_add_player_endurance_index, p, 0, ENDURANCE_PER_SEC * 1000);

	return 0;	
}

/* @brief 奇遇点恢复定时器
 */
int add_player_adventure(void *owner, void *data)
{
	Player *p = (Player *)owner;

	p->add_adventure_tm();

	p->adventure_tm = add_timer_event(0, tm_add_player_adventure_index, p, 0, ADVENTURE_PER_SEC * 1000);

	return 0;	
}

#define REGISTER_TIMER_TYPE(nbr_, cb_) \
	do {\
		if (register_timer_callback(nbr_, cb_, max_timer_type) == -1) {\
			ERROR_LOG("register timer callback error!");\
			return false;\
		}\
	} while(0)

bool init_timer_callback_type()
{
	memset(tcfs, 0x00, sizeof(tcfs));
	REGISTER_TIMER_TYPE(tm_keep_alive_noti_index , keep_alive_noti);
	REGISTER_TIMER_TYPE(tm_connect_to_switch_timely_index, connect_to_switch_timely);
	REGISTER_TIMER_TYPE(tm_connect_to_alarm_timely_index, connect_to_alarm_timely);
	REGISTER_TIMER_TYPE(tm_check_active_player_index , check_active_player);
	REGISTER_TIMER_TYPE(tm_check_arena_count_index , check_arena_count);
	REGISTER_TIMER_TYPE(tm_check_guild_count_index , check_guild_count);
	REGISTER_TIMER_TYPE(tm_give_arena_daily_ranking_bonus_index , give_arena_daily_ranking_bonus);
	REGISTER_TIMER_TYPE(tm_add_player_skill_point_index , add_player_skill_point);
	REGISTER_TIMER_TYPE(tm_add_player_soldier_train_point_index , add_player_soldier_train_point);
	REGISTER_TIMER_TYPE(tm_add_player_energy_index , add_player_energy);
	REGISTER_TIMER_TYPE(tm_add_player_endurance_index , add_player_endurance);
	REGISTER_TIMER_TYPE(tm_add_player_adventure_index , add_player_adventure);
	return  true;
}
/************************************************************************/
/*                      		Timer class                             */ 
/************************************************************************/
Timer::Timer()
{
	
}

Timer::~Timer()
{

}



/************************************************************************/
/*                      TimeStampXmlManage class                        */ 
/************************************************************************/
TimeStampXmlManage::TimeStampXmlManage()
{
	time_xml_map.clear();
}

TimeStampXmlManage::~TimeStampXmlManage()
{

}

int
TimeStampXmlManage::read_from_xml(const char *filename)
{
    xmlDocPtr doc = xmlParseFile(filename);
    if (!doc) {
        throw XmlParseError(std::string("failed to parse timestamp file '") + filename + "'");
        ERROR_LOG("failed to parse timestamp file!");
    }

    xmlNodePtr cur = xmlDocGetRootElement(doc);
    if (!cur) {
        xmlFreeDoc(doc);
        throw XmlParseError(std::string("xmlDocGetRootElement error when loading timestamp file '") + filename + "'");
        ERROR_LOG("xmlDocGetRootElement error when loading timestamp file!");
    }

    int ret = load_time_stamp_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	return ret;
}

int
TimeStampXmlManage::load_time_stamp_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while(cur) {
		if (!xmlStrcmp(cur->name, reinterpret_cast<const xmlChar*>("Time"))) {
			TimeStampXmlMap *p_map = &time_xml_map;
			uint32_t time_id = 0;
			get_xml_prop(time_id, cur, "id");
            TimeStampXmlMap::iterator it = p_map->find(time_id);
            if (it != p_map->end()) {
                KERROR_LOG(0, "time id existed! time_id=%u", time_id);
                return -1;
            }
			if (time_id >= tm_stamp_end) {
                KERROR_LOG(0, "invali time id! time_id=%u", time_id);
                return -1;
			}

            time_stamp_xml_info_t info = { 0 };
            info.id = time_id;

			char date[64];
			get_xml_prop_raw_str(date, cur, "date");
			init_time_stamp_info(info, date);

            KTRACE_LOG(0,"time stamp xml info id=%u,time_stamp=%u",info.id,info.time_stamp);
            p_map->insert(TimeStampXmlMap::value_type(time_id, info));
		}
		cur = cur->next;
	}

    return 0;
}

int
TimeStampXmlManage::init_time_stamp_info(time_stamp_xml_info_t &info, const char *day_str)
{
	int year, month, day, hour, min, sec;
	sscanf(day_str, "%4d-%d-%d %2d:%2d:%2d", &year, &month, &day, &hour, &min, &sec);

	info.time_stamp = utils_mgr->mk_tm(year, month, day, hour, min, sec);

	return 0;
}

uint32_t
TimeStampXmlManage::get_time_stamp(uint32_t time_id)
{
    const TimeStampXmlMap *p_map = &time_xml_map;
    TimeStampXmlMap::const_iterator it = p_map->find(time_id);
    if (it == p_map->end()) {
        return 0;
    }

	const time_stamp_xml_info_t *p_info = &(it->second);
    return p_info->time_stamp;
}

/************************************************************************/
/*                       Client  Request                                */
/************************************************************************/
int cli_keep_alive(Player *p, Cmessage *c_in)
{
	cli_keep_alive_out cli_out;
	cli_out.time = get_now_tv()->tv_sec;
	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}
