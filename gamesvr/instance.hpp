/*
 * =====================================================================================
 *
 *  @file  intance.hpp 
 *
 *  @brief  处理副本相关逻辑
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#ifndef INSTANCE_HPP_
#define INSTANCE_HPP_

#include "common_def.hpp"

class cli_instance_battle_end_out;
class cli_get_instance_list_out;
class db_instance_info_t;
class db_chapter_info_t;
class cli_get_instance_chapter_list_out;
class cli_send_get_common_bonus_noti_out;
class Player;
class item_info_t;

/********************************************************************************/
/*							Troop Info											*/
/********************************************************************************/
enum troop_type_t {
	em_troop_type_start			= 0,
	em_troop_type_instance		= 1,	/*! 副本 */
	em_troop_type_arena			= 2,	/*! 竞技场 */
	em_troop_type_treasure		= 3,	/*! 夺宝 */
	em_troop_type_trial			= 4,	/*! 试炼 */
	em_troop_type_soldier_btl	= 5,	/*! 兵战 */
	em_troop_type_defend		= 6,	/*! 守城 */

	em_troop_type_end
};

/* @brief 默认阵容信息
 */
struct troop_info_t {
	uint32_t type;
	std::vector<uint32_t> heros;
	std::vector<uint32_t> soldiers;
};
typedef std::map<uint32_t, troop_info_t> TroopMap;

/********************************************************************************/
/*							InstanceManager Class								*/
/********************************************************************************/

struct instance_cache_info_t {
	uint32_t instance_id;			/*! 副本ID */
	uint32_t daily_tms;				/*! 今天已完成次数 */
};
typedef std::map<uint32_t, instance_cache_info_t> InstanceCacheMap;

struct chapter_info_t {
	uint32_t chapter_id;
	uint32_t star;
	uint32_t bag_stat;
};
typedef std::map<uint32_t, chapter_info_t> ChapterMap;

class InstanceManager {
public:
	InstanceManager(Player *p);
	~InstanceManager();

	void set_cur_instance_id(uint32_t instance_id) {cur_instance_id = instance_id;}
	int get_cur_instance_id() {return cur_instance_id;}
	int set_hero(uint32_t hero_id, uint32_t status);
	int set_soldier(uint32_t soldier_id, uint32_t status);
	int clear_heros();
	int clear_soldiers();

	int init_chapter_list(std::vector<db_chapter_info_t> &chapter_list);
	int get_chapter_id(uint32_t instance_id);
	const chapter_info_t* get_chapter_info(uint32_t chapter_id);
	int update_instance_chapter_info(uint32_t instance_id);
	int get_chapter_bag_reward(uint32_t chapter_id, uint32_t bag_id);
	int set_chapter_bag_stat(uint32_t chapter_id, uint32_t bag_stat);
	bool check_chapter_is_completed(uint32_t chapter_id);
	int get_instance_battle_left_tms(uint32_t instance_id);

	int battle_request(uint32_t instance_id);
	int battle_end(uint32_t instance_id, uint32_t time, uint32_t win, uint32_t lose, uint32_t kill_hero_id, cli_instance_battle_end_out &cli_out);

	int init_instance_list(std::vector<db_instance_info_t> &instance_list);
	instance_cache_info_t* get_instance_cache_info(uint32_t instance_id);
	int check_instance_battle(uint32_t instance_id);
	bool check_instance_battle_is_win(uint32_t instance_id, uint32_t time, uint32_t win, uint32_t lose);
	int update_instance_db_info(uint32_t instance_id);
	
	int gen_first_rewards(uint32_t instance_id);
	int gen_random_rewards(uint32_t instance_id);
	int give_instance_battle_role_reward(uint32_t instance_id, cli_instance_battle_end_out& cli_out);
	int give_instance_battle_heros_reward(uint32_t instance_id, uint32_t kill_hero_id, cli_instance_battle_end_out& cli_out);
	int give_instance_battle_soldiers_reward(uint32_t instance_id, cli_instance_battle_end_out& cli_out);
	int give_instance_battle_first_reward(uint32_t instance_id, cli_instance_battle_end_out& cli_out);
	int give_instance_battle_random_reward(uint32_t drop_id, cli_instance_battle_end_out& cli_out);
	int give_instance_battle_reward(uint32_t instance_id, uint32_t kill_hero_id, cli_instance_battle_end_out& cli_out);

