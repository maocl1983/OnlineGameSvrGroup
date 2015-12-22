/*
 * =====================================================================================
 *
 *  @file  horse.hpp 
 *
 *  @brief  战马系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#ifndef HORSE_HPP_
#define HORSE_HPP_

#include "common_def.hpp"

class Player;
class db_get_player_items_info_out;
class horse_attr_xml_info_t;
class cli_equip_info_t;
class cli_item_info_t;
class cli_horse_attr_info_t;
class cli_get_horse_info_out;
class horse_equip_xml_info_t;

class HorseEquip {
public:
	HorseEquip(Player *p, uint32_t equip_id);
	~HorseEquip();

private:
	Player *owner;
public:
	uint32_t id;
	uint32_t get_tm;
	uint32_t state;
	const horse_equip_xml_info_t *base_info;

};
typedef std::map<uint32_t, HorseEquip*> HorseEquipMap;

class HorseEquipManager {
public:
	HorseEquipManager(Player *p);
	~HorseEquipManager();

	int init_horse_equips_info(db_get_player_items_info_out *p_out);
	HorseEquip* get_horse_equip(uint32_t get_tm);
	HorseEquip* add_one_horse_equip(uint32_t id);
	
	void pack_all_horse_equips_info(std::vector<cli_equip_info_t> &equip_vec);

private:
	Player *owner;
	HorseEquipMap horse_equips;
};

class Horse {
public:
	uint32_t lv;
	uint32_t exp;

	uint32_t max_hp;
	uint32_t ad;
	uint32_t armor;
	uint32_t resist;
	uint32_t ad_cri;
	uint32_t ad_ren;
	uint32_t cri_damage;
	uint32_t cri_avoid;
	uint32_t hit;
	uint32_t miss;
	uint32_t out_max_hp;
	uint32_t out_ad;
	uint32_t out_armor;
	uint32_t out_resist;
	uint32_t soldier_max_hp;
	uint32_t soldier_ad;

	const horse_attr_xml_info_t *base_info;
	HorseEquipMap horse_equips;

private:
	Player *owner;

public:
	Horse(Player *p);
	~Horse();

	int init_horse_info_from_db(uint32_t horse_lv, uint32_t horse_exp);
	HorseEquip* get_horse_equip(uint32_t get_tm);
	bool check_is_can_add_exp();
	int get_levelup_exp(uint32_t lv);
	int add_exp(uint32_t add_value, bool breakthrough=false);
	int horse_train(uint32_t type, uint32_t &cri_flag);
	int horse_use_exp_items(std::vector<cli_item_info_t> &items);
	int put_on_equip(uint32_t get_tm);
	int put_off_equip(uint32_t get_tm);
	bool check_horse_is_breakthrough();
	int horse_breakthrough();

	int send_horse_attr_change_noti();

	int calc_base_attr();
	int calc_equip_attr();
	int calc_all();

	void pack_horse_attr_info(cli_horse_attr_info_t &attr_info);
	void pack_horse_equips_info(std::vector<cli_equip_info_t> &equip_vec);
	void pack_horse_info(cli_get_horse_info_out &out);
};

/********************************************************************************/
/*							HorseAttrXmlManager									*/
/********************************************************************************/
struct horse_attr_xml_info_t {
	uint32_t lv;
	double max_hp;
	double ad;
	double armor;
	double resist;
	double out_max_hp;
	double out_ad;
	double out_armor;
	double out_resist;
	uint32_t soldier_max_hp;
	uint32_t soldier_ad;
};
typedef std::map<uint32_t, horse_attr_xml_info_t> HorseAttrXmlMap;

class HorseAttrXmlManager {
public:
	HorseAttrXmlManager();
	~HorseAttrXmlManager();

	int read_from_xml(const char *filename);
	const horse_attr_xml_info_t *get_horse_attr_xml_info(uint32_t lv);

private:
	int load_horse_attr_xml_info(xmlNodePtr cur);

private:
	HorseAttrXmlMap horse_map;
};
extern HorseAttrXmlManager horse_attr_xml_mgr;

/********************************************************************************/
/*							HorseExpXmlManager									*/
/********************************************************************************/
struct horse_exp_xml_info_t {
	uint32_t lv;
	uint32_t golds_cost;
	uint32_t golds_exp;
	uint32_t golds_crit2_prob;
	uint32_t diamond_cost;
	uint32_t diamond_exp;
	uint32_t diamond_crit2_prob;
	uint32_t diamond_crit10_prob;
	uint32_t exp;
};
typedef std::map<uint32_t, horse_exp_xml_info_t> HorseExpXmlMap;

class HorseExpXmlManager {
public:
	HorseExpXmlManager();
	~HorseExpXmlManager();

	int read_from_xml(const char *filename);
	const horse_exp_xml_info_t* get_horse_exp_xml_info(uint32_t lv);

private:
	int load_horse_exp_xml_info(xmlNodePtr cur);

private:
	HorseExpXmlMap horse_exp_map;
};
extern HorseExpXmlManager horse_exp_xml_mgr;

/********************************************************************************/
/*							HorseEquipXmlManager								*/
/********************************************************************************/
struct horse_equip_xml_info_t {
	uint32_t id;
	uint32_t max_hp;
	uint32_t ad;
	uint32_t armor;
	uint32_t resist;
	uint32_t ad_cri;
	uint32_t ad_ren;
	uint32_t cri_damage;
	uint32_t cri_avoid;
	uint32_t hit;
	uint32_t miss;
};
typedef std::map<uint32_t, horse_equip_xml_info_t> HorseEquipXmlMap;

class HorseEquipXmlManager {
public:
	HorseEquipXmlManager();
	~HorseEquipXmlManager();

	int read_from_xml(const char *filename);
	const horse_equip_xml_info_t* get_horse_equip_xml_info(uint32_t id);

private:
	int load_horse_equip_xml_info(xmlNodePtr cur);

private:
	HorseEquipXmlMap horse_equip_map;
};
extern HorseEquipXmlManager horse_equip_xml_mgr;

#endif //HORSE_HPP_
