/*
 * =====================================================================================
 *
 *  @file  shop.cpp 
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

#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"
#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"

#include "shop.hpp"
#include "player.hpp"
#include "utils.hpp"
#include "dbroute.hpp"

using namespace std;
using namespace project;

ShopXmlManager shop_xml_mgr;
ItemShopXmlManager item_shop_xml_mgr;

/********************************************************************************/
/*							ShopManager Class									*/
/********************************************************************************/
ShopManager::ShopManager(Player *p) : owner(p)
{
	shop_map.clear();
}

ShopManager::~ShopManager()
{
	shop_map.clear();
}

void 
ShopManager::init_shop_list(db_get_shop_list_out *p_in)
{
	for (uint32_t i = 0; i < p_in->shop_list.size(); i++) {
		db_shop_info_t *p_info = &(p_in->shop_list[i]);
		shop_info_t info;
		info.type = p_info->type;
		for (int j = 0; j < 6; j++) {
			info.goods[j] = p_info->goods_id[j];
		}
		info.buy_stat = p_info->buy_stat;
		info.refresh_tm = p_info->refresh_tm;

		shop_map.insert(ShopMap::value_type(info.type, info));

		T_KTRACE_LOG(owner->user_id, "init shop list\t[%u %u %u %u %u %u %u %u %u %u %u]", 
				info.type, info.goods[0], info.goods[1], info.goods[2], info.goods[3], info.goods[4], info.goods[5], info.goods[6], info.goods[7], 
				info.buy_stat, info.refresh_tm);
	}
}

const shop_info_t *
ShopManager::get_shop_info(uint32_t type)
{
	ShopMap::iterator it = shop_map.find(type);
	if (it == shop_map.end()) {
		return 0;
	}

	return &(it->second);
}

int
ShopManager::refresh_shop(uint32_t type, uint32_t refresh_tm)
{
	shop_info_t info = {};
	info.type = type;
	for (int i = 0; i < 6; i++) {
		const goods_xml_info_t *p_info = shop_xml_mgr.random_one_goods(type);
		if (!p_info) {
			continue;
		}
		while (owner->lv < p_info->role_lv || owner->vip_lv < p_info->vip_lv) {
			p_info = shop_xml_mgr.random_one_goods(type);
		}
		info.goods[i] = p_info->goods_id;
	}
	info.buy_stat = 0;
	info.refresh_tm = refresh_tm ? refresh_tm : get_now_tv()->tv_sec;

	ShopMap::iterator it = shop_map.find(type);
	if (it == shop_map.end()) {//没有则插入
		shop_map.insert(ShopMap::value_type(type, info));
	} else {
		it->second.buy_stat = 0;
		it->second.refresh_tm = info.refresh_tm;
		for (int i = 0; i < 6; i++) {
			it->second.goods[i] = info.goods[i];
		}
	}

	//更新DB
	db_set_shop_info_in db_in;
	db_in.shop_info.type = type;
	for (int i = 0; i < 6; i++) {
		db_in.shop_info.goods_id[i] = info.goods[i];
	}
	db_in.shop_info.buy_stat = 0;
	db_in.shop_info.refresh_tm = info.refresh_tm;
	send_msg_to_dbroute(0, db_set_shop_info_cmd, &db_in, owner->user_id);

	return 0;
}

int
ShopManager::calc_shop_left_refresh_tm(uint32_t type)
{
	const shop_info_t *p_info = get_shop_info(type);
	if (!p_info) {
		return 0;
	}

	uint32_t now_sec = time(0);
	uint32_t now_hour = utils_mgr.get_hour(now_sec);
	uint32_t now_hour_whole_tm = utils_mgr.get_cur_hour_whole_tm();
	uint32_t refresh_tm = 0;
	if (now_hour % 2 == 0) {
		refresh_tm = now_hour_whole_tm + 2 * 3600;
	} else {
		refresh_tm = now_hour_whole_tm + 3600;
	}
	
	uint32_t left_tm = now_sec < refresh_tm ? refresh_tm - now_sec : 0;

	return left_tm;
}