	int instance_clean_up(uint32_t instance_id); 
	int send_instance_clean_up_reward(uint32_t instance_id);

	void pack_instance_list_info(cli_get_instance_list_out &cli_out);
	void pack_instance_lose_info(cli_instance_battle_end_out& cli_out);
	void pack_client_instance_chapter_list(cli_get_instance_chapter_list_out &cli_out);
	void pack_instance_rewards_info(std::vector<cli_item_info_t> &items);

private:
	Player *owner;
	uint32_t cur_instance_id;
	InstanceCacheMap cache_map;
	ChapterMap chapter_map;
	std::set<uint32_t> heros;//参战英雄ID
	std::set<uint32_t> soldiers;//参战小兵ID
	std::vector<item_info_t> first_rewards;	//首次奖励物品信息
	std::vector<item_info_t> random_rewards;//随机奖励物品信息
};



/********************************************************************************/
/*						InstanceXmlManager Class								*/
/********************************************************************************/
struct instance_xml_info_t {
	uint32_t id;			/*! 副本ID */
	uint32_t pre_instance;	/*! 前置副本 */
	uint32_t need_lv;		/*! 所需等级 */
	uint32_t daily_res;		/*! 每日限制 */
	uint32_t limit_time;	/*! 限制时间 */
	uint32_t lose;			/*! 失败条件 */
	uint32_t win;			/*! 胜利条件 */
	uint32_t enemy_power;	/*! 敌方战斗力 */
	uint32_t drop_id;		/*! 掉落ID */
	uint32_t energy;		/*! 消耗体力 */
};
typedef std::map<uint32_t, instance_xml_info_t> InstanceXmlMap;

class InstanceXmlManager {
public:
	InstanceXmlManager();
	~InstanceXmlManager();

	int read_from_xml(const char *filename);
	const instance_xml_info_t *get_instance_xml_info(uint32_t instance_id);
	uint32_t get_chapter_last_instance_id(uint32_t chapter_id);

private:
	int load_instance_xml_info(xmlNodePtr cur);

private:
	InstanceXmlMap instance_xml_map;
};
//extern InstanceXmlManager instance_xml_mgr;

/********************************************************************************/
/*							TroopXmlManager Class								*/
/********************************************************************************/

struct troop_monster_xml_info_t {
	uint32_t pos;
	uint32_t monster_id;
};
typedef std::map<uint32_t, troop_monster_xml_info_t> TroopMonsterXmlMap;

struct troop_xml_info_t {
	uint32_t id;
	TroopMonsterXmlMap monster_map;
};
typedef std::map<uint32_t, troop_xml_info_t> TroopXmlMap;

class TroopXmlManager {
public:
	TroopXmlManager();
	~TroopXmlManager();

	int read_from_xml(const char *filename);

	const troop_xml_info_t* get_troop_xml_info(uint32_t troop_id);
	uint32_t get_troop_monster_id(uint32_t troop_id, uint32_t pos);

private:
	int load_troop_xml_info(xmlNodePtr cur);
	int load_troop_monster_xml_info(xmlNodePtr cur, TroopMonsterXmlMap &monster_map);

private:
	TroopXmlMap troop_xml_map;
};
//extern TroopXmlManager troop_xml_mgr;

/********************************************************************************/
/*							MonsterXmlManager Class								*/
/********************************************************************************/

