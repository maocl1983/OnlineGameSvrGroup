/*
 * =====================================================================================
 *
 *  @file  arena.hpp 
 *
 *  @brief  竞技场系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef ARENA_HPP_
#define ARENA_HPP_

#include "common_def.hpp"

class Player;
class db_arena_info_t;
class cli_arena_info_t;
class cli_arena_battle_request_out;
class db_get_arena_opp_battle_info_out;
class cli_arena_battle_end_out;
/********************************************************************************/
/*							ArenaManager Class									*/
/********************************************************************************/
struct arena_info_t {
	uint32_t user_id;
	char nick[32];
	uint32_t ranking;
	uint32_t lv;
	uint32_t rank;
	uint32_t hero[3];
	uint32_t soldier[3];
	uint32_t btl_power;
	uint32_t state;
};
typedef std::map<uint32_t, arena_info_t> ArenaMap;

class ArenaManager {
public:
	ArenaManager();
	~ArenaManager();

	int get_arena_count();
	int first_init_arena();
	int init_arena(std::vector<db_arena_info_t> &arena_list);
	bool check_arena_is_init();

	int calc_serv_day();
	int calc_robot_lv(uint32_t lv);
	int calc_robot_btl_power(uint32_t lv, uint32_t btl_power);

	int first_add_to_arena(Player *p);
	int add_to_arena(Player *p, arena_info_t *p_info);
	int update_ranking_info_lv(uint32_t user_id, uint32_t lv);
	int update_ranking_info_rank(uint32_t user_id, uint32_t rank);
	int update_ranking_info_btl_power(Player *p);
	int set_arena_defend_team(Player *p, uint32_t hero1, uint32_t hero2, uint32_t hero3, uint32_t soldier1, uint32_t soldier2, uint32_t soldier3);
	bool is_defend_hero(uint32_t user_id, uint32_t hero_id);
	bool is_defend_soldier(uint32_t user_id, uint32_t soldier_id);

	const arena_info_t *get_arena_ranking_info_by_ranking(uint32_t ranking);
	const arena_info_t *get_arena_ranking_info_by_userid(uint32_t user_id);
	int get_arena_ranking_list(uint32_t start, uint32_t end, std::vector<cli_arena_info_t> &arena_list);
	void set_state(uint32_t user_id, int state);
	int swap_ranking(uint32_t ranking1, uint32_t ranking2);
	int give_ranking_bonus(Player *p, uint32_t ranking);
	int give_win_honor(Player *p, uint32_t kill_hero_id);
	int give_arena_daily_ranking_bonus();

	int battle_request(uint32_t user_id, uint32_t ranking);
	int battle_end(Player *p, uint32_t ranking, uint32_t is_win, uint32_t kill_hero_id, cli_arena_battle_end_out &cli_out);

	int pack_single_arena_info_by_ranking(uint32_t ranking, cli_arena_info_t &info);
	int pack_single_arena_info_by_userid(uint32_t user_id, cli_arena_info_t &info);
	int pack_recommend_players_info(uint32_t ranking, std::vector<cli_arena_info_t> &recommend_playes);
	int pack_arena_robot_battle_info(uint32_t ranking, cli_arena_battle_request_out &cli_out);
	int pack_arena_opp_player_battle_info(uint32_t ranking, cli_arena_battle_request_out &cli_out);
	int pack_arena_opp_battle_info(uint32_t ranking, cli_arena_battle_request_out &cli_out);
	int pack_arena_opp_battle_info_from_db(db_get_arena_opp_battle_info_out *p_in, cli_arena_battle_request_out &cli_out);
	int pack_arena_ranking_db_info(uint32_t ranking, db_arena_info_t &ranking_info);

private:
	bool init_flag;
	ArenaMap arena_map;
	std::map<uint32_t, uint32_t> ranking_map;
};
//extern ArenaManager arena_mgr;


/********************************************************************************/
/*						ArenaAttrXmlManager Class								*/
/********************************************************************************/

struct arena_attr_xml_info_t {
	uint32_t min_ranking;
	uint32_t max_ranking;
	uint32_t lv;
	uint32_t rank;
	double max_hp;
	double ad;
	double armor;
	double resist;
	uint32_t min_btl_power;
	uint32_t max_btl_power;
};
typedef std::map<uint32_t, arena_attr_xml_info_t> ArenaAttrXmlMap;

class ArenaAttrXmlManager {
public:
	ArenaAttrXmlManager();
	~ArenaAttrXmlManager();

	int read_from_xml(const char *filename);
	const arena_attr_xml_info_t* get_arena_attr_xml_info(uint32_t ranking);

private:
	int load_arena_attr_xml_info(xmlNodePtr cur);

private:
	ArenaAttrXmlMap arena_attr_xml_map;
};
//extern ArenaAttrXmlManager arena_attr_xml_mgr;


/********************************************************************************/
/*						ArenaHeroXmlManager Class								*/
/********************************************************************************/
struct arena_hero_xml_info_t {
	uint32_t type;
	std::vector<uint32_t> heros;
};
typedef std::map<uint32_t, arena_hero_xml_info_t> ArenaHeroXmlMap;

class ArenaHeroXmlManager {
public:
	ArenaHeroXmlManager();
	~ArenaHeroXmlManager();

	int read_from_xml(const char *filename);
	uint32_t random_one_hero(uint32_t type);

private:
	int load_arena_hero_xml_info(xmlNodePtr cur);

private:
	ArenaHeroXmlMap arena_hero_xml_map;
};
//extern ArenaHeroXmlManager arena_hero_xml_mgr;

/********************************************************************************/
/*						ArenaBonusXmlManager Class								*/
/********************************************************************************/
struct arena_bonus_xml_info_t {
	uint32_t min_ranking;
	uint32_t max_ranking;
	double ranking_diamond;
	uint32_t daily_diamond;
	uint32_t daily_golds;
};
typedef std::map<uint32_t, arena_bonus_xml_info_t> ArenaBonusXmlMap;

class ArenaBonusXmlManager {
public:
	ArenaBonusXmlManager();
	~ArenaBonusXmlManager();

	int read_from_xml(const char *filename);
	const arena_bonus_xml_info_t * get_arena_bonus_xml_info(uint32_t ranking);
	int calc_arena_bonus_diamond(uint32_t old_ranking, uint32_t new_ranking);

private:
	int load_arena_bonus_xml_info(xmlNodePtr cur);

private:
	ArenaBonusXmlMap arena_bonus_xml_map;
};
//extern ArenaBonusXmlManager arena_bonus_xml_mgr;

/********************************************************************************/
/*						ArenaLevelAttrXmlManager Class							*/
/********************************************************************************/
struct arena_level_attr_xml_info_t {
	uint32_t lv;
	uint32_t max_hp;
	uint32_t ad;
	uint32_t armor;
	uint32_t resist;
};
typedef std::map<uint32_t, arena_level_attr_xml_info_t> ArenaLevelAttrXmlMap;

class ArenaLevelAttrXmlManager {
public:
	ArenaLevelAttrXmlManager();
	~ArenaLevelAttrXmlManager();

	int read_from_xml(const char *filename);
	const arena_level_attr_xml_info_t *get_arena_level_attr_xml_info(uint32_t lv);

private:
	int load_arena_level_attr_xml_info(xmlNodePtr cur);

private:
	ArenaLevelAttrXmlMap arena_level_attr_xml_map;
};
//extern ArenaLevelAttrXmlManager arena_level_attr_xml_mgr;

#endif //ARENA_HPP_