int
ShopManager::calc_shop_free_refresh_tms(uint32_t type)
{
	uint32_t res_type = 0;
	if (type == 1) {
		res_type = forever_shop_1_free_refresh_tms;
	} else if (type == 2) {
		res_type = forever_shop_2_free_refresh_tms;
	} else if (type == 3) {
		res_type = forever_shop_3_free_refresh_tms;
	} else {
		return 0;
	}
	const shop_info_t *p_info = get_shop_info(type);
	if (!p_info) {
		return 0;
	}

	uint32_t free_refresh_tms = owner->res_mgr->get_res_value(res_type);
	uint32_t old_free_refresh_tms = free_refresh_tms;
	if (free_refresh_tms >= 3) {
		return 3;
	}

	uint32_t now_sec = time(0);
	uint32_t refresh_tm = p_info->refresh_tm;
	uint32_t diff_tm = now_sec > refresh_tm ? now_sec - refresh_tm : 0;
	if (diff_tm > 6 * 3600) {
		free_refresh_tms += 3;
	} else {
		uint32_t now_hour = utils_mgr.get_hour(now_sec);
		uint32_t refresh_hour = utils_mgr.get_hour(refresh_tm);
		if (now_hour < refresh_hour) {
			now_hour += 24;
		}
		for (uint32_t hour = refresh_hour + 1; hour <= now_hour; hour++) {
			if (hour % 2 == 0) {
				free_refresh_tms += 1;
			}
		}
	}

	if (free_refresh_tms > 3) {
		free_refresh_tms = 3;
	}

	if (old_free_refresh_tms != free_refresh_tms) {
		owner->res_mgr->set_res_value(res_type, free_refresh_tms);
	}

	return free_refresh_tms;
}

int
ShopManager::refresh_shop_request(uint32_t type)
{
	if (!type || type > 3) {
		T_KWARN_LOG(owner->user_id, "shop type err\t[type=%u]", type);
		return cli_invalid_input_arg_err;
	}

	const shop_info_t *p_info = get_shop_info(type);
	if (!p_info) {
		T_KWARN_LOG(owner->user_id, "shop type not exist\t[type=%u]", type);
		return cli_shop_not_exist_err;
	}

	uint32_t free_tms = calc_shop_free_refresh_tms(type);
	if (free_tms) {
		free_tms--;
		uint32_t res_type = forever_shop_1_free_refresh_tms;
		if (type == 2) {
			res_type = forever_shop_2_free_refresh_tms;
		} else if (type == 3) {
			res_type = forever_shop_3_free_refresh_tms;
		}
		owner->res_mgr->set_res_value(res_type, free_tms);
	} else {
		//检查道具
		uint32_t refresh_item = 120005;//TODO
		uint32_t cnt = owner->items_mgr->get_item_cnt(refresh_item);
		if (cnt) {
			owner->items_mgr->del_item_without_callback(refresh_item, 1);
		} else {//钻石
			uint32_t cost_diamond = 50;
			if (owner->diamond < cost_diamond) {
				T_KWARN_LOG(owner->user_id, "refresh shop need diamond not enough\t[diamond=%u, need_diamond=%u]", owner->diamond, cost_diamond);
				return cli_not_enough_diamond_err;
			}
			owner->chg_diamond(-cost_diamond);
		}
	}

	//刷新商店
	refresh_shop(type);

	return 0;

}

int
ShopManager::set_shop_buy_stat(uint32_t type, uint32_t id)
{
	if (!type || type > 3 || !id || id > 6) {
		return 0;
	}

	ShopMap::iterator it = shop_map.find(type);
	if (it == shop_map.end()) {
		return 0;
	}

	uint32_t stat = it->second.buy_stat;
	stat = set_bit_on(stat, id);
	it->second.buy_stat = stat;

	//更新DB
	db_set_shop_buy_stat_in db_in;
	db_in.type = type;
	db_in.buy_stat = stat;

	return send_msg_to_dbroute(0, db_set_shop_buy_stat_cmd, &db_in, owner->user_id);
}

int
ShopManager::set_free_refresh_tms(uint32_t type, uint32_t refresh_tms)
{
	uint32_t res_type = 0;
	if (type == 1) {
		res_type = forever_shop_1_free_refresh_tms;
	} else if (type == 2) {
		res_type = forever_shop_2_free_refresh_tms;
	} else if (type == 3) {
		res_type = forever_shop_3_free_refresh_tms;
	} else {
		return 0;
	}

	owner->res_mgr->set_res_value(res_type, refresh_tms);

	return 0;
}

