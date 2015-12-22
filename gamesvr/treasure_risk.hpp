/*
 * =====================================================================================
 *
 *  @file  treasure_risk.hpp 
 *
 *  @brief  夺宝系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef TREASURE_RISK_HPP_
#define TREASURE_RISK_HPP_

#include "common_def.hpp"

class Player;
class cli_get_treasure_risk_recommend_players_out;
class cli_treasure_risk_winner_draw_out;
class rs_get_treasure_piece_user_list_out;
class cli_treasure_risk_player_info_t;
class db_get_treasure_risk_opp_player_info_out;
class cli_treasure_10_risk_out;

/********************************************************************************/
/*							TreasureManager Class								*/
/********************************************************************************/
typedef std::vector<cli_treasure_risk_player_info_t> TreasureRiskPlayerVec;
class TreasureManager {
public:
	TreasureManager(Player *p);
	~TreasureManager();

	uint32_t get_battle_user_id() {return battle_user;}
	void set_battle_user_id(uint32_t user_id) {battle_user = user_id;}
	uint32_t get_request_piece() {return request_piece;}
	void set_request_piece(uint32_t piece_id) {request_piece = piece_id;}
	int open_protection();

	uint32_t get_request_piece_prob(uint32_t user_id);
	int get_treasure_risk_prob_lv(uint32_t user_id, uint32_t piece_id);
	int get_recommend_players_cnt();
	int give_win_honor(uint32_t kill_hero_id);
	
	int treasure_10_risk(uint32_t request_piece, cli_treasure_10_risk_out &cli_out);
	int battle_request(uint32_t user_id, uint32_t piece_id);
	int battle_end(uint32_t user_id, uint32_t is_win, uint32_t kill_hero_id, uint32_t &piece_id);
	int winner_draw(cli_treasure_risk_winner_draw_out &out);

	int handle_treasure_piece_user_list_redis_return(rs_get_treasure_piece_user_list_out *p_in);

	int pack_recommend_robot(uint32_t piece_id, uint32_t num);
	int pack_recommend_online_player(Player *p);
	int pack_recommend_offline_player(db_get_treasure_risk_opp_player_info_out *p_in);
	void pack_client_recommend_players(cli_get_treasure_risk_recommend_players_out &out);

private:
	Player *owner;
	uint32_t battle_user;
	uint32_t request_piece;
	bool is_win;
	bool reward_stat;
	TreasureRiskPlayerVec recommend_players;
};


/********************************************************************************/
/*							TreasureAttrXmlManager Class						*/
/********************************************************************************/
struct treasure_attr_xml_info_t {
	uint32_t lv;
	uint32_t rank;
	double max_hp;
	double ad;
	double armor;
	double resist;
};
typedef std::map<uint32_t, treasure_attr_xml_info_t> TreasureAttrXmlMap;

class TreasureAttrXmlManager {
public:
	TreasureAttrXmlManager();
	~TreasureAttrXmlManager();

	int read_from_xml(const char *filename);
	const treasure_attr_xml_info_t * get_treasure_attr_xml_info(uint32_t lv);

private:
	int load_treasure_attr_xml_info(xmlNodePtr cur);

private:
	TreasureAttrXmlMap treasure_attr_xml_map;
};
extern TreasureAttrXmlManager treasure_attr_xml_mgr;

/********************************************************************************/
/*							TreasureHeroXmlManager Class						*/
/********************************************************************************/
struct treasure_hero_xml_info_t {
	uint32_t type;
	std::vector<uint32_t> heros;
};
typedef std::map<uint32_t, treasure_hero_xml_info_t> TreasureHeroXmlMap;

class TreasureHeroXmlManager {
public:
	TreasureHeroXmlManager();
	~TreasureHeroXmlManager();

	int read_from_xml(const char *filename);
	uint32_t random_one_hero(uint32_t type);

private:
	int load_treasure_hero_xml_info(xmlNodePtr cur);

private:
	TreasureHeroXmlMap treasure_hero_xml_map;
};
extern TreasureHeroXmlManager treasure_hero_xml_mgr;

/********************************************************************************/
/*							TreasureRewardXmlManager Class						*/
/********************************************************************************/
struct treasure_reward_xml_info_t {
	uint32_t item_id;
	uint32_t num;
	uint32_t prob;
};
typedef std::vector<treasure_reward_xml_info_t> TreasureRewardVec;

class TreasureRewardXmlManager {
public:
	TreasureRewardXmlManager();
	~TreasureRewardXmlManager();

	int read_from_xml(const char *filename);
	const treasure_reward_xml_info_t *random_one_treasure_reward();

private:
	int load_treasure_reward_xml_info(xmlNodePtr cur);

private:
	uint32_t total_prob;
	TreasureRewardVec rewards;
};
extern TreasureRewardXmlManager treasure_reward_xml_mgr;

#endif //TREASURE_RISK_HPP_
