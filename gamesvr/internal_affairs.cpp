/*
 * =====================================================================================
 *
 *  @file  internal_affairs.cpp 
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

#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"
#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"

#include "internal_affairs.hpp"
#include "player.hpp"
#include "hero.hpp"
#include "item.hpp"
#include "dbroute.hpp"
#include "utils.hpp"

using namespace std;
using namespace project;

InternalAffairsXmlManager internal_affairs_xml_mgr;
InternalAffairsRewardXmlManager internal_affairs_reward_xml_mgr;
InternalAffairsLevelXmlManager internal_affairs_level_xml_mgr;

/********************************************************************************/
/*						InternalAffairs Class									*/
/********************************************************************************/
InternalAffairs::InternalAffairs(Player *p) : owner(p)
{

}

InternalAffairs::~InternalAffairs()
{

}

int
InternalAffairs::init_internal_affairs_exp_info_from_db(uint32_t affairs_lv, uint32_t affairs_exp)
{
	lv = affairs_lv;
	exp = affairs_exp;

	return 0;
}

int
InternalAffairs::init_internal_affairs_list(db_get_affairs_list_out *p_in)
{
	for (uint32_t i = 0; i < p_in->affairs_list.size(); i++) {
		db_affairs_info_t *p_info = &(p_in->affairs_list[i]);
		internal_affairs_info_t info = {};
		info.type = p_info->type;
		info.start_tm = p_info->start_tm;
		info.hero_id = p_info->hero_id;
		info.complete_tm = p_info->complete_tm;
		if (p_info->hero_id) {
			Hero *p_hero = owner->hero_mgr->get_hero(p_info->hero_id);
			if (p_hero) {
				p_hero->is_in_affairs = 1;
			}
		}

		T_KTRACE_LOG(owner->user_id, "init internal affairs list\t[%u %u %u %u]", info.type, info.start_tm, info.hero_id, info.complete_tm);

		internal_affairs_map.insert(InternalAffairsMap::value_type(p_info->type, info));
	}

	return 0;
}