int
ShopManager::buy_goods(uint32_t type, uint32_t id)
{
	if (type == 3) {
		return buy_item_goods(id);
	} else if (type == 4) {
		return buy_gem_goods(id);
	}

	if (!type || type > 4 || !id || id > 6) {
		T_KWARN_LOG(owner->user_id, "buy goods input arg err\t[type=%u, id=%u]", type, id);
		return cli_invalid_input_arg_err;
	}


	const shop_info_t *p_info = get_shop_info(type);
	if (!p_info) {
		T_KWARN_LOG(owner->user_id, "shop type not exist\t[type=%u]", type);
		return cli_shop_not_exist_err;
	}

	//检查是否已购买过
	if (test_bit_on(p_info->buy_stat, id)) {
		T_KWARN_LOG(owner->user_id, "shop goods already buy\t[type=%u, id=%u]", type, id);
		return cli_shop_goods_already_buy_err;
	}

	const goods_xml_info_t *p_xml_info = shop_xml_mgr.get_goods_xml_info(type, p_info->goods[id - 1]);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "buy goods err, type=%u, goods_id=%u", type, p_info->goods[id - 1]);
		return cli_invalid_goods_id_err;
	}

	//检查角色等级
	if (owner->lv < p_xml_info->role_lv) {
		T_KWARN_LOG(owner->user_id, "buy goods lv not enough\t[lv=%u, need_lv=%u]", owner->lv, p_xml_info->role_lv);
		return cli_buy_goods_role_lv_not_enough_err;
	}

	//检查VIP等级
	if (owner->vip_lv < p_xml_info->vip_lv) {
		T_KWARN_LOG(owner->user_id, "buy goods vip lv not enough\t[lv=%u, need_lv=%u]", owner->vip_lv, p_xml_info->vip_lv);
		return cli_buy_goods_vip_lv_not_enough_err;
	}

	//检查花费是否足够
	if (p_xml_info->cost_type == 1) {//金币
		if (owner->golds < p_xml_info->cost) {
			T_KWARN_LOG(owner->user_id, "buy goods golds not enough\t[golds=%u, need_golds=%u]", owner->golds, p_xml_info->cost);
			return cli_not_enough_golds_err;
		}
		owner->chg_golds(-p_xml_info->cost);
	} else if (p_xml_info->cost_type == 2) {//魂玉
		if (owner->hero_soul < p_xml_info->cost) {
			T_KWARN_LOG(owner->user_id, "buy goods hero_soul not enough\t[hero_soul=%u, need_hero_soul=%u]", owner->hero_soul, p_xml_info->cost);
			return cli_not_enough_hero_soul_err;
		}
		owner->chg_hero_soul(-p_xml_info->cost);
	} else if (p_xml_info->cost_type == 3) {//钻石
		if (owner->diamond < p_xml_info->cost) {
			T_KWARN_LOG(owner->user_id, "buy goods diamond not enough\t[diamond=%u, need_diamond=%u]", owner->diamond, p_xml_info->cost);
			return cli_not_enough_diamond_err;
		}
		owner->chg_diamond(-p_xml_info->cost);
	}

	owner->items_mgr->add_reward(p_xml_info->item_id, p_xml_info->item_cnt);

	//奖励通知
	cli_send_get_common_bonus_noti_out noti_out;
	owner->items_mgr->pack_give_items_info(noti_out.give_items_info, p_xml_info->item_id, p_xml_info->item_cnt);
	owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	//设置购买标志
	set_shop_buy_stat(type, id);

	return 0;
}

int
ShopManager::buy_item_goods(uint32_t id)
{
	const item_shop_xml_info_t *p_xml_info = item_shop_xml_mgr.get_item_shop_xml_info_by_goods_id(id);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "buy goods input arg err\t[id=%u]", id);
		return cli_invalid_input_arg_err;
	}
	
	uint32_t item_id = p_xml_info->item_id;
	//检查剩余次数
	uint32_t left_tms = get_item_shop_left_buy_tms(item_id);
	if (!left_tms) {
		T_KWARN_LOG(owner->user_id, "goods buy tms not enough\[id=%u]", id);
		return cli_goods_buy_tms_not_enough_err;
	}

	//检查元宝是否足够
	uint32_t res_type = p_xml_info->res_type;
	uint32_t res_value = owner->res_mgr->get_res_value(res_type);

	uint32_t price = get_item_shop_price(item_id, res_value);
	if (owner->diamond < price) {
		T_KWARN_LOG(owner->user_id, "buy item goods diamond not enough\t[diamond=%u, need_diamond=%u]", owner->diamond, price);
		return cli_not_enough_diamond_err;
	}

	//扣除元宝
	owner->chg_diamond(-price);

	//添加道具
	owner->items_mgr->add_item_without_callback(item_id, 1);

	//次数+1
	res_value++;
	owner->res_mgr->set_res_value(res_type, res_value);

	//奖励通知
	cli_send_get_common_bonus_noti_out noti_out;
	owner->items_mgr->pack_give_items_info(noti_out.give_items_info, item_id, 1);
	owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	return 0;
}

