/*
 * =====================================================================================
 *
 *  @file  general.cpp 
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

#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"
#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"

#include "general.hpp"
#include "player.hpp"
#include "restriction.hpp"
#include "item.hpp"
#include "hero.hpp"
#include "utils.hpp"
#include "lua_log.hpp"
#include "log_thread.hpp"

using namespace std;
using namespace project;

NickXmlManager nick_xml_mgr;
#if 0
int cli_daily_checkin(Player *p, Cmessage *c_in)
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);

	REQUIRE(log);
	REQUIRE(restriction);
	REQUIRE(player);
	REQUIRE(utils);

	if (luaL_dofile(L, "./lualib/general.lua")) {
		T_ERROR_LOG("load failed");
		return -1;
	}

	lua_getglobal(L, "cli_daily_checkin");
	lua_pushlightuserdata(L, p);
	lua_pushlightuserdata(L, c_in);
	if (lua_pcall(L, 2, 0, 0)) {
		T_ERROR_LOG("pcall failed");
		return -1;
	}

	return 0;
}
#endif

/********************************************************************************/
/*									General	Class								*/
/********************************************************************************/
int
General::get_new_player_checkin_reward(Player *p, uint32_t checkin_id)
{
	if (!p || !checkin_id || checkin_id > 7) {
		return -1;
	}

	cli_send_get_common_bonus_noti_out noti_out;
	switch (checkin_id) {
	case 1://500元宝
		p->chg_diamond(500);
		noti_out.diamond = 500;
		break;
	case 2: {//曹操
		int ret = p->items_mgr->add_item_without_callback(131013, 1);
		if (ret) {
			p->items_mgr->pack_give_items_info(noti_out.give_items_info, 131013, 1);
		}
	}
		break;
	case 3: {//4级宝石5颗
		uint32_t gem[4] = {110003, 111003, 112003, 113003};
		for (int i = 0; i < 5; i++) {
			uint32_t r = rand() % 4;
			p->items_mgr->add_item_without_callback(gem[r], 1);
			p->items_mgr->pack_give_items_info(noti_out.give_items_info, gem[r], 1);
		}
	}
		break;
	case 4://陨铁石*20
		p->items_mgr->add_item_without_callback(120001, 20);
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 120001, 20);
		break;
	case 5://精炼石*20
		p->items_mgr->add_item_without_callback(120000, 20);
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 120000, 20);
		break;
	case 6://体力丹*4
		p->items_mgr->add_item_without_callback(120007, 4);
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 120007, 4);
		break;
	case 7://紫色武器一把
		p->equip_mgr->add_one_equip(803000);
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 803000, 1);
		break;
	default:
		break;
	}

	p->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	return 0;
}

int
General::get_level_gift(Player *p, uint32_t gift_id)
{
	if (!p) {
		return 0;
	}

	cli_send_get_common_bonus_noti_out noti_out;

	switch (gift_id) {
	case 1://金币50000
		p->chg_golds(50000);
		noti_out.golds = 50000;
		break;
	case 2://武将经验道具*2
		p->items_mgr->add_item_without_callback(100001, 2);
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 100001, 2);
		break;
	case 3://小兵经验道具*2
		p->items_mgr->add_item_without_callback(102001, 2);
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 102001, 2);
		break;
	case 4://18级 扫荡券*30
		p->items_mgr->add_item_without_callback(103000, 30);
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 103000, 30);
		break;
	case 5://陨铁石*10
		p->items_mgr->add_item_without_callback(120001, 10);
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 120001, 10);
		break;
	case 6: {//2级宝石5颗
		uint32_t gem[4] = {110001, 111001, 112001, 113001};
		for (int i = 0; i < 5; i++) {
			uint32_t r = rand() % 4;
			p->items_mgr->add_item_without_callback(gem[r], 1);
			p->items_mgr->pack_give_items_info(noti_out.give_items_info, gem[r], 1);
		}
	}
	case 7://精炼石*10
		p->items_mgr->add_item_without_callback(120000, 10);
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 120000, 10);
		break;
	case 8://竞技场牌子*500
		break;
	case 9://公会贡献点*2000 TODO
		break;
	default:
		break;

	}

	p->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	return 0;
}

