/**
  * ============================================================
  *
  *  @file      timer.hpp
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

#ifndef TIMER_HPP_
#define TIMER_HPP_

#include "common_def.hpp"

enum timer_type_index{
	tm_connect_to_alarm_timely_index = 1,
	tm_connect_to_switch_timely_index,
	tm_keep_alive_noti_index,
	tm_check_active_player_index,
	tm_check_arena_count_index,
	tm_check_guild_count_index,
	tm_give_arena_daily_ranking_bonus_index,
	tm_add_player_skill_point_index,
	tm_add_player_soldier_train_point_index,
	tm_add_player_energy_index,
	tm_add_player_endurance_index,
	tm_add_player_adventure_index,
};

/************************************************************************/
/*                       Timer  Function								*/
/************************************************************************/
bool init_timer_callback_type();
int init_global_timer();
int keep_alive_noti(void *owner, void *data);
int check_active_player(void *owner, void *data);
int check_arena_count(void *owner, void *data);
int check_guild_count(void *owner, void *data);
int add_player_skill_point(void *owner, void *data);
int add_player_soldier_train_point(void *owner, void *data);
int give_arena_daily_ranking_bonus(void *owner, void *data);
int add_player_energy(void *owner, void *data);
int add_player_endurance(void *owner, void *data);
int add_player_adventure(void *owner, void *data);

/* @brief 时间戳类型 
 */
enum time_stamp_type_t {


    tm_stamp_end

};

/* @brief 排行榜定时器-相关类型
 */
enum kings_rank_timer_t {
	em_kings_rank_timer_min = 0,
	em_kings_rank_timer_max
};

/************************************************************************/
/*                      		Timer class                             */ 
/************************************************************************/

/* @brief 定时器管理类
 */
class Timer {
public:
    Timer();
    ~Timer();

};

//extern Timer timer_mgr;

/************************************************************************/
/*                      TimeStampXmlManage class                        */ 
/************************************************************************/

/* @brief 全局时间戳配置表信息
 */
struct time_stamp_xml_info_t {
    uint32_t id;
    uint32_t time_stamp;
};
typedef std::map<uint32_t, time_stamp_xml_info_t> TimeStampXmlMap;

/* @brief 全局时间戳配置表管理类
 */
class TimeStampXmlManage {
public:
    TimeStampXmlManage();
    ~TimeStampXmlManage();

    int read_from_xml(const char *filename);
	int init_time_stamp_info(time_stamp_xml_info_t &info, const char *day_str);

	//const time_stamp_xml_info_t* get_time_xml_info(uint32_t res_id);
	uint32_t get_time_stamp(uint32_t time_id);

private:
    int load_time_stamp_info(xmlNodePtr cur);

private:
    TimeStampXmlMap time_xml_map;
};

//extern TimeStampXmlManage time_stamp_xml_info;

#endif