const internal_affairs_info_t*
InternalAffairs::get_internal_affairs_info(uint32_t type)
{
	InternalAffairsMap::const_iterator it = internal_affairs_map.find(type);
	if (it != internal_affairs_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
InternalAffairs::get_level_up_exp()
{
	const internal_affairs_level_xml_info_t *p_xml_info = internal_affairs_level_xml_mgr.get_internal_affairs_level_xml_info(this->lv);
	if (!p_xml_info) {
		return -1;
	}	

	return p_xml_info->exp;
}

int
InternalAffairs::add_exp(uint32_t add_value)
{
	if (!add_value) {
		return -1;
	}
	if (lv >= MAX_INTERNAL_AFFAIRS_LEVEL) {
		return -1;
	}

	//uint32_t old_lv = lv;
	exp += add_value;

	uint32_t level_up_exp = get_level_up_exp();
	while (exp >= level_up_exp) {
		exp -= level_up_exp;
		lv++;
		level_up_exp = get_level_up_exp();
		if (lv >= MAX_INTERNAL_AFFAIRS_LEVEL) {
			exp = level_up_exp;
			break;
		}
	}

	//更新DB
	db_update_affairs_exp_info_in db_in;
	db_in.affairs_lv = lv;
	db_in.affairs_exp = exp;
	send_msg_to_dbroute(0, db_update_affairs_exp_info_cmd, &db_in, owner->user_id);

	//通知前端
	cli_internal_affairs_exp_change_noti_out noti_out;
	noti_out.lv = lv;
	noti_out.exp = exp;
	noti_out.add_exp = add_value;
	noti_out.levelup_exp = get_level_up_exp();
	owner->send_to_self(cli_internal_affairs_exp_change_noti_cmd, &noti_out, 0);

	T_KDEBUG_LOG(owner->user_id, "ADD INTERNAL AFFAIRS EXP\t[%u %u %u]", lv, exp, add_value);

	return 0;
}

int
InternalAffairs::calc_internal_affairs_role_exp()
{
	if (owner->res_mgr->get_res_value(daily_internal_affairs_role_exp_flag)) {
		return 0;
	}
	uint32_t yesterday_affairs_cnt = owner->res_mgr->get_res_value(daily_yesterday_internal_affairs_cnt);

	uint32_t role_exp = yesterday_affairs_cnt * 1000 + lv * 3000;

	return role_exp;
}

int
InternalAffairs::get_internal_affairs_role_exp()
{
	if (owner->res_mgr->get_res_value(daily_internal_affairs_role_exp_flag)) {
		return cli_internal_affairs_role_exp_already_gotted_err;
	}

	Hero *p_hero = owner->hero_mgr->get_hero(owner->role_id);
	if (p_hero) {
		uint32_t add_exp = calc_internal_affairs_role_exp();
		p_hero->add_exp(add_exp);
		owner->res_mgr->set_res_value(daily_internal_affairs_role_exp_flag, 1);
	}

	return 0;
}

int 
InternalAffairs::add_internal_affairs(uint32_t type, uint32_t hero_id)
{
	InternalAffairsMap::const_iterator it = internal_affairs_map.find(type);
	if (it != internal_affairs_map.end()) {
		return -1;
	}

	Hero *p_hero = owner->hero_mgr->get_hero(hero_id);
	if (!p_hero || p_hero->is_in_affairs) {
		return -1;
	}
	p_hero->is_in_affairs = 1;

	uint32_t now_sec = get_now_tv()->tv_sec;

	//更新缓存
	internal_affairs_info_t info = {};
	info.type = type;
	info.start_tm = now_sec;
	info.hero_id = hero_id;
	internal_affairs_map.insert(InternalAffairsMap::value_type(type, info));

	//更新DB
	db_add_affairs_info_in db_in;
	db_in.affairs_info.type = type;
	db_in.affairs_info.start_tm = now_sec;
	db_in.affairs_info.hero_id = hero_id;
	send_msg_to_dbroute(0, db_add_affairs_info_cmd, &db_in, owner->user_id);
	
	return 0;
}

int
InternalAffairs::join_internal_affairs(uint32_t type, uint32_t hero_id, uint32_t &left_tm)
{
	const internal_affairs_xml_info_t *p_xml_info = internal_affairs_xml_mgr.get_internal_affairs_xml_info(type);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "invalid internal affairs type\t[type=%u]", type);
		return cli_invalid_internal_affairs_type_err;
	}

	if (lv < p_xml_info->unlock_lv) {
		T_KWARN_LOG(owner->user_id, "internal affairs type not unlock\t[type=%u, lv=%u, unlock_lv=%u]", type, lv, p_xml_info->unlock_lv);
		return cli_internal_affairs_type_not_unlock_err;
	}

	if (owner->energy < p_xml_info->energy) {
		T_KWARN_LOG(owner->user_id, "internal affairs need energy not enough\t[type=%u, energy=%u, need_energy=%u]", type, owner->energy, p_xml_info->energy);
		return cli_not_enough_energy_err;
	}
	
	const internal_affairs_info_t *p_info = get_internal_affairs_info(type);
	if (p_info) {
		T_KWARN_LOG(owner->user_id, "already join the internal affairs\t[type=%u]", type);
		return cli_internal_affairs_already_joined_err;
	}

	Hero *p_hero = owner->hero_mgr->get_hero(hero_id);
	if (!p_hero) {
		T_KWARN_LOG(owner->user_id, "invlaid hero id\t[hero_id=%u]", hero_id);
		return cli_invalid_hero_err;
	}

	if (p_hero->is_in_affairs) {
		T_KWARN_LOG(owner->user_id, "hero already in affairs\t[hero_id=%u]", hero_id);
		return cli_hero_already_in_internal_affairs_err;
	}

	//加入内政
	add_internal_affairs(type, hero_id);

	left_tm = p_xml_info->time;//TODO
	//left_tm = 10;

	return 0;
}

int
InternalAffairs::set_complete_time(uint32_t type)
{
	InternalAffairsMap::iterator it = internal_affairs_map.find(type);
	if (it == internal_affairs_map.end()) {
		return -1;
	}

	uint32_t now_sec = get_now_tv()->tv_sec;
	
	//更新缓存
	it->second.complete_tm = now_sec;

	//更新DB
	db_set_affairs_complete_time_in db_in;
	db_in.type = type;
	db_in.complete_tm = now_sec;
	send_msg_to_dbroute(0, db_set_affairs_complete_time_cmd, &db_in, owner->user_id);

	return 0;
}

int
InternalAffairs::add_internal_affairs_reward(uint32_t type)
{
	const internal_affairs_reward_item_xml_info_t *p_xml_info = internal_affairs_reward_xml_mgr.random_one_reward(type);
	if (p_xml_info) {
		owner->items_mgr->add_reward(p_xml_info->item_id, p_xml_info->item_cnt);
	}

	return 0;
}