int
General::get_diamond_gift(Player *p, uint32_t gift_id)
{
	if (!p) {
		return 0;
	}

	cli_send_get_common_bonus_noti_out noti_out;
	switch (gift_id) {
	case 1://金币2w,陨铁石2
		p->chg_golds(20000);
		p->items_mgr->add_item_without_callback(120001, 2);
		noti_out.golds = 20000;
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 120001, 2);
		break;
	case 2://金币8W,精炼石20
		p->chg_golds(80000);
		p->items_mgr->add_item_without_callback(120000, 20);
		noti_out.golds = 80000;
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 120000, 20);
		break;
	case 3://武将经验道具*10
		p->items_mgr->add_item_without_callback(100001, 10);
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 100001, 10);
		break;
	case 4://兵种经验道具*10
		p->items_mgr->add_item_without_callback(102001, 10);
		p->items_mgr->pack_give_items_info(noti_out.give_items_info, 102001, 10);
		break;
	case 5:{//3级宝石*5
		uint32_t gem[4] = {110002, 111002, 112002, 113002};
		for (int i = 0; i < 5; i++) {
			uint32_t r = rand() % 4;
			p->items_mgr->add_item_without_callback(gem[r], 1);
			p->items_mgr->pack_give_items_info(noti_out.give_items_info, gem[r], 1);
		}
	}
		break;
	case 6:{//紫色武将 吕布
		int ret = p->items_mgr->add_item_without_callback(131011, 1);
		if (ret) {
			p->items_mgr->pack_give_items_info(noti_out.give_items_info, 131011, 1);
		}
	}
		break;
	case 7:{//橙色武将 董卓
		int ret = p->items_mgr->add_item_without_callback(131009, 1);
		if (ret) {
			p->items_mgr->pack_give_items_info(noti_out.give_items_info, 131009, 1);
		}
	}
	default:
		break;
	}

	p->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	return 0;
}

/********************************************************************************/
/*							NickXmlManager Class								*/
/********************************************************************************/
NickXmlManager::NickXmlManager()
{

}

NickXmlManager::~NickXmlManager()
{

}

const char *
NickXmlManager::random_one_sumname(uint32_t sex)
{
	if (!sex || sex > 2) {
		return 0;
	}

	if (sex == 1) {
		uint32_t sz = male_sumname_xml_map.size();
		uint32_t r = rand() % sz + 1;
		NickXmlMap::iterator it = male_sumname_xml_map.find(r);
		if (it != male_sumname_xml_map.end()) {
			return it->second.nick;
		}
	} else {
		uint32_t sz = female_sumname_xml_map.size();
		uint32_t r = rand() % sz + 1;
		NickXmlMap::iterator it = female_sumname_xml_map.find(r);
		if (it != female_sumname_xml_map.end()) {
			return it->second.nick;
		}
	}

	return 0;
}

const char *
NickXmlManager::random_one_name(uint32_t sex)
{
	if (!sex || sex > 2) {
		return 0;
	}

	if (sex == 1) {
		uint32_t sz = male_name_xml_map.size();
		uint32_t r = rand() % sz + 1;
		NickXmlMap::iterator it = male_name_xml_map.find(r);
		if (it != male_name_xml_map.end()) {
			return it->second.nick;
		}
	} else {
		uint32_t sz = female_name_xml_map.size();
		uint32_t r = rand() % sz + 1;
		NickXmlMap::iterator it = female_name_xml_map.find(r);
		if (it != female_name_xml_map.end()) {
			return it->second.nick;
		}
	}

	return 0;
}

int
NickXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_nick_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return 0;
}

