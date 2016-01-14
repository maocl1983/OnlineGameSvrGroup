/*
 * =====================================================================================
 *
 *  @file  shop.hpp 
 *
 *  @brief  商城系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef SHOP_HPP_
#define SHOP_HPP_

#include "common_def.hpp"

class Player;
class db_get_shop_list_out;
class cli_shop_info_t;
class cli_goods_info_t;

/********************************************************************************/
/*							ShopManager Class									*/
/********************************************************************************/
struct shop_info_t {
	uint32_t type;
	uint32_t goods[8];
	uint32_t buy_stat;
	uint32_t refresh_tm;
};
typedef std::map<uint32_t, shop_info_t> ShopMap;

class ShopManager {
public:
	ShopManager(Player *p);
	~ShopManager();

	void init_shop_list(db_get_shop_list_out *p_in);

	const shop_info_t *get_shop_info(uint32_t type);

	int calc_shop_free_refresh_tms(uint32_t type);
	int calc_shop_left_refresh_tm(uint32_t type);
	int set_free_refresh_tms(uint32_t type, uint32_t refresh_tms);
	int refresh_shop(uint32_t type, uint32_t refresh_tm=0);
	int set_shop_buy_stat(uint32_t type, uint32_t id);

	int get_item_shop_price(uint32_t item_id, uint32_t buy_tms);
	int get_item_shop_left_buy_tms(uint32_t item_id);
	int get_item_shop_buy_limit(uint32_t item_id);

	int get_shop_left_buy_tms(uint32_t type, uint32_t id);
	int get_shop_price(uint32_t type, uint32_t id);
	
	int refresh_shop_request(uint32_t type);
	int buy_goods(uint32_t type, uint32_t id);
	int buy_item_goods(uint32_t id);
	int buy_gem_goods(uint32_t id);
	
	void pack_client_item_shop_info(cli_shop_info_t &shop_info);
	void pack_client_gem_shop_info(cli_shop_info_t &shop_info);
	void pack_client_shop_info(uint32_t type, cli_shop_info_t &shop_info);

private:
	Player *owner;
	ShopMap shop_map;
};


/********************************************************************************/
/*						ShopXmlManager Class									*/
/********************************************************************************/
struct goods_xml_info_t {
	uint32_t goods_id;
	uint32_t item_id;
	uint32_t item_cnt;
	uint32_t prob;
	uint32_t cost_type;
	uint32_t cost;
	uint32_t role_lv;
	uint32_t vip_lv;
};
typedef std::map<uint32_t, goods_xml_info_t> GoodsXmlMap;

struct shop_xml_info_t {
	uint32_t type;
	uint32_t total_prob;	
	GoodsXmlMap goods;
};
typedef std::map<uint32_t, shop_xml_info_t> ShopXmlMap;

class ShopXmlManager {
public:
	ShopXmlManager();
	~ShopXmlManager();

	const goods_xml_info_t *random_one_goods(uint32_t type);
	const goods_xml_info_t *get_goods_xml_info(uint32_t type, uint32_t goods_id);
	int read_from_xml(const char *filename);

private:
	int load_shop_xml_info(xmlNodePtr cur);

private:
	ShopXmlMap shop_map;
};
//extern ShopXmlManager shop_xml_mgr;

/********************************************************************************/
/*								ItemShopXmlManager								*/
/********************************************************************************/
struct item_shop_buy_xml_info_t {
	uint32_t buy_tms;
	uint32_t cost_diamond;
};
typedef std::map<uint32_t, item_shop_buy_xml_info_t> ItemShopBuyXmlMap;

struct item_shop_xml_info_t {
	uint32_t id;
	uint32_t item_id;
	uint32_t res_type;
	ItemShopBuyXmlMap buy_map;
};
typedef std::map<uint32_t, item_shop_xml_info_t> ItemShopXmlMap;

class ItemShopXmlManager {
public:
	ItemShopXmlManager();
	~ItemShopXmlManager();

	int read_from_xml(const char *filename);
	const item_shop_xml_info_t *get_item_shop_xml_info(uint32_t item_id);
	const item_shop_xml_info_t *get_item_shop_xml_info_by_goods_id(uint32_t goods_id);
	int get_item_shop_price(uint32_t item_id, uint32_t buy_tms);
	void pack_item_shop_info(Player *p, std::vector<cli_goods_info_t> &goods);

private:
	int load_item_shop_xml_info(xmlNodePtr cur);

private:
	ItemShopXmlMap item_shop_xml_map;
};
//extern ItemShopXmlManager item_shop_xml_mgr;

#endif //SHOP_HPP_