int
InternalAffairs::complete_internal_affairs(uint32_t type)
{
	const internal_affairs_xml_info_t *p_xml_info = internal_affairs_xml_mgr.get_internal_affairs_xml_info(type);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "invalid internal affairs type\t[type=%u]", type);
		return cli_invalid_internal_affairs_type_err;
	}

	if (owner->energy < p_xml_info->energy) {
		T_KWARN_LOG(owner->user_id, "internal affairs need energy not enough\t[type=%u, energy=%u, need_energy=%u]", type, owner->energy, p_xml_info->energy);
		return cli_not_enough_energy_err;
	}

	const internal_affairs_level_xml_info_t *p_level_xml_info = internal_affairs_level_xml_mgr.get_internal_affairs_level_xml_info(lv);
	if (!p_level_xml_info) {
		T_KWARN_LOG(owner->user_id, "internal affairs level invalid\t[lv=%u]", lv);
		return cli_internal_affairs_level_invalid_err;
	}

	if (owner->golds < p_level_xml_info->cost_golds) {
		T_KWARN_LOG(owner->user_id, "complete internal affairs need golds not enough\t[golds=%u, need_golds=%u]", owner->golds, p_level_xml_info->cost_golds);
		return cli_not_enough_golds_err;
	}

	const internal_affairs_info_t *p_info = get_internal_affairs_info(type);
	if (!p_info) {
		T_KWARN_LOG(owner->user_id, "complete internal affairs err\t[type=%u]", type);
		return cli_internal_affairs_not_join_err;
	}

	uint32_t now_sec = get_now_tv()->tv_sec;
	//if (p_info->start_tm + p_xml_info->time > now_sec) {
	if (p_info->start_tm + 10 > now_sec) {
		T_KWARN_LOG(owner->user_id, "internal affairs already in cd\t[start_tm=%u, now_sec=%u]", p_info->start_tm, now_sec);
		return cli_internal_affairs_already_in_cd_err;
	}

	if (p_info->complete_tm > 0) {
		T_KWARN_LOG(owner->user_id, "internal affairs already completed err, type=%u", type);
		return cli_internal_affairs_already_completed_err;
	}

	//扣除体力
	owner->chg_energy(0 - p_xml_info->energy);

	//扣除金币
	owner->chg_golds(0 - p_level_xml_info->cost_golds);

	//设置完成时间
	set_complete_time(type);

	//增加内政经验
	add_exp(p_xml_info->exp);

	//添加奖励
	add_internal_affairs_reward(type);

	//检查任务
	owner->task_mgr->check_task(em_task_type_internal_affairs);

	return 0;
}

void 
InternalAffairs::pack_client_internal_affairs_info(cli_get_internal_affairs_panel_info_out &cli_out)
{
	cli_out.lv = lv;
	cli_out.exp = exp;
	cli_out.levelup_exp = get_level_up_exp();
	cli_out.role_exp = calc_internal_affairs_role_exp();
	cli_out.role_exp_flag = owner->res_mgr->get_res_value(daily_internal_affairs_role_exp_flag);

	std::vector<uint32_t> week_affairs;
	internal_affairs_xml_mgr.get_cur_week_interal_affairs(week_affairs);

	uint32_t now_sec = get_now_tv()->tv_sec;
	for (uint32_t i = 0; i < week_affairs.size(); i++) {
		uint32_t type = week_affairs[i];
		const internal_affairs_xml_info_t *p_xml_info = internal_affairs_xml_mgr.get_internal_affairs_xml_info(type);
		const internal_affairs_level_xml_info_t *p_level_xml_info = internal_affairs_level_xml_mgr.get_internal_affairs_level_xml_info(lv);
		if (!p_xml_info || !p_level_xml_info) {
			continue;
		}
		cli_internal_affairs_info_t info;
		info.type = type;
		info.energy = p_xml_info->energy;
		info.golds = p_level_xml_info->cost_golds;
		info.is_unlock = lv < p_xml_info->unlock_lv ? 0 : 1;

		const internal_affairs_info_t *p_info = get_internal_affairs_info(type);
		if (p_info) {
			uint32_t end_tm = p_info->start_tm + p_xml_info->time;//TODO
			//uint32_t end_tm = p_info->start_tm + 10;
			info.start_tm = p_info->start_tm;
			info.hero_id = p_info->hero_id;
			info.left_tm = end_tm > now_sec ? (end_tm - now_sec) : 0;
			info.is_completed = p_info->complete_tm ? 1 : 0;
		}

		T_KTRACE_LOG(owner->user_id, "pack client internal affairs info\t[%u %u %u %u %u %u]", 
				info.type, info.start_tm, info.hero_id, info.left_tm, info.is_completed, info.is_unlock);

		cli_out.affairs_list.push_back(info);
	}
}


