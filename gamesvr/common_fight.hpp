/*
 * =====================================================================================
 *
 *  @file  common_fight.hpp 
 *
 *  @brief  处理一些通用玩法
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#ifndef COMMON_FIGHT_HPP_
#define COMMON_FIGHT_HPP_

//#include "common_fight.hpp"

class Player;
class item_info_t;
class cli_item_info_t;
class cli_get_common_fight_panel_info_out;

enum common_fight_type_t {
	em_common_fight_start = 0,

	em_common_fight_normal_pve		= 1,	/*! 普通PVE */
	em_common_fight_arena			= 2,	/*! 竞技场 */
	em_common_fight_treasure		= 3,	/*! 夺宝 */
	em_common_fight_trial 			= 4,	/*! 试炼-技能战 */
	em_common_fight_soldier			= 5, 	/*! 兵战 */
	em_common_fight_defend			= 6,	/*! 守城 */
	em_common_fight_guide			= 7,	/*! 新手指引 */
	em_common_fight_golds			= 8,	/*! 金币 */
	em_common_fight_kill_guard		= 9,	/*! 过关斩将*/
	em_common_fight_riding_alone	= 10,	/*! 千里走单骑-百鬼夜行 */

	em_common_fight_end
};

/********************************************************************************/
/*							CommonFight Class									*/
/********************************************************************************/

class CommonFight {
public:
	CommonFight(Player *p);
	~CommonFight();

	void set_kill_guard_idx(uint32_t idx) {kill_guard_idx = idx;}
	uint32_t get_kill_guard_idx() {return kill_guard_idx;}
	int get_kill_guard_unlock_idx();

	void set_common_fight_idx(uint32_t idx){common_fight_idx = idx;}
	void set_common_fight_cd_tm(uint32_t type);
	uint32_t get_common_fight_idx() {return common_fight_idx;}
	int get_common_fight_unlock_idx(uint32_t type);

	bool check_common_fight_tms_is_enough(uint32_t type);
	int get_common_fight_cd_tm(uint32_t type);

	int random_rewards(uint32_t type, uint32_t idx);
	int battle_end(uint32_t type, uint32_t is_win, uint32_t &left_tms, std::vector<cli_item_info_t> &items_vec);

	void pack_common_fight_rewards(std::vector<cli_item_info_t> &vec);
	void pack_common_fight_info(uint32_t type, cli_get_common_fight_panel_info_out &cli_out);

private:
	Player *owner;
	uint32_t kill_guard_idx;
	uint32_t common_fight_idx;
	std::vector<item_info_t> rewards;
};

/********************************************************************************/
/*							CommonFightXmlManager Class							*/
/********************************************************************************/
struct common_fight_step_xml_info_t {
	uint32_t step;
	uint32_t unlock_lv;
};
typedef std::map<uint32_t, common_fight_step_xml_info_t> CommonFightStepXmlMap;

struct common_fight_xml_info_t {
	uint32_t type;
	CommonFightStepXmlMap step_map;
};
typedef std::map<uint32_t, common_fight_xml_info_t> CommonFightXmlMap;

class CommonFightXmlManager {
public:
	CommonFightXmlManager();
	~CommonFightXmlManager();

	int read_from_xml(const char *filename);
	int get_common_fight_unlock_step(uint32_t type, uint32_t lv);

private:
	int load_common_fight_xml_info(xmlNodePtr cur);

private:
	CommonFightXmlMap common_fight_xml_map;
};
//extern CommonFightXmlManager common_fight_xml_mgr;

/********************************************************************************/
/*							CommonFightDropXmlManager Class						*/
/********************************************************************************/
struct common_fight_drop_item_xml_info_t {
	uint32_t item_id;
	uint32_t item_cnt;
	uint32_t prob;
};
struct common_fight_drop_step_xml_info_t {
	uint32_t step;
	common_fight_drop_item_xml_info_t rewards[4];
};
typedef std::map<uint32_t, common_fight_drop_step_xml_info_t> CommonFightDropStepXmlMap;

struct common_fight_drop_xml_info_t {
	uint32_t type;
	CommonFightDropStepXmlMap step_map;
};
typedef std::map<uint32_t, common_fight_drop_xml_info_t> CommonFightDropXmlMap;

class CommonFightDropXmlManager {
public:
	CommonFightDropXmlManager();
	~CommonFightDropXmlManager();

	int read_from_xml(const char *filename);
	const common_fight_drop_step_xml_info_t *get_common_fight_drop_xml_info(uint32_t type, uint32_t step);

private:
	int load_common_fight_drop_xml_info(xmlNodePtr cur);

private:
	CommonFightDropXmlMap common_fight_drop_xml_map;
};
//extern CommonFightDropXmlManager common_fight_drop_xml_mgr;


#endif //COMMON_FIGHT_HPP_
