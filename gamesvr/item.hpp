/*
 * =====================================================================================
 *
 *  @file  item.hpp
 *
 *  @brief  处理物品相关的信息
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 * copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#ifndef ITEM_HPP_
#define ITEM_HPP_

#include "common_def.hpp"

class Player;
class cli_item_info_t;
class db_get_player_items_info_out;
class cli_get_items_info_out;

/* @brief 特殊道具ID
 */
enum special_item_id_t {
	em_item_start				= 0,
	em_item_golds				= 1,	/*! 金币 */
	em_item_diamond 			= 2,	/*! 钻石 */
	em_item_skill_point 		= 3,	/*! 技能点 */
	em_item_soldier_train_point = 4,	/*! 兵种训练点 */

	em_item_end
};

/********************************************************************************/
/*								ItemsManager Class								*/
/********************************************************************************/

typedef std::map<uint32_t, item_info_t> ItemsMap;

class ItemsManager {
public:
	ItemsManager(Player *owner);
	~ItemsManager();

	int init_items_info(db_get_player_items_info_out *p_out);
	uint32_t get_item_cnt(uint32_t id);
	int get_item_max_cnt(uint32_t item_id);
	int add_items(uint32_t id, uint32_t add_cnt);
	int del_items(uint32_t id, uint32_t del_cnt);
	int add_item_without_callback(uint32_t id, uint32_t add_cnt);
	int del_item_without_callback(uint32_t id, uint32_t del_cnt);
	bool check_is_valid_item(uint32_t item_id);
	int add_reward(uint32_t item_id, uint32_t item_cnt);
	int compound_item_piece(uint32_t piece_id, uint32_t &item_id);

	int open_treasure_box(uint32_t box_id, uint32_t cnt);
	int open_random_gift(uint32_t item_id, uint32_t cnt);
	int sell_item(uint32_t item_id, uint32_t cnt);

	int pack_all_items_info(cli_get_items_info_out &out);
	void pack_give_items_info(std::vector<cli_item_info_t> &item_vec, uint32_t item_id, uint32_t item_cnt);
private:
	Player* owner;
	ItemsMap items;
};

/********************************************************************************/
/*							ItemsXmlManager Class								*/
/********************************************************************************/
enum item_type_t {
	em_item_type_min 		= 0,

	em_item_type_for_hero_exp		= 1,	/*! 英雄经验 */
	em_item_type_for_equip_exp		= 2,	/*! 装备经验 */
	em_item_type_for_equip_refining	= 3,	/*! 装备精炼 */

	em_item_type_for_hero_maxhp		= 4,    /*! 血量 */
	em_item_type_for_hero_ad		= 5,    /*! 攻击 */
	em_item_type_for_hero_armor		= 6,    /*! 护甲 */
	em_item_type_for_hero_resist	= 7,    /*! 魔抗 */

	em_item_type_for_soldier_card	= 10,	/*! 小兵卡 */
	em_item_type_for_hero_card		= 11,	/*! 英雄卡牌 */
	em_item_type_for_soldier_soul	= 12,	/*! 小兵灵魂碎片 */
	em_item_type_for_hero_honor		= 13,	/*! 英雄战功 */
	em_item_type_for_horse_equip	= 14,	/*! 战马装备 */
	em_item_type_for_avoid_fight	= 15,	/*! 免战牌 */
	em_item_type_for_adventure		= 16,	/*! 奇遇令 */
	em_item_type_for_energy			= 17,	/*! 体力丹 */
	em_item_type_for_endurance		= 18,	/*! 耐力丹 */
	em_item_type_for_golds			= 19,	/*! 金块 */
	em_item_type_for_horse_exp		= 20,	/*! 战马经验 */
	em_item_type_for_soldier_exp	= 21,	/*! 小兵经验 */
	em_item_type_for_battle			= 22,	/*! 战斗道具 */
	em_item_type_for_keychain		= 23,	/*! 钥匙 */
	em_item_type_for_treasure_box	= 24,	/*! 宝箱 */
	em_item_type_for_refresh_shop	= 25,	/*! 商店刷新 */
	em_item_type_for_random_gift	= 26,	/*! 随机礼包 */

	em_item_type_max
};

struct item_xml_info_t {
	uint32_t id;
	uint32_t type;
	uint32_t rank;
	uint32_t effect;
	uint32_t relation_id;
	uint32_t max;
	uint32_t price;
};
typedef std::map<uint32_t, item_xml_info_t> ItemXmlMap;

class ItemsXmlManager {
public:
	ItemsXmlManager();
	~ItemsXmlManager();

	int read_from_xml(const char *filename);
	const item_xml_info_t* get_item_xml_info(uint32_t item_id);

private:
	int load_items_xml_info(xmlNodePtr cur);

private:
	ItemXmlMap items;
};
extern ItemsXmlManager items_xml_mgr;

/********************************************************************************/
/*							HeroRankItemXmlManager Class						*/
/********************************************************************************/
struct hero_rank_item_xml_info_t {
	uint32_t item_id;
	uint32_t item_rank;
	uint32_t price;
	uint32_t raw[4][2];
};
typedef std::map<uint32_t, hero_rank_item_xml_info_t> HeroRankItemXmlMap;

/* @brief 英雄进阶道具配置类
 */
class HeroRankItemXmlManager {
public:
	HeroRankItemXmlManager();
	~HeroRankItemXmlManager();

	int read_from_xml(const char *filename);
	const hero_rank_item_xml_info_t *get_hero_rank_item_xml_info(uint32_t item_id);

private:
	int load_hero_rank_item_xml_info(xmlNodePtr cur);

private:
	HeroRankItemXmlMap hero_rank_item_xml_map;
};
extern HeroRankItemXmlManager hero_rank_item_xml_mgr;

/********************************************************************************/
/*							ItemPieceXmlManager Class							*/
/********************************************************************************/
struct item_piece_xml_info_t {
	uint32_t item_id;
	uint32_t item_rank;
	uint32_t need_num;
	uint32_t comp_fee;
	uint32_t relation_id;
	uint32_t type;
};
typedef std::map<uint32_t, item_piece_xml_info_t> ItemPieceXmlMap;

class ItemPieceXmlManager {
public:
	ItemPieceXmlManager();
	~ItemPieceXmlManager();

	int read_from_xml(const char *filename);
	const item_piece_xml_info_t *get_item_piece_xml_info(uint32_t item_id);

private:
	int load_item_piece_xml_info(xmlNodePtr cur);

private:
	ItemPieceXmlMap item_piece_xml_map;
};
extern ItemPieceXmlManager item_piece_xml_mgr;

/********************************************************************************/
/*							RandomItemXmlManager Class							*/
/********************************************************************************/
struct random_item_item_xml_info_t {
	uint32_t item_id;
	uint32_t item_cnt;
	uint32_t prob;
};
typedef std::vector<random_item_item_xml_info_t> TreasureBoxItemXmlVec;
struct random_item_xml_info_t {
	uint32_t box_id;
	uint32_t total_prob;
	TreasureBoxItemXmlVec items;
};
typedef std::map<uint32_t, random_item_xml_info_t> TreasureBoxXmlMap;

class RandomItemXmlManager {
public:
	RandomItemXmlManager();
	~RandomItemXmlManager();

	int read_from_xml(const char *filename);
	const random_item_item_xml_info_t* random_one_item(uint32_t box_id);

private:
	int load_random_item_xml_info(xmlNodePtr cur);

private:
	TreasureBoxXmlMap random_item_xml_map;
};
extern RandomItemXmlManager random_item_xml_mgr;

#endif //ITEM_HPP_