int
ShopManager::buy_gem_goods(uint32_t id)
{
	if (!id || id > 4) {
		T_KWARN_LOG(owner->user_id, "buy goods input arg err\t[id=%u]", id);
		return cli_invalid_input_arg_err;
	}
	
	uint32_t gem_id[4] = {110000, 111000, 112000, 113000};
	uint32_t item_id = gem_id[id - 1];

	//检查元宝是否足够
	if (owner->diamond < 25) {
		T_KWARN_LOG(owner->user_id, "buy item goods diamond not enough\t[diamond=%u, need_diamond=%u]", owner->diamond, 25);
		return cli_not_enough_diamond_err;
	}

	//扣除元宝
	owner->chg_diamond(-25);

	//添加道具
	owner->items_mgr->add_item_without_callback(item_id, 1);

	//奖励通知
	cli_send_get_common_bonus_noti_out noti_out;
	owner->items_mgr->pack_give_items_info(noti_out.give_items_info, item_id, 1);
	owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	return 0;
}

int
ShopManager::get_item_shop_price(uint32_t item_id, uint32_t buy_tms)
{
	uint32_t price = item_shop_xml_mgr.get_item_shop_price(item_id, buy_tms);

	if (!price) {
		price = -1;
	}

	return price;
}

int
ShopManager::get_item_shop_buy_limit(uint32_t item_id)
{
	uint32_t buy_limit = 5;
	if (item_id == 120007) {
		buy_limit = 2;
	}

	return buy_limit;
}

int
ShopManager::get_item_shop_left_buy_tms(uint32_t item_id)
{
	uint32_t buy_limit = get_item_shop_buy_limit(item_id);

	const item_shop_xml_info_t *p_xml_info = item_shop_xml_mgr.get_item_shop_xml_info(item_id);
	if (p_xml_info) {
		uint32_t buy_tms = owner->res_mgr->get_res_value(p_xml_info->res_type);
		uint32_t left_tms = buy_tms < buy_limit ? buy_limit - buy_tms : 0;

		return left_tms;
	}

	return 0;
}

int
ShopManager::get_shop_left_buy_tms(uint32_t type, uint32_t id)
{
	uint32_t left_buy_tms = 0;
	if (type == 3) {
		const item_shop_xml_info_t *p_xml_info = item_shop_xml_mgr.get_item_shop_xml_info_by_goods_id(id);
		if (p_xml_info) {
			left_buy_tms = get_item_shop_left_buy_tms(p_xml_info->item_id);
		}
	} else if (type == 4) {
		return 10000;
	} else {
		const shop_info_t *p_info = get_shop_info(type);
		if (p_info) {
			left_buy_tms = test_bit_on(p_info->buy_stat, id) ? 0 : 1;
		}
	}

	return left_buy_tms;
}

int
ShopManager::get_shop_price(uint32_t type, uint32_t id)
{
	int price = -1;
	if (type == 3) {
		const item_shop_xml_info_t *p_xml_info = item_shop_xml_mgr.get_item_shop_xml_info_by_goods_id(id);
		if (p_xml_info) {
			uint32_t buy_tms = owner->res_mgr->get_res_value(p_xml_info->res_type);
			price = get_item_shop_price(p_xml_info->item_id, buy_tms);
		}
	} else if (type == 4) {
		return 25;
	} else {
		const goods_xml_info_t *p_xml_info = shop_xml_mgr.get_goods_xml_info(type, id);
		if (p_xml_info) {
			price = p_xml_info->cost;
		}
	}

	return price;
}

void
ShopManager::pack_client_item_shop_info(cli_shop_info_t &shop_info)
{
	shop_info.type = 3;
	item_shop_xml_mgr.pack_item_shop_info(owner, shop_info.goods);
}

