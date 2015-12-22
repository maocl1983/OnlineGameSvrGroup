/*
 * =====================================================================================
 *
 *  @file  ten_even_draw.hpp 
 *
 *  @brief  十连抽系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#ifndef TEN_EVEN_DRAW_HPP_
#define TEN_EVEN_DRAW_HPP_

#include "common_def.hpp"

/********************************************************************************/
/*							TenEvenDrawManager Class							*/
/********************************************************************************/

class Player;
class cli_get_ten_even_draw_info_out;
class cli_ten_even_draw_request1_out;
class cli_ten_even_draw_request2_out;

class TenEvenDrawManager {
public:
	TenEvenDrawManager(Player *p);
	~TenEvenDrawManager();

	int ten_even_draw_for_golds(uint32_t type, cli_ten_even_draw_request1_out &out);
	int ten_even_draw_for_diamond(uint32_t type, cli_ten_even_draw_request2_out &out); 

	int calc_left_draw_tms_can_special();

	void pack_ten_even_draw_info(cli_get_ten_even_draw_info_out &out);

public:
	Player *owner;
};

/********************************************************************************/
/*						TenEvenDrawGoldsXmlManager Class						*/
/********************************************************************************/
struct ten_even_draw_golds_xml_info_t {
	uint32_t id;
	uint32_t prob;
};
typedef std::map<uint32_t, ten_even_draw_golds_xml_info_t> TenEvenDrawGoldsXmlMap;

class TenEvenDrawGoldsXmlManager {
public:
	TenEvenDrawGoldsXmlManager();
	~TenEvenDrawGoldsXmlManager();

	int random_one_item();
	int read_from_xml(const char *filename);

private:
	int load_ten_even_draw_golds_xml_info(xmlNodePtr cur);

private:
	uint32_t total_prob;
	TenEvenDrawGoldsXmlMap draw_map;
};
extern TenEvenDrawGoldsXmlManager ten_even_draw_golds_xml_mgr;

/********************************************************************************/
/*					TenEvenDrawDiamondXmlManager Class							*/
/********************************************************************************/
struct ten_even_draw_diamond_xml_info_t {
	uint32_t id;
	uint32_t prob;
};
typedef std::map<uint32_t, ten_even_draw_diamond_xml_info_t> TenEvenDrawDiamondXmlMap;

class TenEvenDrawDiamondXmlManager {
public:
	TenEvenDrawDiamondXmlManager();
	~TenEvenDrawDiamondXmlManager();

	int random_one_item();
	int read_from_xml(const char *filename);

private:
	int load_ten_even_draw_diamond_xml_info(xmlNodePtr cur);

private:
	uint32_t total_prob;
	TenEvenDrawDiamondXmlMap draw_map;
};
extern TenEvenDrawDiamondXmlManager ten_even_draw_diamond_xml_mgr;

/********************************************************************************/
/*					TenEvenDrawDiamondSpecialXmlManager Class					*/
/********************************************************************************/
struct ten_even_draw_diamond_special_xml_info_t {
	uint32_t id;
	uint32_t prob;
};
typedef std::map<uint32_t, ten_even_draw_diamond_special_xml_info_t> TenEvenDrawDiamondSpecialXmlMap;

class TenEvenDrawDiamondSpecialXmlManager {
public:
	TenEvenDrawDiamondSpecialXmlManager();
	~TenEvenDrawDiamondSpecialXmlManager();

	int random_one_item();
	int read_from_xml(const char *filename);

private:
	int load_ten_even_draw_diamond_special_xml_info(xmlNodePtr cur);

private:
	uint32_t total_prob;
	TenEvenDrawDiamondSpecialXmlMap draw_map;
};
extern TenEvenDrawDiamondSpecialXmlManager ten_even_draw_diamond_special_xml_mgr;

#endif //TEN_EVEN_DRAW_HPP_