struct monster_xml_info_t {
	uint32_t id; 
	uint32_t camp;              /*! 阵营 */
	uint32_t sex;               /*! 性别 */
	uint32_t army;              /*! 兵种 */
	uint32_t rank;              /*! 初始品阶 */
	uint32_t str_growth;        /*! 力量成长 */
	uint32_t int_growth;        /*! 智力成长 */
	uint32_t agi_growth;        /*! 敏捷成长 */
	uint32_t range;             /*! 射程 */
	uint32_t atk_type;          /*! 攻击类型 */
	uint32_t def_type;          /*! 护甲类型 */
	hero_attr_info_t attr;      /*! 英雄基础属性 */
	std::vector<uint32_t> skills;   /*! 英雄技能 */
	std::vector<uint32_t> talents;  /*! 英雄天赋 */
};

typedef std::map<uint32_t, monster_xml_info_t> MonsterXmlMap;

class MonsterXmlManager {
public:
	MonsterXmlManager();
	~MonsterXmlManager();

	int read_from_xml(const char *filename);

	const monster_xml_info_t* get_monster_xml_info(uint32_t monster_id);

private:
	int load_monster_xml_info(xmlNodePtr cur);

private:
	MonsterXmlMap monster_xml_map;
};
//extern MonsterXmlManager monster_xml_mgr;

/********************************************************************************/
/*							InstanceDropXmlManager Class						*/
/********************************************************************************/

struct instance_drop_item_info_t {
	uint32_t item_id;
	uint32_t item_cnt;
};
struct instance_drop_prob_item_info_t {
	uint32_t item_id;
	uint32_t item_cnt;
	uint32_t prob;
};

struct instance_drop_xml_info_t {
	uint32_t id;
	uint32_t golds;
	uint32_t hero_exp;
	uint32_t soldier_exp;
	uint32_t hero_honor;
	uint32_t repeat_num;
	instance_drop_item_info_t first_items[4];
	instance_drop_prob_item_info_t random_items[5];
};
typedef std::map<uint32_t, instance_drop_xml_info_t> InstanceDropXmlMap;

class InstanceDropXmlManager {
public:
	InstanceDropXmlManager();
	~InstanceDropXmlManager();

	int read_from_xml(const char* filename);

	const instance_drop_xml_info_t* get_instance_drop_xml_info(uint32_t drop_id);

private:
	int load_instance_drop_xml_info(xmlNodePtr cur);

private:
	InstanceDropXmlMap instance_drop_xml_map;
};
//extern InstanceDropXmlManager instance_drop_xml_mgr;


/********************************************************************************/
/*							InstanceChapterXmlManager Class						*/
/********************************************************************************/
struct instance_chapter_xml_info_t {
	uint32_t chapter_id;
	uint32_t bag[3];
};
typedef std::map<uint32_t, instance_chapter_xml_info_t> InstanceChapterXmlMap;

class InstanceChapterXmlManager {
public:
	InstanceChapterXmlManager();
	~InstanceChapterXmlManager();

	int read_from_xml(const char *filename);
	const instance_chapter_xml_info_t* get_instance_chapter_xml_info(uint32_t chapter_id);

private:
	int load_instance_chapter_xml_info(xmlNodePtr cur);

private:
	InstanceChapterXmlMap instance_chapter_xml_map;
};
//extern InstanceChapterXmlManager instance_chapter_xml_mgr;


/********************************************************************************/
/*							InstanceBagXmlManager Class							*/
/********************************************************************************/
struct instance_bag_xml_info_t {
	uint32_t id;
	uint32_t star;
	uint32_t golds;
	uint32_t diamond;
	uint32_t items[3][2];
};
typedef std::map<uint32_t, instance_bag_xml_info_t> InstanceBagXmlMap;

class InstanceBagXmlManager {
public:
	InstanceBagXmlManager();
	~InstanceBagXmlManager();

	int read_from_xml(const char *filename);
	const instance_bag_xml_info_t* get_instance_bag_xml_info(uint32_t bag_id);

private:
	int load_instance_bag_xml_info(xmlNodePtr cur);

private:
	InstanceBagXmlMap instance_bag_xml_map;
};
//extern InstanceBagXmlManager instance_bag_xml_mgr;


#endif //INSTANCE_HPP_
