/*
 * =====================================================================================
 *
 *  @file  adventure.hpp 
 *
 *  @brief  奇遇系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#ifndef ADVENTURE_HPP_
#define ADVENTURE_HPP_

#include "common_def.hpp"

class db_get_adventure_list_out;
class cli_get_adventure_list_out;
class Player;
class item_info_t;

/********************************************************************************/
/*						AdventureManager Class									*/
/********************************************************************************/
struct adventure_info_t {
	uint32_t time;
	uint32_t adventure_id;
	uint32_t item_id;
	uint32_t item_cnt;
};
typedef std::map<uint32_t, adventure_info_t> AdventureMap;

class AdventureManager {
public:
	AdventureManager(Player *p);
	~AdventureManager();

	int init_adventure_list(db_get_adventure_list_out *p_in);

	const adventure_info_t* get_adventure(uint32_t time);
	int add_adventure(uint32_t adventure_id);
	int del_adventure(uint32_t time);
	int calc_daily_max_complete_tms();

	int del_expire_adventure();
	int trigger_adventure();
	int complete_adventure(uint32_t time, uint32_t type);

	void pack_client_adventure_list(cli_get_adventure_list_out &out);

private:
	AdventureMap adventure_map;
	Player *owner;
};

/********************************************************************************/
/*						AdventureXmlManager Class								*/
/********************************************************************************/
struct adventure_xml_info_t {
	uint32_t id;
	uint32_t type;
	uint32_t time;
	uint32_t select_1;
	uint32_t select_2;
};
typedef std::map<uint32_t, adventure_xml_info_t> AdventureXmlMap;

class AdventureXmlManager {
public:
	AdventureXmlManager();
	~AdventureXmlManager();

	uint32_t random_one_adventure(uint32_t type);
	const adventure_xml_info_t* get_adventure_xml_info(uint32_t adventure_id);
	int read_from_xml(const char *filename);

private:
	int load_adventure_xml_info(xmlNodePtr cur);

private:
	AdventureXmlMap adventure_xml_map;
	std::vector<uint32_t> adventures[2];
};
//extern AdventureXmlManager adventure_xml_mgr;


/********************************************************************************/
/*						AdventureSelectXmlManager Class							*/
/********************************************************************************/
struct adventure_select_xml_info_t {
	uint32_t select_id;
	uint32_t role_exp;
	uint32_t reward_item_id;
	uint32_t reward_item_cnt;
	uint32_t cost_diamond;
	uint32_t cost_adventure;
};
typedef std::map<uint32_t, adventure_select_xml_info_t> AdventureSelectXmlMap;

class AdventureSelectXmlManager {
public:
	AdventureSelectXmlManager();
	~AdventureSelectXmlManager();

	const adventure_select_xml_info_t* get_adventure_select_xml_info(uint32_t select_id);
	int read_from_xml(const char *filename);

private:
	int load_adventure_select_xml_info(xmlNodePtr cur);

private:
	AdventureSelectXmlMap adventure_select_xml_map;
};
//extern AdventureSelectXmlManager adventure_select_xml_mgr;

/********************************************************************************/
/*						AdventureItemXmlManager Class							*/
/********************************************************************************/
struct adventure_item_xml_info_t {
	uint32_t select_id;
	std::vector<item_info_t> items;
};
typedef std::map<uint32_t, adventure_item_xml_info_t> AdventureItemXmlMap;

class AdventureItemXmlManager {
public:
	AdventureItemXmlManager();
	~AdventureItemXmlManager();

	int read_from_xml(const char *filename);
	const item_info_t *random_one_reward(uint32_t select_id);

private:
	int load_adventure_item_xml_info(xmlNodePtr cur);

private:
	AdventureItemXmlMap adventure_item_xml_map;
};
//extern AdventureItemXmlManager adventure_item_xml_mgr;

#endif //ADVENTURE_HPP_