void
ShopManager::pack_client_gem_shop_info(cli_shop_info_t &shop_info)
{
	shop_info.type = 4;
	uint32_t item_id[4] = {110000, 111000, 112000, 113000};
	for (int i = 0; i < 4; i++) {
		cli_goods_info_t goods;
		goods.id = i + 1;
		goods.item_id = item_id[i];
		goods.item_cnt = 1;
		goods.cost_type = 3;
		goods.cost = 25;
		goods.left_buy_tms = 10000;//表示不限次数
		shop_info.goods.push_back(goods);
	}
}

void
ShopManager::pack_client_shop_info(uint32_t type, cli_shop_info_t &shop_info)
{
	if (!type || type > 4) {
		return;
	}
	if (type == 3) {//道具商店
		return pack_client_item_shop_info(shop_info);
	} else if (type == 4) {//宝石商店
		return pack_client_gem_shop_info(shop_info);
	}

	const shop_info_t *p_info = get_shop_info(type);
	if (!p_info) {//第一次则插入
		refresh_shop(type);
		set_free_refresh_tms(type, 3);
	}

	p_info = get_shop_info(type);
	if (!p_info) {
		return;
	}

	uint32_t free_refresh_tms = calc_shop_free_refresh_tms(type);
	uint32_t zero_tm = utils_mgr.get_next_day_zero_tm(p_info->refresh_tm);
	uint32_t now_sec = get_now_tv()->tv_sec;
	if (p_info->refresh_tm < zero_tm && now_sec >= zero_tm) {//每天零点刷新
		refresh_shop(type, zero_tm);
		free_refresh_tms++;
		if (free_refresh_tms > 3) {
			free_refresh_tms = 3;
		}
		set_free_refresh_tms(type, free_refresh_tms);
		free_refresh_tms = calc_shop_free_refresh_tms(type);
	}

	shop_info.type = type;
	shop_info.left_refresh_tm = calc_shop_left_refresh_tm(type);
	shop_info.free_tms = free_refresh_tms;
	for (int i = 0; i < 6; i++) {
		const goods_xml_info_t *p_xml_info = shop_xml_mgr.get_goods_xml_info(type, p_info->goods[i]);
		if (!p_xml_info) {
			continue;
		}
		cli_goods_info_t goods_info;
		goods_info.id = i + 1;
		goods_info.item_id = p_xml_info->item_id;
		goods_info.item_cnt = p_xml_info->item_cnt;
		goods_info.cost_type = p_xml_info->cost_type;
		goods_info.cost = p_xml_info->cost;
		goods_info.left_buy_tms = test_bit_on(p_info->buy_stat, i + 1) ? 0 : 1;
		shop_info.goods.push_back(goods_info);
	}

	T_KTRACE_LOG(owner->user_id, "pack client shop info\t[%u %u %u %u %u %u %u %u %u]", 
			shop_info.type, shop_info.left_refresh_tm, shop_info.free_tms, shop_info.goods[0].item_id, shop_info.goods[1].item_id, shop_info.goods[2].item_id, 
			shop_info.goods[3].item_id, shop_info.goods[4].item_id, shop_info.goods[5].item_id);

}


/********************************************************************************/
/*						ShopXmlManager Class									*/
/********************************************************************************/
ShopXmlManager::ShopXmlManager()
{

}

ShopXmlManager::~ShopXmlManager()
{

}

const goods_xml_info_t*
ShopXmlManager::random_one_goods(uint32_t type)
{
	ShopXmlMap::iterator it = shop_map.find(type);
	if (it == shop_map.end()) {
		return 0;
	}

	shop_xml_info_t *p_xml_info = &(it->second);
	if (!p_xml_info->total_prob) {
		return 0;
	}

	uint32_t r = rand() % p_xml_info->total_prob;
	uint32_t cur_prob = 0;
	GoodsXmlMap::iterator it2 = p_xml_info->goods.begin();
	for (; it2 != p_xml_info->goods.end(); ++it2) {
		cur_prob += it2->second.prob;
		if (r < cur_prob) {
			return &(it2->second);
		}
	}

	return 0;
}

