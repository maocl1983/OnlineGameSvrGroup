/*
 * =====================================================================================
 *
 *  @file  achievement.hpp 
 *
 *  @brief  成就系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef ACHIEVEMENT_HPP_
#define ACHIEVEMENT_HPP_

#include "common_def.hpp"

class Player;
class db_get_achievement_list_out;
class cli_get_achievement_list_out;
class achievement_xml_info_t;

enum achievement_type_t {
	em_achievement_type_start				= 0,
	em_achievement_type_story_instance 		= 1,	/*! 剧情副本 */
	em_achievement_type_skill_strength		= 2,	/*! 技能强化 */
	em_achievement_type_soldier_train		= 3,	/*! 小兵训练 */
	em_achievement_type_hero_lv				= 4,	/*! 武将等级 */
	em_achievement_type_hero_rank			= 5,	/*! 武将品阶 */
	em_achievement_type_equip_strength		= 6,	/*! 装备强化 */
	em_achievement_type_hero_honor_lv		= 7,	/*! 战功等级 */
	em_achievement_type_adventure			= 8,	/*! 奇遇 */
	em_achievement_type_soldier_star		= 9,	/*! 小兵升星 */
	em_achievement_type_arena				= 10,	/*! 竞技场 */
	em_achievement_type_btl_power			= 11,	/*! 战斗力 */
	em_achievement_type_soldier_rank		= 12,	/*! 小兵升阶 */
	em_achievement_type_btl_soul_rank		= 13,	/*! 战魂品质 */
	em_achievement_type_btl_soul_cat		= 14,	/*! 战魂种类 */
	en_achievement_type_btl_soul_lv			= 15,	/*! 战魂等级 */

	em_achievement_type_end
};

/********************************************************************************/
/*									AchievementManager									*/
/********************************************************************************/
struct achievement_info_t {
	uint32_t id;
	uint32_t completed;
	uint32_t reach_tms;
	uint32_t time;
	uint32_t reward_stat;
	const achievement_xml_info_t *base_info;
};
typedef std::map<uint32_t, achievement_info_t> AchievementMap;

class AchievementManager {
public:
	AchievementManager(Player *p);
	~AchievementManager();

	void init_achievement_list(db_get_achievement_list_out *p_in);
	
   	const achievement_info_t *get_achievement_info(uint32_t achievement_id);
	bool check_pre_achievement_is_completed(uint32_t achievement_id);
	int check_achievement(uint32_t achievement_type, uint32_t parm1=0, uint32_t parm2=0);
	int progress_achievement(uint32_t achievement_id, uint32_t reach_tms=0);
	int get_achievement_completed_reward(uint32_t achievement_id);

	void pack_client_achievement_list(cli_get_achievement_list_out &out);

private:
	Player *owner;
	AchievementMap achievement_map;
};


/********************************************************************************/
/*								AchievementXmlManager									*/
/********************************************************************************/
struct achievement_xml_info_t {
	uint32_t id;
	uint32_t type;
	uint32_t parm1;
	uint32_t parm2;
	uint32_t unlock_lv;
	uint32_t pre_achievement;
	uint32_t item_id1;
	uint32_t item_cnt1;
	uint32_t item_id2;
	uint32_t item_cnt2;
	uint32_t golds;
	uint32_t diamond;
};
typedef std::map<uint32_t, achievement_xml_info_t> AchievementXmlMap;

struct achievement_type_xml_info_t {
	uint32_t type;
	std::vector<uint32_t> achievement_list;
};
typedef std::map<uint32_t, achievement_type_xml_info_t> AchievementTypeXmlMap;

class AchievementXmlManager {
public:
	AchievementXmlManager();
	~AchievementXmlManager();

	int read_from_xml(const char *filename);
	const achievement_xml_info_t* get_achievement_xml_info(uint32_t achievement_id);
	const achievement_type_xml_info_t* get_achievement_type_xml_info(uint32_t achievement_type);

private:
	int load_achievement_xml_info(xmlNodePtr cur);

private:
	AchievementXmlMap achievement_map;
	AchievementTypeXmlMap achievement_type_map;
};
//extern AchievementXmlManager achievement_xml_mgr;

#endif //ACHIEVEMENT_HPP_
