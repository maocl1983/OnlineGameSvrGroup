/*
 * =====================================================================================
 *
 *  @file  ten_even_draw.cpp 
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

#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"
#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"

#include "global_data.hpp"
#include "ten_even_draw.hpp"
#include "player.hpp"
#include "restriction.hpp"

using namespace std;
using namespace project;

//TenEvenDrawGoldsXmlManager ten_even_draw_golds_xml_mgr;
//TenEvenDrawDiamondXmlManager ten_even_draw_diamond_xml_mgr;
//TenEvenDrawDiamondSpecialXmlManager ten_even_draw_diamond_special_xml_mgr;

/********************************************************************************/
/*							TenEvenDrawManager Class							*/
/********************************************************************************/
TenEvenDrawManager::TenEvenDrawManager(Player *p) : owner(p)
{

}

TenEvenDrawManager::~TenEvenDrawManager()
{

}

#if 0
int
TenEvenDrawManager::ten_even_draw_for_golds(uint32_t type, cli_ten_even_draw_request1_out &out)
{
	if (!type || type > 2) {
		return cli_invalid_input_arg_err;
	}
	uint32_t used_free_tms = owner->res_mgr->get_res_value(daily_ten_even_draw_free_tms);
	if (type == 1) {//单抽
		if (used_free_tms <= 5) {
			//检测CD时间
			uint32_t draw_tm = owner->res_mgr->get_res_value(daily_ten_even_draw_free_tm);
			uint32_t now_sec = get_now_tv()->tv_sec;
			if (now_sec < draw_tm + 10 * 60) {
				uint32_t cost_golds = 10000;
				if (owner->golds < cost_golds) {
					T_KWARN_LOG(owner->user_id, "single draw need golds not enough\t[golds=%u, need_golds=%u]", owner->golds, cost_golds);
					return cli_not_enough_golds_err;
				}
				//扣除金币
				owner->chg_golds(-cost_golds);
			} else {
				//免费次数+1
				used_free_tms++;
				owner->res_mgr->set_res_value(daily_ten_even_draw_free_tms, used_free_tms);
				owner->res_mgr->set_res_value(daily_ten_even_draw_free_tm, now_sec);
			}
		} else {
			uint32_t cost_golds = 10000;
			if (owner->golds < cost_golds) {
				T_KWARN_LOG(owner->user_id, "single draw need golds not enough\t[golds=%u, need_golds=%u]", owner->golds, cost_golds);
				return cli_not_enough_golds_err;
			}
			//扣除金币
			owner->chg_golds(-cost_golds);
		}
	} else {//十连抽
		uint32_t cost_golds = 90000;
		if (owner->golds < cost_golds) {
			T_KWARN_LOG(owner->user_id, "ten even draw need golds not enough\t[golds=%u, need_golds=%u]", owner->golds, cost_golds);
			return cli_not_enough_golds_err;
		}
		//扣除金币
		owner->chg_golds(-cost_golds);
	}

	//增加物品
	if (type == 1) {//单抽
		if (!owner->res_mgr->get_res_value(forever_ten_even_draw_golds_first)) {//首次
			owner->res_mgr->set_res_value(forever_ten_even_draw_golds_first, 1);
			owner->items_mgr->add_item_without_callback(131004, 1);
		} else {
			uint32_t item_id = ten_even_draw_golds_xml_mgr->random_one_item();
			int ret = owner->items_mgr->add_item_without_callback(item_id, 1);
			if (ret) {
				out.items.push_back(item_id);
			}
		}
	} else {//十连抽
		for (int i = 0; i < 10; i++) {
			uint32_t item_id = ten_even_draw_golds_xml_mgr->random_one_item();
			owner->items_mgr->add_item_without_callback(item_id, 1);
			out.items.push_back(item_id);
		}
	}

	uint32_t now_sec = get_now_tv()->tv_sec;
	uint32_t draw_tm = owner->res_mgr->get_res_value(daily_ten_even_draw_free_tm);
	out.type = type;
	out.left_tms = (used_free_tms < 5) ? (5 - used_free_tms) : 0;
	out.cd = (now_sec < draw_tm + 10 * 60) ? (draw_tm + 10 * 60 - now_sec) : 0;

	return 0;
}
#endif