const goods_xml_info_t*
ShopXmlManager::get_goods_xml_info(uint32_t type, uint32_t goods_id)
{
	ShopXmlMap::iterator it = shop_map.find(type);
	if (it == shop_map.end()) {
		return 0;
	}
	GoodsXmlMap::iterator it2 = it->second.goods.find(goods_id);
	if (it2 == it->second.goods.end()) {
		return 0;
	}

	return &(it2->second);
}

int
ShopXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_shop_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
ShopXmlManager::load_shop_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("shop"))) {
			uint32_t type = 0;
			get_xml_prop(type, cur, "type");
			goods_xml_info_t sub_info;
			get_xml_prop(sub_info.goods_id, cur, "id");
			get_xml_prop(sub_info.item_id, cur, "item_id");
			get_xml_prop(sub_info.item_cnt, cur, "item_cnt");
			get_xml_prop(sub_info.prob, cur, "prob");
			get_xml_prop(sub_info.cost_type, cur, "cost_type");
			get_xml_prop(sub_info.cost, cur, "cost");
			get_xml_prop_def(sub_info.role_lv, cur, "role_lv", 0);
			get_xml_prop_def(sub_info.vip_lv, cur, "role_lv", 0);

			ShopXmlMap::iterator it = shop_map.find(type);
			if (it != shop_map.end()) {
				GoodsXmlMap::iterator it2 = it->second.goods.find(sub_info.goods_id);
				if (it2 != it->second.goods.end()) {
					ERROR_LOG("load shop xml info err, shop id exists, type=%u, goods_id=%u", type, sub_info.goods_id);
					return -1;
				}

				it->second.total_prob += sub_info.prob;
				it->second.goods.insert(GoodsXmlMap::value_type(sub_info.goods_id, sub_info));
			} else {
				shop_xml_info_t info;
				info.type = type;
				info.total_prob = sub_info.prob;
				info.goods.insert(GoodsXmlMap::value_type(sub_info.goods_id, sub_info));
				
				shop_map.insert(ShopXmlMap::value_type(type, info));
			}
			TRACE_LOG("load shop xml info\t[%u %u %u %u %u %u %u %u %u]", 
					type, sub_info.goods_id, sub_info.item_id, sub_info.item_cnt, sub_info.prob, sub_info.cost_type, sub_info.cost, sub_info.role_lv, sub_info.vip_lv);
		}
		cur = cur->next;
	}
	return 0;
}

/********************************************************************************/
/*								ItemShopXmlManager								*/
/********************************************************************************/
ItemShopXmlManager::ItemShopXmlManager()
{
	item_shop_xml_map.clear();
}

ItemShopXmlManager::~ItemShopXmlManager()
{

}

const item_shop_xml_info_t *
ItemShopXmlManager::get_item_shop_xml_info_by_goods_id(uint32_t goods_id)
{
	ItemShopXmlMap::iterator it = item_shop_xml_map.begin();
	for (; it != item_shop_xml_map.end(); ++it) {
		if (it->second.id == goods_id) {
			return &(it->second);
		}
	}

	return 0;
}

