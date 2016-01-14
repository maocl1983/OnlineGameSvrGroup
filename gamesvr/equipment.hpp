/*
 * =====================================================================================
 *
 *  @file  equipment.hpp 
 *
 *  @brief  处理装备相关信息
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef EQUIPMENT_HPP_
#define EQUIPMENT_HPP_

#include "common_def.hpp"

class Player;
class equip_xml_info_t;
class db_get_player_items_info_out;
class cli_get_items_info_out;
class cli_equip_info_t;
class cli_item_info_t;

/********************************************************************************/
/*							Equipment Class										*/
/********************************************************************************/

/* @brief 装备信息类
 * */
class Equipment {
private:
	Player* owner;
public:
	uint32_t id;			/*! 装备ID */
	uint32_t hero_id;		/*! 英雄ID */
	uint32_t get_tm;		/*! 获取时间 */	
	uint32_t lv;			/*! 强化等级 */	
	uint32_t exp;			/*! 强化经验 */	
	uint32_t refining_lv;	/*! 装备精炼等级 */	
	uint32_t gem[3];		/*! 镶嵌的宝石ID */
	uint32_t gem_stat;		/*! 宝石孔解锁状态 */

	const equip_xml_info_t* base_info;	/*! 基本配置信息*/


public:
	Equipment(Player *p, uint32_t equip_id);
	~Equipment();

	int calc_equip_max_lv();
	int add_exp(uint32_t add_value);
	int get_level_up_exp(uint32_t lv);
	int get_max_level_up_exp();
	int calc_equip_total_exp();
   	int	strength_equipment(std::vector<cli_item_info_t>& irons, std::vector<uint32_t>& equips);
	int refining_equipment();
	int inlaid_gem(uint32_t gem_id, uint32_t pos);
	int put_off_gem(uint32_t pos);
	int unlock_gem_hole(uint32_t pos);
	bool check_is_have_same_type_gem(uint32_t type);

	int calc_equip_gem_attr(hero_attr_info_t &info);
};

/********************************************************************************/
/*							EquipmentManager Class								*/
/********************************************************************************/
typedef std::map<uint32_t, Equipment*> EquipmentMap;
typedef std::map<uint32_t, std::vector<Equipment*> > HeroEquipmentMap;

/* @brief 背包装备信息类
 */
class EquipmentManager {
public:
	EquipmentManager(Player *p);
	~EquipmentManager();

	int init_equips_info(db_get_player_items_info_out *p_out);
	int init_heros_equip_info();
	int add_one_equip(uint32_t equip_id);
    int del_equip(uint32_t get_tm);
	int compound_equipment(uint32_t equip_id, std::vector<uint32_t> &pieces);

	bool check_is_valid_gem(uint32_t gem_id);
	int compound_gem(uint32_t gem_id, uint32_t gem_cnt, uint32_t &new_gem_id, uint32_t &new_gem_cnt);

	Equipment* get_equip(uint32_t get_tm);

	int pack_all_equips_info(std::vector<cli_equip_info_t> &equip_vec); 

private:
	Player *owner;
	EquipmentMap equip_map;
	HeroEquipmentMap hero_equip_map;
};
 
/********************************************************************************/
/*						EquipmentXmlManager Class								*/
/********************************************************************************/

struct equip_xml_info_t {
	uint32_t id;				/*! 装备ID */
	uint32_t rank;				/*! 品阶 */
	uint32_t equip_type;		/*! 装备类型 */
	uint32_t class_type;		/*! 职业分类 */
	uint32_t class_effect;		/*! 职业加成 */
	uint32_t suit_id;			/*! 所属套装id */
	uint32_t suit_effect;		/*! 套装加成 */
	hero_attr_info_t attr;		/*! 基础属性 */
	hero_attr_info_t up_attr;	/*! 每级成长属性 */
};
typedef std::map<uint32_t, equip_xml_info_t> EquipXmlMap;

/* @brief 装备配置管理类
 */
class EquipmentXmlManager {
public:
	EquipmentXmlManager();
	~EquipmentXmlManager();

	int read_from_xml(const char* filename);

	const equip_xml_info_t* get_equip_xml_info(uint32_t equip_id);

private:
	int load_equip_xml_info(xmlNodePtr cur);

private:
	EquipXmlMap equip_xml_map;

};
//extern EquipmentXmlManager equip_xml_mgr;

/********************************************************************************/
/*					EquipmentRefiningXmlManager Class							*/
/********************************************************************************/

struct equip_refining_sub_xml_info_t {
	uint32_t lv;
	double maxhp;
	double ad;
	double armor;
	double resist;
	double extra_maxhp;
	double extra_ad;
	double extra_armor;
	double extra_resist;
	uint32_t stone_num;
	uint32_t cost;
};
typedef std::map<uint32_t, equip_refining_sub_xml_info_t> EquipRefiningSubXmlMap;

struct equip_refining_xml_info_t {
	uint32_t equip_id;
	EquipRefiningSubXmlMap level_map;
};
typedef std::map<uint32_t, equip_refining_xml_info_t> EquipRefiningXmlMap;

/* @brief 装备精炼配置表
 */
class EquipRefiningXmlManager {
public:
	EquipRefiningXmlManager();
	~EquipRefiningXmlManager();

	int read_from_xml(const char *filename);
	const equip_refining_sub_xml_info_t* get_equip_refining_xml_info(uint32_t equip_id, uint32_t refining_lv);

private:
	int load_equip_refining_xml_info(xmlNodePtr cur);

private:
	EquipRefiningXmlMap equip_refining_xml_map;
};
//extern EquipRefiningXmlManager equip_refining_xml_mgr;

/********************************************************************************/
/*						EquipCompoundXmlManager Class							*/
/********************************************************************************/

struct equip_compound_xml_info_t {
	uint32_t id;
	uint32_t piece[8];
};
typedef std::map<uint32_t, equip_compound_xml_info_t> EquipCompoundXmlMap;

class EquipCompoundXmlManager {
public:
	EquipCompoundXmlManager();
	~EquipCompoundXmlManager();

	int read_from_xml(const char *filename);
	const equip_compound_xml_info_t *get_equip_compound_xml_info(uint32_t id);

private:
	int load_equip_compound_xml_info(xmlNodePtr cur);

private:
	EquipCompoundXmlMap equip_compound_xml_map;
};
//extern EquipCompoundXmlManager equip_compound_xml_mgr;

/********************************************************************************/
/*						EquipLevelXmlManager Class								*/
/********************************************************************************/
struct equip_level_xml_info_t {
	uint32_t lv;
	uint32_t rank_exp[5];
};
typedef std::map<uint32_t, equip_level_xml_info_t> EquipLevelXmlMap;

class EquipLevelXmlManager {
public:
	EquipLevelXmlManager();
	~EquipLevelXmlManager();

	int read_from_xml(const char *filename);
	const equip_level_xml_info_t *get_equip_level_xml_info(uint32_t equip_lv);

private:
	int load_equip_level_xml_info(xmlNodePtr cur);

private:
	EquipLevelXmlMap equip_level_xml_map;
};
//extern EquipLevelXmlManager equip_level_xml_mgr;

#endif //EQUIPMENT_HPP_