int
TenEvenDrawManager::ten_even_draw_for_golds(uint32_t type, cli_ten_even_draw_request1_out &out)
{
	if (!type || type > 2) {
		return cli_invalid_input_arg_err;
	}
	//uint32_t used_free_tms = owner->res_mgr->get_res_value(daily_ten_even_draw_free_tms);
	if (type == 1) {//单抽
			//检测CD时间
		uint32_t draw_tm = owner->res_mgr->get_res_value(daily_ten_even_draw_free_tm);
		uint32_t now_sec = get_now_tv()->tv_sec;
		if (now_sec < draw_tm + 24 * 60 * 60) {
			uint32_t cost_diamond = 100;
			if (owner->diamond < cost_diamond) {
				T_KWARN_LOG(owner->user_id, "single draw need diamond not enough\t[diamond=%u, need_diamond=%u]", owner->diamond, cost_diamond);
				return cli_not_enough_diamond_err;
			}
			//扣除钻石
			owner->chg_diamond(-cost_diamond);
		} else {
			//免费次数+1
			//used_free_tms++;
			//owner->res_mgr->set_res_value(daily_ten_even_draw_free_tms, used_free_tms);
			owner->res_mgr->set_res_value(daily_ten_even_draw_free_tm, now_sec);
		}
	} else {//十连抽
			uint32_t cost_diamond = 900;
			if (owner->diamond < cost_diamond) {
				T_KWARN_LOG(owner->user_id, "ten even draw need diamond not enough\t[diamond=%u, need_diamond=%u]", owner->diamond, cost_diamond);
				return cli_not_enough_diamond_err;
			}
			//扣除钻石
			owner->chg_diamond(-cost_diamond);
	}

	//增加物品
	if (type == 1) {//单抽
		if (!owner->res_mgr->get_res_value(forever_ten_even_draw_golds_first)) {//首次
			owner->res_mgr->set_res_value(forever_ten_even_draw_golds_first, 1);
			owner->items_mgr->add_item_without_callback(131004, 1);
		} else {
			uint32_t item_id = ten_even_draw_golds_xml_mgr->random_one_item();
			int ret = owner->items_mgr->add_item_without_callback(item_id, 1);
			if (ret) {
				out.items.push_back(item_id);
			}
		}
	} else {//十连抽
		for (int i = 0; i < 10; i++) {
			uint32_t item_id = ten_even_draw_golds_xml_mgr->random_one_item();
			owner->items_mgr->add_item_without_callback(item_id, 1);
			out.items.push_back(item_id);
		}
	}

	uint32_t now_sec = get_now_tv()->tv_sec;
	uint32_t draw_tm = owner->res_mgr->get_res_value(daily_ten_even_draw_free_tm);
	out.type = type;
	out.cd = (now_sec < draw_tm + 24 * 60 * 60) ? (draw_tm + 24 * 60 * 60 - now_sec) : 0;

	return 0;
}