const item_shop_xml_info_t *
ItemShopXmlManager::get_item_shop_xml_info(uint32_t item_id)
{
	ItemShopXmlMap::iterator it = item_shop_xml_map.find(item_id);
	if (it != item_shop_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

void
ItemShopXmlManager::pack_item_shop_info(Player *p, std::vector<cli_goods_info_t> &goods)
{
	uint32_t size = item_shop_xml_map.size();
	goods.resize(size);

	ItemShopXmlMap::iterator it = item_shop_xml_map.begin();
	for (; it != item_shop_xml_map.end(); ++it) {
		const item_shop_xml_info_t *p_xml_info = &(it->second);
		uint32_t buy_tms = p->res_mgr->get_res_value(p_xml_info->res_type);
		goods[p_xml_info->id - 1].id = p_xml_info->id;
		goods[p_xml_info->id - 1].item_id = p_xml_info->item_id;
		goods[p_xml_info->id - 1].item_cnt = 1;
		goods[p_xml_info->id - 1].cost_type = 3;
		goods[p_xml_info->id - 1].cost = get_item_shop_price(p_xml_info->item_id, buy_tms);
		goods[p_xml_info->id - 1].left_buy_tms = p->shop_mgr->get_item_shop_left_buy_tms(p_xml_info->item_id);

		T_KTRACE_LOG(p->user_id, "pack client shop item shop info\t[%u %u %u %u %u %u]", 
				goods[p_xml_info->id - 1].id, goods[p_xml_info->id - 1].item_id, goods[p_xml_info->id - 1].item_cnt, 
				goods[p_xml_info->id - 1].cost_type, goods[p_xml_info->id - 1].cost, goods[p_xml_info->id - 1].left_buy_tms);
	}
}

int
ItemShopXmlManager::get_item_shop_price(uint32_t item_id, uint32_t buy_tms)
{
	ItemShopXmlMap::iterator it = item_shop_xml_map.find(item_id);
	if (it == item_shop_xml_map.end()) {
		return -1;
	}

	ItemShopBuyXmlMap::iterator it2 = it->second.buy_map.find(buy_tms + 1);
	if (it2 == it->second.buy_map.end()) {
		return -1;
	}

	return it2->second.cost_diamond;
}

int
ItemShopXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_item_shop_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

int
ItemShopXmlManager::load_item_shop_xml_info(xmlNodePtr cur) 
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("shop"))) {
			uint32_t goods_id = 0;
			uint32_t item_id = 0;
			uint32_t res_type = 0;
			item_shop_buy_xml_info_t sub_info = {};
			get_xml_prop(goods_id, cur, "id");
			get_xml_prop(item_id, cur, "item_id");
			get_xml_prop(res_type, cur, "res_type");
			get_xml_prop(sub_info.buy_tms, cur, "buy_tms");
			get_xml_prop(sub_info.cost_diamond, cur, "cost_diamond");

			ItemShopXmlMap::iterator it = item_shop_xml_map.find(item_id);
			if (it != item_shop_xml_map.end()) {
				it->second.buy_map.insert(ItemShopBuyXmlMap::value_type(sub_info.buy_tms, sub_info));			
			} else {
				item_shop_xml_info_t info = {};
				info.id = goods_id;
				info.item_id = item_id;
				info.res_type = res_type;
				info.buy_map.insert(ItemShopBuyXmlMap::value_type(sub_info.buy_tms, sub_info));

				item_shop_xml_map.insert(ItemShopXmlMap::value_type(item_id, info));
			}

			TRACE_LOG("load item shop xml info\t[%u %u %u %u %u]", goods_id, item_id, res_type, sub_info.buy_tms, sub_info.cost_diamond);
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							Client Request										*/
/********************************************************************************/
/* @brief 拉取商城列表
 */
int cli_get_shop_panel_info(Player *p, Cmessage *c_in)
{
	cli_get_shop_panel_info_in *p_in = P_IN;
	cli_get_shop_panel_info_out cli_out;
	p->shop_mgr->pack_client_shop_info(p_in->type, cli_out.shop_info);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 购买商品
 */
int cli_buy_shop(Player *p, Cmessage *c_in)
{
	cli_buy_shop_in *p_in = P_IN;
	int ret = p->shop_mgr->buy_goods(p_in->type, p_in->id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_buy_shop_out cli_out;
	cli_out.type = p_in->type;
	cli_out.id = p_in->id;
	cli_out.left_tms = p->shop_mgr->get_shop_left_buy_tms(p_in->type, p_in->id);
	cli_out.price = p->shop_mgr->get_shop_price(p_in->type, p_in->id);

	T_KDEBUG_LOG(p->user_id, "BUY SHOP\t[type=%u, id=%u, left_tms=%u, price=%u]", p_in->type, p_in->id, cli_out.left_tms, cli_out.price);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 刷新商店
 */
int cli_refresh_shop(Player *p, Cmessage *c_in)
{
	cli_refresh_shop_in *p_in = P_IN;

	cli_refresh_shop_out cli_out;
	int ret = p->shop_mgr->refresh_shop_request(p_in->type);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	p->shop_mgr->pack_client_shop_info(p_in->type, cli_out.shop_info);

	T_KDEBUG_LOG(p->user_id, "REFRESH SHOP\t[type=%u]", p_in->type);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/********************************************************************************/
/*							DB Return											*/
/********************************************************************************/
/* @brief 拉取商城列表
 */
int db_get_shop_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);
	db_get_shop_list_out *p_in = P_IN;

	if (p->wait_cmd == cli_proto_login_cmd) {
		p->shop_mgr->init_shop_list(p_in);

		p->login_step++;
		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u GET SHOP LIST", p->login_step);

		//拉取奇遇信息
		return send_msg_to_dbroute(p, db_get_adventure_list_cmd, 0, p->user_id);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}