/********************************************************************************/
/*						InternalAffairsXmlManager Class							*/
/********************************************************************************/
InternalAffairsXmlManager::InternalAffairsXmlManager()
{
	internal_affairs_xml_map.clear();
}

InternalAffairsXmlManager::~InternalAffairsXmlManager()
{
	
}

const internal_affairs_xml_info_t *
InternalAffairsXmlManager::get_internal_affairs_xml_info(uint32_t type)
{
	InternalAffairsXmlMap::const_iterator it = internal_affairs_xml_map.find(type);
	if (it != internal_affairs_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
InternalAffairsXmlManager::get_cur_week_interal_affairs(vector<uint32_t> &vec)
{
	uint32_t cur_week = utils_mgr.get_week_day(time(0));
	if (cur_week == 0) {
		cur_week = 7;
	}
	InternalAffairsXmlMap::const_iterator it = internal_affairs_xml_map.begin();
	for (; it != internal_affairs_xml_map.end(); ++it) {
		const internal_affairs_xml_info_t *p_xml_info = &(it->second);
		if (test_bit_on(p_xml_info->week, cur_week)) {
			vec.push_back(p_xml_info->type);
		}
	}

	return 0;
}

int
InternalAffairsXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_internal_affairs_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

int
InternalAffairsXmlManager::load_internal_affairs_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("affairs"))) {
			uint32_t type = 0;
			get_xml_prop(type, cur, "type");
			InternalAffairsXmlMap::iterator it = internal_affairs_xml_map.find(type);
			if (it != internal_affairs_xml_map.end()) {
				ERROR_LOG("load internal affairs xml info err, type=%u", type);
				return -1;
			}

			internal_affairs_xml_info_t info = {};
			info.type = type;
			get_xml_prop(info.energy, cur, "energy");
			get_xml_prop(info.time, cur, "time");
			get_xml_prop(info.exp, cur, "exp");
			uint32_t week = 0;
			get_xml_prop(week, cur, "week");
			get_xml_prop(info.unlock_lv, cur, "unlock_lv");

			info.time *= 60;
			uint32_t i = 0;
			while ((i = week % 10) > 0) {
				info.week = set_bit_on(info.week, i);
				week /= 10;
			}

			TRACE_LOG("load internal affairs xml info\t[%u %u %u %u %u %u]", 
					info.type, info.energy, info.time, info.exp, info.week, info.unlock_lv);

			internal_affairs_xml_map.insert(InternalAffairsXmlMap::value_type(type, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*						InternalAffairsRewardXmlManager Class					*/
/********************************************************************************/
InternalAffairsRewardXmlManager::InternalAffairsRewardXmlManager()
{
	internal_affairs_reward_xml_map.clear();
}

InternalAffairsRewardXmlManager::~InternalAffairsRewardXmlManager()
{

}

const internal_affairs_reward_item_xml_info_t *
InternalAffairsRewardXmlManager::random_one_reward(uint32_t type)
{
	InternalAffairsRewardXmlMap::iterator it = internal_affairs_reward_xml_map.find(type);
	if (it != internal_affairs_reward_xml_map.end()) {
		if (it->second.total_weight) {
			uint32_t r = rand() % it->second.total_weight;
			for (uint32_t i = 0; i < it->second.rewards.size(); i++) {
				internal_affairs_reward_item_xml_info_t *p_item_info = &(it->second.rewards[i]);
				if (p_item_info->weight < r) {
					return p_item_info;
				}
			}
		}	
	}

	return 0;
}

int
InternalAffairsRewardXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_internal_affairs_reward_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

int
InternalAffairsRewardXmlManager::load_internal_affairs_reward_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("affairs"))) {
			uint32_t type = 0;
			get_xml_prop(type, cur, "type");
			internal_affairs_reward_item_xml_info_t sub_info = {};
			get_xml_prop(sub_info.item_id, cur, "item_id");
			get_xml_prop(sub_info.item_cnt, cur, "item_cnt");
			get_xml_prop(sub_info.weight, cur, "weight");

			InternalAffairsRewardXmlMap::iterator it = internal_affairs_reward_xml_map.find(type);
			if (it != internal_affairs_reward_xml_map.end()) {
				it->second.rewards.push_back(sub_info);
				it->second.total_weight += sub_info.weight;
			} else {
				internal_affairs_reward_xml_info_t info = {};
				info.type = type;
				info.rewards.push_back(sub_info);
				info.total_weight = sub_info.weight;
				
				internal_affairs_reward_xml_map.insert(InternalAffairsRewardXmlMap::value_type(type, info));
			}

			TRACE_LOG("load internal affairs reward xml info\t[%u %u %u %u]", type, sub_info.item_id, sub_info.item_cnt, sub_info.weight);
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*						InternalAffairsLevelXmlManager Class					*/
/********************************************************************************/
InternalAffairsLevelXmlManager::InternalAffairsLevelXmlManager()
{
	internal_affairs_level_xml_map.clear();
}

InternalAffairsLevelXmlManager::~InternalAffairsLevelXmlManager()
{

}

const internal_affairs_level_xml_info_t*
InternalAffairsLevelXmlManager::get_internal_affairs_level_xml_info(uint32_t lv)
{
	InternalAffairsLevelXmlMap::const_iterator it = internal_affairs_level_xml_map.find(lv);
	if (it != internal_affairs_level_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int 
InternalAffairsLevelXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_internal_affairs_level_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

int
InternalAffairsLevelXmlManager::load_internal_affairs_level_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("affairs"))) {
			uint32_t lv = 0;
			get_xml_prop(lv, cur, "lv");
			InternalAffairsLevelXmlMap::iterator it = internal_affairs_level_xml_map.find(lv);
			if (it != internal_affairs_level_xml_map.end()) {
				ERROR_LOG("load internal affairs level xml info err, lv=%u", lv);
				return -1;
			}

			internal_affairs_level_xml_info_t info = {};
			info.lv = 0;
			get_xml_prop(info.exp, cur, "exp");
			get_xml_prop(info.cost_golds, cur, "cost_golds");

			TRACE_LOG("load internal affairs level xml info\t[%u %u %u]", info.lv, info.exp, info.cost_golds);

			internal_affairs_level_xml_map.insert(InternalAffairsLevelXmlMap::value_type(lv, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								Client Request									*/
/*******a*************************************************************************/
/* @brief 拉取内政面板信息
 */
int cli_get_internal_affairs_panel_info(Player *p, Cmessage *c_in)
{
	cli_get_internal_affairs_panel_info_out cli_out;
	
	p->internal_affairs->pack_client_internal_affairs_info(cli_out);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 执行内政
 */
int cli_join_internal_affairs(Player *p, Cmessage *c_in)
{
	cli_join_internal_affairs_in *p_in = P_IN;

	uint32_t left_tm = 0;
	int ret = p->internal_affairs->join_internal_affairs(p_in->type, p_in->hero_id, left_tm);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_join_internal_affairs_out cli_out;
	cli_out.type = p_in->type;
	cli_out.hero_id = p_in->hero_id;
	cli_out.left_tm = left_tm;

	T_KDEBUG_LOG(p->user_id, "JOIN INTERNAL AFFAIRS\t[type=%u, hero_id=%u, left_tm=%u]", p_in->type, p_in->hero_id, left_tm);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 完成内政
 */
int cli_complete_internal_affairs(Player *p, Cmessage *c_in)
{
	cli_complete_internal_affairs_in *p_in = P_IN;

	int ret = p->internal_affairs->complete_internal_affairs(p_in->type);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_complete_internal_affairs_out cli_out;
	cli_out.type = p_in->type;

	T_KDEBUG_LOG(p->user_id, "COMPLETE INTERNAL AFFAIRS\t[type=%u]", p_in->type);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 领取主公经验
 */
int cli_get_internal_affairs_role_exp(Player *p, Cmessage *c_in)
{
	int ret = p->internal_affairs->get_internal_affairs_role_exp();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_get_internal_affairs_role_exp_out cli_out;
	cli_out.role_exp = 0;

	T_KDEBUG_LOG(p->user_id, "GET INTERNAL AFFAIRS ROLE EXP");

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}


/********************************************************************************/
/*								DB Return										*/
/********************************************************************************/
/* @brief 拉取内政列表
 */
int db_get_affairs_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);

	db_get_affairs_list_out *p_in = P_IN;

	if (p->wait_cmd == cli_proto_login_cmd) {
		p->internal_affairs->init_internal_affairs_list(p_in);
		
		//拉取成就列表
		return send_msg_to_dbroute(p, db_get_achievement_list_cmd, 0, p->user_id);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}