int
TenEvenDrawManager::ten_even_draw_for_diamond(uint32_t type, cli_ten_even_draw_request2_out &out)
{
	if (!type || type > 2) {
		return cli_invalid_input_arg_err;
	}

	uint32_t draw_tms = owner->res_mgr->get_res_value(forever_ten_even_draw_diamond_tms);
	if (type == 1) {//单抽
		uint32_t draw_tm = owner->res_mgr->get_res_value(forever_ten_even_draw_free_tm);
		uint32_t now_sec = get_now_tv()->tv_sec;
		if (now_sec < draw_tm + 48 * 3600) {
			uint32_t cost_diamond = 280;
			if (owner->diamond < cost_diamond) {
				T_KWARN_LOG(owner->user_id, "single draw need diamond not enough\t[diamond=%u, need_diamond=%u]", owner->diamond, cost_diamond);
				return cli_not_enough_diamond_err;
			}
			//扣除钻石
			owner->chg_diamond(-cost_diamond);
		} else {
			owner->res_mgr->set_res_value(forever_ten_even_draw_free_tm, now_sec);
		}
	} else {//十连抽
		uint32_t cost_diamond = 2450;
		if (owner->diamond < cost_diamond) {
			T_KWARN_LOG(owner->user_id, "ten even draw need diamond not enough\t[diamond=%u, need_diamond=%u]", owner->diamond, cost_diamond);
			return cli_not_enough_diamond_err;
		}
		//扣除钻石
		owner->chg_diamond(-cost_diamond);
	}

	if (type == 1) {//单抽
		uint32_t first_flag = owner->res_mgr->get_res_value(forever_ten_even_draw_diamond_first);
		uint32_t item_id = 0;
		if (!first_flag) {//首次单抽必送五星武将
			owner->res_mgr->set_res_value(forever_ten_even_draw_diamond_first, 1);
			item_id = 131001;
		} else {
			draw_tms++;
			if (!owner->res_mgr->get_res_value(forever_ten_even_draw_first)) {//非新手引导首次购买
				owner->res_mgr->set_res_value(forever_ten_even_draw_first, 1);
				item_id = 131008;
			} else {
				//第3,5,10必出武将
				if (draw_tms == 3 || draw_tms == 5 || draw_tms == 10 || (draw_tms > 10 && draw_tms % 10 == 0)) {
					item_id = ten_even_draw_diamond_special_xml_mgr->random_one_item();
				} else {
					item_id = ten_even_draw_diamond_xml_mgr->random_one_item();
				}
			}
		}
		int ret = owner->items_mgr->add_item_without_callback(item_id, 1);
		if (ret) {
			out.items.push_back(item_id);
		}
	} else {//十连抽
		for (int i = 0; i < 10; i++) {
			draw_tms++;
			uint32_t item_id = 0;
			if (draw_tms == 3 || draw_tms == 5 || draw_tms == 10 || (draw_tms > 10 && draw_tms % 10 == 0)) {
				item_id = ten_even_draw_diamond_special_xml_mgr->random_one_item();
			} else {
				item_id = ten_even_draw_diamond_xml_mgr->random_one_item();
			}
			int ret = owner->items_mgr->add_item_without_callback(item_id, 1);
			if (ret) {
				out.items.push_back(item_id);
			}
		}
	}

	//设置抽取次数
	owner->res_mgr->set_res_value(forever_ten_even_draw_diamond_tms, draw_tms);

	uint32_t draw_tm = owner->res_mgr->get_res_value(forever_ten_even_draw_free_tm);
	uint32_t now_sec = get_now_tv()->tv_sec;
	out.type = type;
	out.cd = (now_sec < draw_tm + 48 * 3600) ? (draw_tm + 48 * 3600 - now_sec) : 0;
	out.left_draw_tms = calc_left_draw_tms_can_special();

	return 0;
}

int 
TenEvenDrawManager::calc_left_draw_tms_can_special()
{
	uint32_t draw_tms = owner->res_mgr->get_res_value(forever_ten_even_draw_diamond_tms);
	draw_tms += 1;
	uint32_t left_tms = 0;
	if (draw_tms <= 3) {
		left_tms = 3 - draw_tms;
	} else if (draw_tms <= 5) {
		left_tms = 5 - draw_tms;
	} else if (draw_tms <= 10) {
		left_tms = 10 - draw_tms;
	} else {
		if (draw_tms % 10 == 0) {
			left_tms = 0;
		} else {
			uint32_t t = (draw_tms / 10 + 1) * 10;
			left_tms = t - draw_tms;
		}
	}

	return left_tms;
}

