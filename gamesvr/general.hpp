/*
 * =====================================================================================
 *
 *  @file  general.hpp 
 *
 *  @brief  处理一些杂项逻辑
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef GENERAL_HPP_
#define GENERAL_HPP_

#include "common_def.hpp"
class Player;

/********************************************************************************/
/*                          NickXmlManager Class                                */
/********************************************************************************/
struct nick_xml_info_t {
	uint32_t id; 
	char nick[16];
};
typedef std::map<uint32_t, nick_xml_info_t> NickXmlMap;

class NickXmlManager {
public:
	NickXmlManager();
	~NickXmlManager();

	int read_from_xml(const char *filename);
	const char *random_one_sumname(uint32_t sex);
	const char *random_one_name(uint32_t sex);

private:
	int load_nick_xml_info(xmlNodePtr cur);
	int load_male_nick_xml_info(xmlNodePtr cur);
	int load_female_nick_xml_info(xmlNodePtr cur);
private:
	NickXmlMap male_sumname_xml_map;
	NickXmlMap male_name_xml_map;
	NickXmlMap female_sumname_xml_map;
	NickXmlMap female_name_xml_map;
};
extern NickXmlManager nick_xml_mgr;


/********************************************************************************/
/*                          General	Class 		                               */
/********************************************************************************/
class General {
public:
	General() {}
	~General() {}

	static int get_new_player_checkin_reward(Player *p, uint32_t checkin_id);
	static int get_level_gift(Player *p, uint32_t gift_id);
	static int get_diamond_gift(Player *p, uint32_t gift_id);

};


#endif //GENERAL_HPP_
