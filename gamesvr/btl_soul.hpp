/*
 * =====================================================================================
 *
 *  @file  btl_soul.hpp 
 *
 *  @brief  战魂系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef BTL_SOUL_HPP_
#define BTL_SOUL_HPP_

#include "common_def.hpp"

class Player;
class btl_soul_xml_info_t;
class db_get_player_btl_soul_list_out;
class cli_get_btl_soul_list_out;
class cli_divine_request_out;
class cli_divine_request_one_key_out;
class cli_add_btl_soul_to_bag_out;
class cli_swallow_btl_soul_from_divine_bag_out;

/* @brief 战魂种类
 */
enum btl_soul_effect_t {
	em_btl_soul_effect_start		= 0,
	em_btl_soul_effect_ad 			= 1,	/*! 攻击 */
	em_btl_soul_effect_hit			= 2,	/*! 命中 */
	em_btl_soul_effect_miss			= 3,	/*! 闪避 */
	em_btl_soul_effect_cri			= 4,	/*! 暴击 */
	em_btl_soul_effect_ren			= 5,	/*! 韧性 */
	em_btl_soul_effect_maxhp		= 6,	/*! 血量 */
	em_btl_soul_effect_cri_damage	= 7,	/*! 暴伤 */
	em_btl_soul_effect_cri_avoid	= 8,	/*! 暴免 */
	em_btl_soul_effect_final_damage	= 9,	/*! 最终伤害 */
	em_btl_soul_effect_final_avoid	= 10,	/*! 最终免伤 */
	em_btl_soul_effect_armor		= 11,	/*! 护甲 */
	em_btl_soul_effect_resist		= 12,	/*! 魔抗 */
	em_btl_soul_effect_armor_chuan	= 13,	/*! 穿甲 */
	em_btl_soul_effect_resist_chuan	= 14,	/*! 穿魔 */

	em_btl_soul_effect_end	
};

class BtlSoul {
private:
	Player *owner;
public:
	uint32_t id;
	uint32_t get_tm;
	uint32_t lv;
	uint32_t exp;
	uint32_t hero_id;
	uint32_t tmp;

	const btl_soul_xml_info_t *base_info;
public:
	BtlSoul(Player *p, uint32_t id);
	~BtlSoul();

	int add_exp(uint32_t add_value);

};

/********************************************************************************/
/*							BtlSoulManager Class								*/
/********************************************************************************/
typedef std::map<uint32_t, BtlSoul*> BtlSoulMap;
typedef std::map<uint32_t, std::vector<BtlSoul*> > HeroBtlSoulMap;
/* @brief 战魂管理类
 */
class BtlSoulManager {
public:
	BtlSoulManager(Player *p);
	~BtlSoulManager();

	int init_btl_soul_list(db_get_player_btl_soul_list_out *p_out);
	int init_hero_btl_soul_list();
	BtlSoul* get_btl_soul(uint32_t get_tm);
	int btl_soul_level_up(uint32_t get_tm);
	int get_divine_btl_soul_cnt();

	int divine_exec(uint32_t divine_id, uint32_t &btl_soul_id);
	int divine_request(uint32_t divine_id, cli_divine_request_out &out);
	int divine_request_one_key(uint32_t divine_id, cli_divine_request_one_key_out &out);
	int add_btl_soul_from_tmp_to_bag(uint32_t get_tm, cli_add_btl_soul_to_bag_out &out);
	int add_btl_soul_from_tmp_to_bag_one_key();
	int swallow_btl_soul_from_divine_bag(std::vector<uint32_t> &swallow_list, cli_swallow_btl_soul_from_divine_bag_out &out);
	BtlSoul* add_btl_soul(uint32_t id, uint32_t tmp=0);
	int add_btl_soul(std::vector<uint32_t> &btl_soul_vec);
	int del_btl_soul(uint32_t get_tm);
	int swallow_btl_soul(std::vector<uint32_t> &swallow_list);
	bool check_is_valid_btl_soul(uint32_t id);

	void pack_all_btl_soul_info(cli_get_btl_soul_list_out &out);

private:
	Player *owner;
	BtlSoulMap btl_soul_map;
	HeroBtlSoulMap hero_btl_soul_map;
	BtlSoulMap divine_btl_soul_map;
};


/********************************************************************************/
/*							BtlSoulXmlManager Class								*/
/********************************************************************************/
struct btl_soul_xml_info_t {
	uint32_t id;
	uint32_t type;
	uint32_t rank;
	uint32_t effect;
	uint32_t exp;
};
typedef std::map<uint32_t, btl_soul_xml_info_t> BtlSoulXmlMap;

class BtlSoulXmlManager {
public:
	BtlSoulXmlManager();
	~BtlSoulXmlManager();

	int read_from_xml(const char *filename);
	const btl_soul_xml_info_t * get_btl_soul_xml_info(uint32_t id);

private:
	int load_btl_soul_xml_info(xmlNodePtr cur);

private:
	BtlSoulXmlMap btl_soul_xml_map;
};
//extern BtlSoulXmlManager btl_soul_xml_mgr;

/********************************************************************************/
/*							BtlSoulLevelXmlManager Class						*/
/********************************************************************************/
struct btl_soul_level_xml_level_info_t {
	uint32_t lv;
	uint32_t exp;
};
typedef std::map<uint32_t, btl_soul_level_xml_level_info_t> BtlSoulLevelXmlLevelMap;

struct btl_soul_level_xml_info_t {
	uint32_t rank;
	BtlSoulLevelXmlLevelMap level_map;
};
typedef std::map<uint32_t, btl_soul_level_xml_info_t> BtlSoulLevelXmlMap;

class BtlSoulLevelXmlManager {
public:
	BtlSoulLevelXmlManager();
	~BtlSoulLevelXmlManager();

	int read_from_xml(const char *filename);
	int get_btl_soul_levelup_exp(uint32_t rank, uint32_t lv);

private:
	int load_btl_soul_level_xml_info(xmlNodePtr cur);

private:
	BtlSoulLevelXmlMap rank_map;
};
//extern BtlSoulLevelXmlManager btl_soul_level_xml_mgr;

/********************************************************************************/
/*							DivineItemXmlManager Class							*/
/********************************************************************************/

struct divine_item_detail_xml_info_t {
	uint32_t item_id;
	uint32_t prob;
};
typedef std::vector<divine_item_detail_xml_info_t> DivineItemDetailXmlVec;

struct divine_item_xml_info_t {
	uint32_t divine_id;
	uint32_t total_prob;
	DivineItemDetailXmlVec items;
};
typedef std::map<uint32_t, divine_item_xml_info_t> DivineItemXmlMap;

class DivineItemXmlManager {
public:
	DivineItemXmlManager();
	~DivineItemXmlManager();

	int read_from_xml(const char *filename);
	const divine_item_xml_info_t * get_divine_item_xml_info(uint32_t divine_id);
	int random_divine_item(uint32_t divine_id);

private:
	int load_divine_item_xml_info(xmlNodePtr cur);

private:
	DivineItemXmlMap divine_item_xml_map;
};
//extern DivineItemXmlManager divine_item_xml_mgr;

#endif // BTL_SOUL_HPP_