void 
TenEvenDrawManager::pack_ten_even_draw_info(cli_get_ten_even_draw_info_out &out)
{
	uint32_t free_tms = owner->res_mgr->get_res_value(daily_ten_even_draw_free_tms);
	uint32_t tm1 = owner->res_mgr->get_res_value(daily_ten_even_draw_free_tm);
	uint32_t tm2 = owner->res_mgr->get_res_value(forever_ten_even_draw_free_tm);
	uint32_t now_tm = get_now_tv()->tv_sec;
	out.left_tms = (free_tms < 5) ? (5 - free_tms) : 0;
	out.cd1 = (now_tm < tm1 + 24 * 3600) ? (tm1 + 24 * 3600 - now_tm) : 0;
	out.cd2 = (now_tm < tm2 + 48 * 3600) ? (tm2 + 48 * 3600 - now_tm) : 0;
	out.single_cost1 = 100;
	out.ten_cost1 = 900;
	out.single_cost2 = 280;
	out.ten_cost2 = 2450;
	out.left_draw_tms = calc_left_draw_tms_can_special();
}

/********************************************************************************/
/*						TenEvenDrawGoldsXmlManager Class						*/
/********************************************************************************/
TenEvenDrawGoldsXmlManager::TenEvenDrawGoldsXmlManager()
{
	total_prob = 0;
	draw_map.clear();
}

TenEvenDrawGoldsXmlManager::~TenEvenDrawGoldsXmlManager()
{

}

int
TenEvenDrawGoldsXmlManager::random_one_item()
{
	if (!total_prob) {
		return 0;
	}
	uint32_t r = rand() % total_prob;
	uint32_t cur_prob = 0;
	TenEvenDrawGoldsXmlMap::iterator it = draw_map.begin();
	for (; it != draw_map.end(); ++it) {
		cur_prob += it->second.prob;
		if (r < cur_prob) {
			return it->second.id;
		}
	}

	return 0;
}

int
TenEvenDrawGoldsXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_ten_even_draw_golds_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);

	return ret;
}

int
TenEvenDrawGoldsXmlManager::load_ten_even_draw_golds_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("item"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "item_id");
			TenEvenDrawGoldsXmlMap::iterator it = draw_map.find(id);
			if (it != draw_map.end()) {
				ERROR_LOG("load ten even draw golds xml info err, id exist, id=%u", id);
				return -1;
			}

			ten_even_draw_golds_xml_info_t info;
			info.id = id;
			get_xml_prop(info.prob, cur, "item_prob");
			total_prob += info.prob;

			TRACE_LOG("load ten even draw golds xml info\t[%u %u]", info.id, info.prob);

			draw_map.insert(TenEvenDrawGoldsXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*						TenEvenDrawDiamondXmlManager Class						*/
/********************************************************************************/
TenEvenDrawDiamondXmlManager::TenEvenDrawDiamondXmlManager()
{

}

TenEvenDrawDiamondXmlManager::~TenEvenDrawDiamondXmlManager()
{

}

int
TenEvenDrawDiamondXmlManager::random_one_item()
{
	if (!total_prob) {
		return 0;
	}
	uint32_t r = rand() % total_prob;
	uint32_t cur_prob = 0;
	TenEvenDrawDiamondXmlMap::iterator it = draw_map.begin();
	for (; it != draw_map.end(); ++it) {
		cur_prob += it->second.prob;
		if (r < cur_prob) {
			return it->second.id;
		}
	}

	return 0;
}

int
TenEvenDrawDiamondXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_ten_even_draw_diamond_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);

	return ret;
}

