/*
 * =====================================================================================
 *
 *  @file  internal_affairs.hpp 
 *
 *  @brief  内政系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#ifndef INTERNAL_AFFAIRS_HPP_
#define INTERNAL_AFFAIRS_HPP_

#include "common_def.hpp"
class Player; 
class db_get_affairs_list_out;
class cli_get_internal_affairs_panel_info_out;

/********************************************************************************/
/*								InternalAffairs Class							*/
/********************************************************************************/
struct internal_affairs_info_t {
	uint32_t type;
	uint32_t start_tm;
	uint32_t hero_id;
	uint32_t complete_tm;
};
typedef std::map<uint32_t, internal_affairs_info_t> InternalAffairsMap;

class InternalAffairs {
public:
	InternalAffairs(Player *p);
	~InternalAffairs();

	int init_internal_affairs_list(db_get_affairs_list_out *p_in);
	int init_internal_affairs_exp_info_from_db(uint32_t affairs_lv, uint32_t affairs_exp);

	const internal_affairs_info_t* get_internal_affairs_info(uint32_t type);
	int get_level_up_exp();
	int add_exp(uint32_t add_value);
	int calc_internal_affairs_role_exp();
	int add_internal_affairs(uint32_t type, uint32_t hero_id);
	int set_complete_time(uint32_t type);
	int add_internal_affairs_reward(uint32_t type);

	int join_internal_affairs(uint32_t type, uint32_t hero_id, uint32_t &left_tm);
	int complete_internal_affairs(uint32_t type);
	int get_internal_affairs_role_exp();

	void pack_client_internal_affairs_info(cli_get_internal_affairs_panel_info_out &cli_out);

public:
	uint32_t lv;
	uint32_t exp;

private:
	Player *owner;
	InternalAffairsMap internal_affairs_map;
};


/********************************************************************************/
/*						InternalAffairsXmlManager Class							*/
/********************************************************************************/
struct internal_affairs_xml_info_t {
	uint32_t type;
	uint32_t energy;
	uint32_t time;
	uint32_t exp;
	uint32_t week;
	uint32_t unlock_lv;
};
typedef std::map<uint32_t, internal_affairs_xml_info_t> InternalAffairsXmlMap;

class InternalAffairsXmlManager {
public:
	InternalAffairsXmlManager();
	~InternalAffairsXmlManager();

	int read_from_xml(const char *filename);
	const internal_affairs_xml_info_t * get_internal_affairs_xml_info(uint32_t type);
	int get_cur_week_interal_affairs(std::vector<uint32_t> &vec);

private:
	int load_internal_affairs_xml_info(xmlNodePtr cur);

private:
	InternalAffairsXmlMap internal_affairs_xml_map;
};
//extern InternalAffairsXmlManager internal_affairs_xml_mgr;

/********************************************************************************/
/*						InternalAffairsRewardXmlManager Class					*/
/********************************************************************************/
struct internal_affairs_reward_item_xml_info_t {
	uint32_t item_id;
	uint32_t item_cnt;
	uint32_t weight;
};
typedef std::vector<internal_affairs_reward_item_xml_info_t> InternalAffairsRewardItemVec;

struct internal_affairs_reward_xml_info_t {
	uint32_t type;
	uint32_t total_weight;
	InternalAffairsRewardItemVec rewards;
};
typedef std::map<uint32_t, internal_affairs_reward_xml_info_t> InternalAffairsRewardXmlMap;

class InternalAffairsRewardXmlManager {
public:
	InternalAffairsRewardXmlManager();
	~InternalAffairsRewardXmlManager();

	const internal_affairs_reward_item_xml_info_t* random_one_reward(uint32_t type);
	int read_from_xml(const char *filename);

private:
	int load_internal_affairs_reward_xml_info(xmlNodePtr cur);

private:
	InternalAffairsRewardXmlMap internal_affairs_reward_xml_map;
};
//extern InternalAffairsRewardXmlManager internal_affairs_reward_xml_mgr;

/********************************************************************************/
/*						InternalAffairsLevelXmlManager Class					*/
/********************************************************************************/
struct internal_affairs_level_xml_info_t {
	uint32_t lv;
	uint32_t exp;
	uint32_t cost_golds;
};
typedef std::map<uint32_t, internal_affairs_level_xml_info_t> InternalAffairsLevelXmlMap;

class InternalAffairsLevelXmlManager {
public:
	InternalAffairsLevelXmlManager();
	~InternalAffairsLevelXmlManager();

	int read_from_xml(const char *filename);
	const internal_affairs_level_xml_info_t* get_internal_affairs_level_xml_info(uint32_t lv);

private:
	int load_internal_affairs_level_xml_info(xmlNodePtr cur);

private:
	InternalAffairsLevelXmlMap internal_affairs_level_xml_map;
};
//extern InternalAffairsLevelXmlManager internal_affairs_level_xml_mgr;


#endif //INTERNAL_AFFAIRS_HPP_