int
NickXmlManager::load_nick_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("type"))) {
			uint32_t sex = 0;
			get_xml_prop(sex, cur, "sex");
			if (sex == 1) {
				int ret = load_male_nick_xml_info(cur);
				if (ret == -1) {
					ERROR_LOG("load nick xml info err, sex=1");
					return -1;
				}
			} else if (sex == 2) {
				int ret = load_female_nick_xml_info(cur);
				if (ret == -1) {
					ERROR_LOG("load nick xml info err, sex=2");
					return -1;
				}
			}
		}
		cur = cur->next;
	}

	return 0;
}

int
NickXmlManager::load_male_nick_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("sumname"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			NickXmlMap::iterator it = male_sumname_xml_map.find(id);
			if (it != male_sumname_xml_map.end()) {
				ERROR_LOG("load male sumname xml info err, id=%u", id);
				return -1;
			}

			nick_xml_info_t info = {};
			info.id = id;
			get_xml_prop(info.nick, cur, "name");

			TRACE_LOG("load male sumname xml info\t[%u %s]", info.id, info.nick);

			male_sumname_xml_map.insert(NickXmlMap::value_type(id, info));

		} else if (!xmlStrcmp(cur->name, XMLCHAR_CAST("name"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			NickXmlMap::iterator it = male_name_xml_map.find(id);
			if (it != male_name_xml_map.end()) {
				ERROR_LOG("load male name xml info err, id=%u", id);
				return -1;
			}

			nick_xml_info_t info = {};
			info.id = id;
			get_xml_prop(info.nick, cur, "name");

			TRACE_LOG("load male name xml info\t[%u %s]", info.id, info.nick);

			male_name_xml_map.insert(NickXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}

	return 0;
}

int
NickXmlManager::load_female_nick_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("sumname"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			NickXmlMap::iterator it = female_sumname_xml_map.find(id);
			if (it != female_sumname_xml_map.end()) {
				ERROR_LOG("load female sumname xml info err, id=%u", id);
				return -1;
			}

			nick_xml_info_t info = {};
			info.id = id;
			get_xml_prop(info.nick, cur, "name");

			TRACE_LOG("load female sumname xml info\t[%u %s]", info.id, info.nick);

			female_sumname_xml_map.insert(NickXmlMap::value_type(id, info));
		} else if (!xmlStrcmp(cur->name, XMLCHAR_CAST("name"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			NickXmlMap::iterator it = female_name_xml_map.find(id);
			if (it != female_name_xml_map.end()) {
				ERROR_LOG("load female name xml info err, id=%u", id);
				return -1;
			}

			nick_xml_info_t info = {};
			info.id = id;
			get_xml_prop(info.nick, cur, "name");

			TRACE_LOG("load female name xml info\t[%u %s]", info.id, info.nick);

			female_name_xml_map.insert(NickXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 每日签到
 */
int cli_daily_checkin(Player *p, Cmessage *c_in)
{

	uint32_t daily_flag =  p->res_mgr->get_res_value(daily_checkin_stat);
	if (daily_flag) {
		return p->send_to_self_error(p->wait_cmd, cli_already_checkin_err, 1);	
	}

	uint32_t cur_day = utils_mgr.get_day(time(0));
	uint32_t month_stat = p->res_mgr->get_res_value(month_checkin_days_stat);
	if (test_bit_on(month_stat, cur_day)) {
		return p->send_to_self_error(p->wait_cmd, cli_already_checkin_err, 1);	
	}

	p->res_mgr->set_res_value(daily_checkin_stat, 1);
	month_stat = set_bit_on(month_stat, cur_day);
	p->res_mgr->set_res_value(month_checkin_days_stat, month_stat);

	//发放奖励 TODO
	p->items_mgr->add_item_without_callback(60000, 1);

	KDEBUG_LOG(p->user_id, "DAILY CHECKIN\t[cur_day=%u month_stat=%u]", cur_day, month_stat);

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 拉取新手签到面板
 */
int cli_get_new_player_checkin_panel_info(Player *p, Cmessage *c_in)
{
	cli_get_new_player_checkin_panel_info_out cli_out;
	cli_out.login_day = p->res_mgr->get_res_value(forever_player_login_day);
	cli_out.today_flag = p->res_mgr->get_res_value(daily_new_player_checkin_flag);
	cli_out.reward_stat = p->res_mgr->get_res_value(forever_new_player_checkin_reward_stat);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 新手签到
 */
int cli_new_player_checkin(Player *p, Cmessage *c_in)
{
	cli_new_player_checkin_in *p_in = P_IN;

	if (!p_in->checkin_id || p_in->checkin_id > 7) {
		T_KWARN_LOG(p->user_id, "new player checkin input err\t[checkin_id=%u]", p_in->checkin_id);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_input_arg_err, 1);
	}

	uint32_t login_day = p->res_mgr->get_res_value(forever_player_login_day);
	//uint32_t today_flag = p->res_mgr->get_res_value(daily_new_player_checkin_flag);
	uint32_t reward_stat = p->res_mgr->get_res_value(forever_new_player_checkin_reward_stat);

	if (login_day < p_in->checkin_id) {
		T_KWARN_LOG(p->user_id, "new player checkin login day not match\t[login_day=%u, checkin_id=%u]", login_day, p_in->checkin_id);
		return p->send_to_self_error(p->wait_cmd, cli_new_player_checkin_login_day_not_match_err, 1);
	}

	/*
	if (today_flag) {
		T_KWARN_LOG(p->user_id, "new player checkin today already checkin");
		return p->send_to_self_error(p->wait_cmd, cli_new_player_already_checkin_err, 1);
	}*/

	if (test_bit_on(reward_stat, p_in->checkin_id)) {
		T_KWARN_LOG(p->user_id, "new player checkin rewawrd already gotted\t[reward_stat=%u, checkin_id=%u]", reward_stat, p_in->checkin_id);
		return p->send_to_self_error(p->wait_cmd, cli_new_player_already_checkin_err, 1);
	}

	//发送奖励
	General::get_new_player_checkin_reward(p, p_in->checkin_id);

	//设置限制
	reward_stat = set_bit_on(reward_stat, p_in->checkin_id);
	p->res_mgr->set_res_value(forever_new_player_checkin_reward_stat, reward_stat);

	T_KDEBUG_LOG(p->user_id, "NEW PLAYER CHECKIN\t[checkin_id=%u]", p_in->checkin_id);

	cli_new_player_checkin_out cli_out;
	cli_out.checkin_id = p_in->checkin_id;
	cli_out.reward_stat = reward_stat;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 拉取等级礼包面板信息
 */
int cli_get_level_gift_panel_info(Player *p, Cmessage *c_in)
{
	uint32_t gift_stat = p->res_mgr->get_res_value(forever_level_gift_stat);

	cli_get_level_gift_panel_info_out cli_out;
	cli_out.gift_stat = gift_stat;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 领取等级礼包
 */
int cli_get_level_gift(Player *p, Cmessage *c_in)
{
	cli_get_level_gift_in *p_in = P_IN;

	if (!p_in->gift_id || p_in->gift_id > 9) {
		return cli_invalid_input_arg_err;
	}

	uint32_t reward_lv[9] = {5, 10, 15, 18, 20, 25, 30, 35, 40};
	uint32_t need_lv = reward_lv[p_in->gift_id - 1];
	if (p->lv < need_lv) {
		return cli_level_gift_role_lv_not_enough_err;
	}

	uint32_t gift_stat = p->res_mgr->get_res_value(forever_level_gift_stat);
	if (test_bit_on(gift_stat, p_in->gift_id)) {
		T_KWARN_LOG(p->user_id, "already gotted the level gift\t[gift_id=%u]", p_in->gift_id);
		return cli_level_gift_already_gotted_err;
	}

	//发送奖励
	General::get_level_gift(p, p_in->gift_id);

	//设置限制
	gift_stat = set_bit_on(gift_stat, p_in->gift_id);
	p->res_mgr->set_res_value(forever_level_gift_stat, gift_stat);

	T_KDEBUG_LOG(p->user_id, "GET LEVEL GIFT\t[gift_id=%u]", p_in->gift_id);

	cli_get_level_gift_out cli_out;
	cli_out.gift_id = p_in->gift_id;
	cli_out.gift_stat = gift_stat;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 拉取钻石礼包面板信息
 */
int cli_get_diamond_gift_panel_info(Player *p, Cmessage *c_in)
{
	uint32_t cost_diamond = p->res_mgr->get_res_value(forever_total_cost_diamond);
	uint32_t gift_stat = p->res_mgr->get_res_value(forever_diamond_gift_stat);

	cli_get_diamond_gift_panel_info_out cli_out;
	cli_out.cost_diamond = cost_diamond;
	cli_out.gift_stat = gift_stat;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 领取钻石礼包
 */
int cli_get_diamond_gift(Player *p, Cmessage *c_in)
{
	cli_get_diamond_gift_in *p_in = P_IN;
	if (!p_in->gift_id || p_in->gift_id > 7) {
		return cli_invalid_input_arg_err;
	}

	uint32_t cost_diamond = p->res_mgr->get_res_value(forever_total_cost_diamond);
	uint32_t need_cost_diamond[7] = {200, 500, 1000, 1500, 2000, 3000, 5000};
	uint32_t dst_diamond = need_cost_diamond[p_in->gift_id - 1];
	if (cost_diamond < dst_diamond) {
		T_KWARN_LOG(p->user_id, "get diamond gift cost diamond not enough\t[cost_diamond=%u, dst_diamond=%u]", cost_diamond, dst_diamond);
		return cli_diamond_gift_cost_diamond_not_enough_err;
	}

	uint32_t gift_stat = p->res_mgr->get_res_value(forever_diamond_gift_stat);
	if (test_bit_on(gift_stat, p_in->gift_id)) {
		T_KWARN_LOG(p->user_id, "diamond gift already gotted\t[gift_id=%u]", p_in->gift_id);
		return cli_diamond_gift_already_gotted_err;
	}

	//发送奖励
	General::get_diamond_gift(p, p_in->gift_id);

	//设置限制
	gift_stat = set_bit_on(gift_stat, p_in->gift_id);
	p->res_mgr->set_res_value(forever_diamond_gift_stat, gift_stat);

	T_KDEBUG_LOG(p->user_id, "GET DIAMOND GIFT\t[gift_id=%u]", p_in->gift_id);

	cli_get_diamond_gift_out cli_out;
	cli_out.gift_id = p_in->gift_id;
	cli_out.gift_stat = gift_stat;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 领取每日体力
 */
int cli_get_daily_energy_gift(Player *p, Cmessage *c_in)
{
	uint32_t now_hour = utils_mgr.get_hour(time(0));
	uint32_t time_zone = 0;
	if (now_hour >= 11 && now_hour <= 14) {
		time_zone = 1;
	} else if (now_hour >= 17 && now_hour <= 19) {
		time_zone = 2;
	} else if (now_hour >= 21 && now_hour <= 23) {
		time_zone = 3;
	}

	if (!time_zone) {
		T_KWARN_LOG(p->user_id, "get daily energy gift hour err\t[now_hour=%u]", now_hour);
		return cli_get_daily_energy_gift_time_err;
	}

	uint32_t daily_stat = p->res_mgr->get_res_value(daily_energy_gift_stat);
	if (test_bit_on(daily_stat, time_zone)) {
		T_KWARN_LOG(p->user_id, "daily energy gift already gotted\t[now_hour=%u]", now_hour);
		return cli_energy_gift_already_gotted_err;
	}

	//领取体力
	p->chg_energy(50);

	//设置限制
	daily_stat = set_bit_on(daily_stat, time_zone);
	p->res_mgr->set_res_value(daily_energy_gift_stat, daily_stat);

	T_KDEBUG_LOG(p->user_id, "GET DAILY ENERGY GIFT\t[now_hour=%u]", now_hour);

	cli_get_daily_energy_gift_out cli_out;
	cli_out.stat = daily_stat;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}
