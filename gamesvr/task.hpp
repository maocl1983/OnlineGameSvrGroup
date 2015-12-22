/*
 * =====================================================================================
 *
 *  @file  task.hpp 
 *
 *  @brief  任务系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef TASK_HPP_
#define TASK_HPP_

#include "common_def.hpp"

class Player;
class db_get_task_list_out;
class cli_get_task_list_out;
class task_xml_info_t;

enum task_type_t {
	em_task_type_start		= 0,
	em_task_type_story_instance 		= 1,	/*! 剧情副本 */
	em_task_type_skill_strength			= 2,	/*! 技能强化 */
	em_task_type_soldier_train			= 3,	/*! 小兵训练 */
	em_task_type_hero_lv				= 4,	/*! 武将等级 */
	em_task_type_hero_rank				= 5,	/*! 武将品阶 */
	em_task_type_equip_strength			= 6,	/*! 装备强化 */
	em_task_type_hero_honor_lv			= 7,	/*! 战功等级 */
	em_task_type_adventure				= 8,	/*! 奇遇 */
	em_task_type_soldier_star			= 9,	/*! 小兵升星 */
	em_task_type_arena					= 10,	/*! 竞技场 */
	em_task_type_btl_power				= 11,	/*! 战斗力 */
	em_task_type_soldier_rank			= 12,	/*! 小兵升阶 */
	em_task_type_role_lv				= 13,	/*! 主公等级  */
	em_task_type_inlaid_gem				= 14,	/*! 装备宝石镶嵌 */
	em_task_type_internal_affairs		= 15,	/*! 内政 */
	em_task_type_grant_title			= 16,	/*! 分封 */
	em_task_type_common_fight			= 17,	/*! 通用玩法 */

	em_task_type_end
};

/********************************************************************************/
/*									TaskManager									*/
/********************************************************************************/
struct task_info_t {
	uint32_t id;
	uint32_t completed;
	uint32_t reach_tms;
	uint32_t time;
	uint32_t reward_stat;
	const task_xml_info_t *base_info;
};
typedef std::map<uint32_t, task_info_t> TaskMap;

class TaskManager {
public:
	TaskManager(Player *p);
	~TaskManager();

	void init_task_list(db_get_task_list_out *p_in);
	
   	const task_info_t *get_task_info(uint32_t task_id);
	bool check_pre_task_is_completed(uint32_t task_id);
	int check_task(uint32_t task_type, uint32_t parm1=0, uint32_t parm2=0);
	int progress_task(uint32_t task_id, uint32_t reach_tms=0);
	int get_task_completed_reward(uint32_t task_id);

	void pack_client_task_list(cli_get_task_list_out &out);

private:
	Player *owner;
	TaskMap task_map;
};


/********************************************************************************/
/*								TaskXmlManager									*/
/********************************************************************************/
struct task_xml_info_t {
	uint32_t id;
	uint32_t type;
	uint32_t parm1;
	uint32_t parm2;
	uint32_t unlock_lv;
	uint32_t pre_task;
	uint32_t item_id1;
	uint32_t item_cnt1;
	uint32_t item_id2;
	uint32_t item_cnt2;
	uint32_t golds;
	uint32_t diamond;
};
typedef std::map<uint32_t, task_xml_info_t> TaskXmlMap;

struct task_type_xml_info_t {
	uint32_t type;
	std::vector<uint32_t> task_list;
};
typedef std::map<uint32_t, task_type_xml_info_t> TaskTypeXmlMap;

class TaskXmlManager {
public:
	TaskXmlManager();
	~TaskXmlManager();

	int read_from_xml(const char *filename);
	const task_xml_info_t* get_task_xml_info(uint32_t task_id);
	const task_type_xml_info_t* get_task_type_xml_info(uint32_t task_type);

private:
	int load_task_xml_info(xmlNodePtr cur);

private:
	TaskXmlMap task_map;
	TaskTypeXmlMap task_type_map;
};
extern TaskXmlManager task_xml_mgr;

#endif //TASK_HPP_