int
TenEvenDrawDiamondXmlManager::load_ten_even_draw_diamond_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("item"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "item_id");
			TenEvenDrawDiamondXmlMap::iterator it = draw_map.find(id);
			if (it != draw_map.end()) {
				ERROR_LOG("load ten even draw diamond xml info err, id exist, id=%u", id);
				return -1;
			}

			ten_even_draw_diamond_xml_info_t info;
			info.id = id;
			get_xml_prop(info.prob, cur, "item_prob");
			total_prob += info.prob;

			TRACE_LOG("load ten even draw diamond xml info\t[%u %u]", info.id, info.prob);

			draw_map.insert(TenEvenDrawDiamondXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}

	return 0;
}
/********************************************************************************/
/*						TenEvenDrawDiamondSpecialXmlManager Class				*/
/********************************************************************************/
TenEvenDrawDiamondSpecialXmlManager::TenEvenDrawDiamondSpecialXmlManager()
{

}

TenEvenDrawDiamondSpecialXmlManager::~TenEvenDrawDiamondSpecialXmlManager()
{

}

int
TenEvenDrawDiamondSpecialXmlManager::random_one_item()
{
	if (!total_prob) {
		return 0;
	}
	uint32_t r = rand() % total_prob;
	uint32_t cur_prob = 0;
	TenEvenDrawDiamondSpecialXmlMap::iterator it = draw_map.begin();
	for (; it != draw_map.end(); ++it) {
		cur_prob += it->second.prob;
		if (r < cur_prob) {
			return it->second.id;
		}
	}

	return 0;
}

int
TenEvenDrawDiamondSpecialXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_ten_even_draw_diamond_special_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);

	return ret;
}

int
TenEvenDrawDiamondSpecialXmlManager::load_ten_even_draw_diamond_special_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("item"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "item_special_id");
			TenEvenDrawDiamondSpecialXmlMap::iterator it = draw_map.find(id);
			if (it != draw_map.end()) {
				ERROR_LOG("load ten even draw diamond special xml info err, id exist, id=%u", id);
				return -1;
			}

			ten_even_draw_diamond_special_xml_info_t info;
			info.id = id;
			get_xml_prop(info.prob, cur, "item_prob");
			total_prob += info.prob;

			TRACE_LOG("load ten even draw diamond special xml info\t[%u %u]", info.id, info.prob);

			draw_map.insert(TenEvenDrawDiamondSpecialXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}

	return 0;
}
/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/

/* @brief 拉取十连抽信息
 */
int cli_get_ten_even_draw_info(Player *p, Cmessage *c_in)
{
	cli_get_ten_even_draw_info_out cli_out;
	p->ten_even_draw_mgr->pack_ten_even_draw_info(cli_out);

	T_KDEBUG_LOG(p->user_id, "GET TEN EVEN DRAW INFO\t[left_tms=%u, cd1=%u, cd2=%u]", cli_out.left_tms, cli_out.cd1, cli_out.cd2);

	return p->send_to_self(p->wait_cmd, &cli_out, 1); 
}

/* @brief 十连抽请求-铜钱
 */
int cli_ten_even_draw_request1(Player *p, Cmessage *c_in)
{
	cli_ten_even_draw_request1_in *p_in = P_IN;
	if (!p_in->type || p_in->type > 2) {
		return p->send_to_self_error(p->wait_cmd, cli_invalid_input_arg_err, 1);
	}

	cli_ten_even_draw_request1_out cli_out;
	int ret = p->ten_even_draw_mgr->ten_even_draw_for_golds(p_in->type, cli_out);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "TEN EVEN DRAW REQUEST\t[type=%u]", p_in->type);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 十连抽请求-元宝
 */
int cli_ten_even_draw_request2(Player *p, Cmessage *c_in)
{
	cli_ten_even_draw_request2_in *p_in = P_IN;
	if (!p_in->type || p_in->type > 2) {
		return p->send_to_self_error(p->wait_cmd, cli_invalid_input_arg_err, 1);
	}

	cli_ten_even_draw_request2_out cli_out;
	int ret = p->ten_even_draw_mgr->ten_even_draw_for_diamond(p_in->type, cli_out);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "TEN EVEN DRAW REQUEST\t[type=%u]", p_in->type);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

