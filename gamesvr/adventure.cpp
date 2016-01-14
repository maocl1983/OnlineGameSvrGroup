/*
 * =====================================================================================
 *
 *  @file  adventure.cpp 
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

#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"
#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"

#include "global_data.hpp"
#include "adventure.hpp"
#include "player.hpp"
#include "dbroute.hpp"

using namespace std;
using namespace project;

//AdventureXmlManager adventure_xml_mgr;
//AdventureSelectXmlManager adventure_select_xml_mgr;
//AdventureItemXmlManager adventure_item_xml_mgr;

/********************************************************************************/
/*						AdventureManager Class									*/
/********************************************************************************/
AdventureManager::AdventureManager(Player *p) : owner(p)
{
	adventure_map.clear();
}

AdventureManager::~AdventureManager()
{
	adventure_map.clear();
}

int
AdventureManager::init_adventure_list(db_get_adventure_list_out *p_in)
{
	for (uint32_t i = 0; i < p_in->adventure_list.size(); i++) {
		db_adventure_info_t *p_info = &(p_in->adventure_list[i]);
		adventure_info_t info = {};
		info.time = p_info->time;
		info.adventure_id = p_info->adventure_id;
		info.item_id = p_info->item_id;
		info.item_cnt = p_info->item_cnt;

		T_KTRACE_LOG(owner->user_id, "init adventure list\t[%u %u %u %u]", info.time, info.adventure_id, info.item_id, info.item_cnt);

		adventure_map.insert(AdventureMap::value_type(info.time, info));
	}

	return 0;
}

const adventure_info_t *
AdventureManager::get_adventure(uint32_t time)
{
	AdventureMap::iterator it = adventure_map.find(time);
	if (it == adventure_map.end()) {
		return 0;
	}

	return &(it->second);
}

int
AdventureManager::trigger_adventure()
{
	uint32_t type = (rand() % 100 < 80) ? 1 : 2;
	uint32_t adventure_id = adventure_xml_mgr->random_one_adventure(type);
	if (adventure_id) {
		add_adventure(adventure_id);
	}

	return 0;
}

int
AdventureManager::del_expire_adventure()
{
	uint32_t now_sec = get_now_tv()->tv_sec;
	AdventureMap::iterator it = adventure_map.begin();
	while (it != adventure_map.end()) {
		adventure_info_t *p_info = &(it++->second);
		const adventure_xml_info_t *p_xml_info = adventure_xml_mgr->get_adventure_xml_info(p_info->adventure_id);
		if (p_xml_info) {
			if (p_info->time + p_xml_info->time < now_sec) {//expire
				del_adventure(p_info->time);
			}
		}
	}

	return 0;
}

int
AdventureManager::add_adventure(uint32_t adventure_id)
{
	const adventure_xml_info_t *p_xml_info = adventure_xml_mgr->get_adventure_xml_info(adventure_id);
	if (!p_xml_info) {
		return 0;
	}
	const item_info_t *p_item_xml_info = adventure_item_xml_mgr->random_one_reward(p_xml_info->select_1);
	if (!p_item_xml_info) {
		return 0;
	}

	uint32_t now_sec = get_now_tv()->tv_sec;
	AdventureMap::iterator it;
	while ((it = adventure_map.find(now_sec)) != adventure_map.end()) {
		now_sec++;
	}

	adventure_info_t info;
	info.time = now_sec;
	info.adventure_id = adventure_id;
	info.item_id = p_item_xml_info->id;
	info.item_cnt = p_item_xml_info->cnt;
	adventure_map.insert(AdventureMap::value_type(now_sec, info));

	//更新DB
	db_add_adventure_in db_in;
	db_in.time = now_sec;
	db_in.adventure_id = adventure_id;
	db_in.item_id = p_item_xml_info->id;
	db_in.item_cnt = p_item_xml_info->cnt;
	send_msg_to_dbroute(0, db_add_adventure_cmd, &db_in, owner->user_id);

	//通知前端
	cli_adventure_trigger_noti_out noti_out;
	noti_out.adventure_info.time = now_sec;
	noti_out.adventure_info.adventure_id = adventure_id;
	noti_out.adventure_info.item_id = p_item_xml_info->id;
	noti_out.adventure_info.item_cnt = p_item_xml_info->cnt;
	noti_out.adventure_info.left_time = p_xml_info->time;
	owner->send_to_self(cli_adventure_trigger_noti_cmd, &noti_out, 0);

	T_KDEBUG_LOG(owner->user_id, "ADD ADVENTURE\t[adventure_id=%u, time=%u, item_id=%u, item_cnt=%u]", 
			adventure_id, now_sec, p_item_xml_info->id, p_item_xml_info->cnt);

	return 0;
}

int
AdventureManager::del_adventure(uint32_t time)
{
	AdventureMap::iterator it = adventure_map.find(time);
	if (it == adventure_map.end()) {
		return 0;
	}

	adventure_map.erase(time);

	//更新DB
	db_del_adventure_in db_in;
	db_in.time = time;
	send_msg_to_dbroute(0, db_del_adventure_cmd, &db_in, owner->user_id);

	T_KDEBUG_LOG(owner->user_id, "DEL ADVENTURE\t[time=%u]", time);

	return 0;
}

int
AdventureManager::calc_daily_max_complete_tms()
{
	return 30;
}

int
AdventureManager::complete_adventure(uint32_t time, uint32_t type)
{
	if (!type || type > 2) {
		T_KWARN_LOG(owner->user_id, "complete adventure input err\t[type=%u]", type);
		return cli_invalid_input_arg_err;
	}
	const adventure_info_t *p_info = get_adventure(time);
	if (!p_info) {
		T_KWARN_LOG(owner->user_id, "complete adventure time err\t[time=%u]", time);
		return cli_adventure_not_exist_err;
	}

	const adventure_xml_info_t *p_xml_info = adventure_xml_mgr->get_adventure_xml_info(p_info->adventure_id);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "invalid adventure id\t[adventure_id=%u]", p_info->adventure_id);
		return cli_invalid_adventure_id_err;
	}

	//检查每日完成次数
	uint32_t daily_tms = owner->res_mgr->get_res_value(daily_adventure_complete_tms);
	uint32_t max_tms = calc_daily_max_complete_tms();
	if (daily_tms >= max_tms) {
		return cli_adventure_complete_tms_not_enough_err;
	}

	uint32_t now_sec = get_now_tv()->tv_sec;
	if (p_xml_info->type == 1) {
		if (type == 2) {//延迟型奇遇非立即完成
			if (p_xml_info->time + time > now_sec) {
				T_KWARN_LOG(owner->user_id, "adventure already in cd\t[time=%u, adventure_id=%u]", time, p_info->adventure_id);
				return cli_adventure_already_in_cd_err;
			}	
		}
	} else if (p_xml_info->type == 2) {//限时型奇遇检测是否已过期
		if (time + p_xml_info->time < now_sec) {//已过期
			T_KWARN_LOG(owner->user_id, "adventure has expired\t[time=%u, adventure_id=%u]", time, p_info->adventure_id);
			return cli_adventure_has_expired_err;
		}
	}

	uint32_t select_id = 0;
	if (type == 1) {
		select_id = p_xml_info->select_1;
	} else {
		select_id = p_xml_info->select_2;
	}

	const adventure_select_xml_info_t *p_xml_info_2 = adventure_select_xml_mgr->get_adventure_select_xml_info(select_id);
	if (!p_xml_info_2) {
		T_KWARN_LOG(owner->user_id, "adventure select id not exist\t[adventure_id=%u, select_id=%u]", p_info->adventure_id, select_id);
		return cli_invalid_adventure_select_id_err;
	}

	if (owner->diamond< p_xml_info_2->cost_diamond) {
		T_KWARN_LOG(owner->user_id, "complete adventure need diamond not enough\t[diamond=%u, need_diamond=%u]", owner->diamond, p_xml_info_2->cost_diamond);
		return cli_not_enough_diamond_err;
	}

	if (owner->adventure < p_xml_info_2->cost_adventure) {
		T_KWARN_LOG(owner->user_id, "complete adventure need adventure not enough\t[cnt=%u, need_cnt=%u]", 
				owner->adventure, p_xml_info_2->cost_adventure);
		return cli_not_enough_adventure_err;
	}

	//扣除金币
	if (p_xml_info_2->cost_diamond) {
		owner->chg_diamond(0 - p_xml_info_2->cost_diamond);
	}

	//扣除奇遇点
	if (p_xml_info_2->cost_adventure) {
		owner->chg_adventure(0 - p_xml_info_2->cost_adventure);
	}

	//添加主角经验
	if (p_xml_info_2->role_exp) {
		owner->add_role_exp(p_xml_info_2->role_exp);
	}

	//奖励物品
	cli_send_get_common_bonus_noti_out noti_out;
	if (type == 1) {
		owner->items_mgr->add_reward(p_info->item_id, p_info->item_cnt);
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, p_info->item_id, p_info->item_cnt); 
	} else {
		owner->items_mgr->add_reward(p_info->item_id, p_info->item_cnt / 2);
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, p_info->item_id, p_info->item_cnt / 2); 
	}
	owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);
	
	//限制次数+1
	daily_tms++;
	owner->res_mgr->set_res_value(daily_adventure_complete_tms, daily_tms);

	//清除奇遇信息
	del_adventure(time);

	//检查任务
	owner->task_mgr->check_task(em_task_type_adventure);

	//检查成就
	owner->achievement_mgr->check_achievement(em_achievement_type_adventure);

	return 0;
}

void 
AdventureManager::pack_client_adventure_list(cli_get_adventure_list_out &out)
{
	uint32_t now_sec = time(0);
	AdventureMap::iterator it = adventure_map.begin();
	for (; it != adventure_map.end(); ++it) {
		adventure_info_t *p_info = &(it->second);
		const adventure_xml_info_t *p_xml_info = adventure_xml_mgr->get_adventure_xml_info(p_info->adventure_id);
		if (!p_xml_info) {
			continue;
		}
		uint32_t complete_tm = p_info->time + p_xml_info->time;
		cli_adventure_info_t info;
		info.time = p_info->time;
		info.adventure_id = p_info->adventure_id;
		info.item_id = p_info->item_id;
		info.item_cnt = p_info->item_cnt;
		info.left_time = now_sec < complete_tm  ? complete_tm - now_sec : 0;
		out.adventure_list.push_back(info);

		T_KTRACE_LOG(owner->user_id, "pack client adventure list\t[%u %u %u %u %u]", 
				info.time, info.adventure_id, info.item_id, info.item_cnt, info.left_time );
	}
}


/********************************************************************************/
/*						AdventureXmlManager Class								*/
/********************************************************************************/
AdventureXmlManager::AdventureXmlManager()
{

}

AdventureXmlManager::~AdventureXmlManager()
{

}

uint32_t
AdventureXmlManager::random_one_adventure(uint32_t type)
{
	if (type == 1) {
		uint32_t sz = adventures[0].size();
		if (sz > 0) {
			uint32_t r = rand() % sz;
			return adventures[0][r];
		}
	} else if (type == 2) {
		uint32_t sz = adventures[1].size();
		if (sz > 0) {
			uint32_t r = rand() % sz;
			return adventures[1][r];
		}
	}

	return 0;
}

const adventure_xml_info_t*
AdventureXmlManager::get_adventure_xml_info(uint32_t adventure_id)
{
	AdventureXmlMap::iterator it = adventure_xml_map.find(adventure_id);
	if (it != adventure_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
AdventureXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_adventure_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	return ret;
}

int
AdventureXmlManager::load_adventure_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("adventure"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			AdventureXmlMap::iterator it = adventure_xml_map.find(id);
			if (it != adventure_xml_map.end()) {
				ERROR_LOG("load adventure xml info err, id exists, id=%u", id);
				return -1;
			}
			adventure_xml_info_t info = {};
			info.id = id;
			get_xml_prop(info.type, cur, "type");
			get_xml_prop(info.time, cur, "time");
			get_xml_prop(info.select_1, cur, "select_1");
			get_xml_prop(info.select_2, cur, "select_2");

			TRACE_LOG("load adventure xml info\t[%u %u %u %u %u]", info.id, info.type, info.time, info.select_1, info.select_2);

			adventure_xml_map.insert(AdventureXmlMap::value_type(id, info));

			if (info.type == 1) {
				adventures[0].push_back(id);
			} else if (info.type == 2) {
				adventures[1].push_back(id);
			}
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*						AdventureSelectXmlManager Class							*/
/********************************************************************************/
AdventureSelectXmlManager::AdventureSelectXmlManager()
{

}

AdventureSelectXmlManager::~AdventureSelectXmlManager()
{

}

const adventure_select_xml_info_t*
AdventureSelectXmlManager::get_adventure_select_xml_info(uint32_t select_id)
{
	AdventureSelectXmlMap::iterator it = adventure_select_xml_map.find(select_id);
	if (it != adventure_select_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
AdventureSelectXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_adventure_select_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	return ret;
}

int
AdventureSelectXmlManager::load_adventure_select_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("select"))) {
			uint32_t select_id = 0;
			get_xml_prop(select_id, cur, "select_id");
			AdventureSelectXmlMap::iterator it = adventure_select_xml_map.find(select_id);
			if (it != adventure_select_xml_map.end()) {
				ERROR_LOG("load adventure select xml info err, select id exists, select_id=%u", select_id);
				return -1;
			}
			adventure_select_xml_info_t info = {};
			info.select_id = select_id;
			get_xml_prop_def(info.role_exp, cur, "role_exp", 0);
			get_xml_prop_def(info.reward_item_id, cur, "reward_item_id", 0);
			get_xml_prop_def(info.reward_item_cnt, cur, "reward_item_cnt", 0);
			get_xml_prop_def(info.cost_diamond, cur, "cost_diamond", 0);
			get_xml_prop_def(info.cost_adventure, cur, "cost_adventure", 0);

			TRACE_LOG("load adventure select xml info\t[%u %u %u %u %u %u]", 
					info.select_id, info.role_exp, info.reward_item_id, info.reward_item_cnt, info.cost_diamond, info.cost_adventure);

			adventure_select_xml_map.insert(AdventureSelectXmlMap::value_type(select_id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*						AdventureItemXmlManager Class							*/
/********************************************************************************/
AdventureItemXmlManager::AdventureItemXmlManager()
{

}

AdventureItemXmlManager::~AdventureItemXmlManager()
{

}

const item_info_t*
AdventureItemXmlManager::random_one_reward(uint32_t select_id)
{
	AdventureItemXmlMap::iterator it = adventure_item_xml_map.find(select_id);
	if (it == adventure_item_xml_map.end()) {
		return 0;
	}

	uint32_t sz = it->second.items.size();
	uint32_t r = rand() % sz;

	return &(it->second.items[r]);
}

int
AdventureItemXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_adventure_item_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

int
AdventureItemXmlManager::load_adventure_item_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("select"))) {
			uint32_t select_id = 0;
			get_xml_prop(select_id, cur, "id");
			item_info_t item_info = {};
			get_xml_prop(item_info.id, cur, "reward_item_id");
			get_xml_prop(item_info.cnt, cur, "reward_item_cnt");
			AdventureItemXmlMap::iterator it = adventure_item_xml_map.find(select_id);
			if (it != adventure_item_xml_map.end()) {
				it->second.items.push_back(item_info);
			} else {
				adventure_item_xml_info_t info = {};
				info.select_id = select_id;
				info.items.push_back(item_info);
				adventure_item_xml_map.insert(AdventureItemXmlMap::value_type(select_id, info));
			}

			TRACE_LOG("load adventure item xml info\t[%u %u %u]", select_id, item_info.id, item_info.cnt);
		}
		cur = cur->next;
	}

	return 0;
}


/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 拉取奇遇列表
 */
int cli_get_adventure_list(Player *p, Cmessage *c_in)
{
	//先检查延时性的奇遇是否过期，若是则删除
	p->adventure_mgr->del_expire_adventure();

	cli_get_adventure_list_out cli_out;
	uint32_t max_tms = p->adventure_mgr->calc_daily_max_complete_tms();
	uint32_t daily_tms = p->res_mgr->get_res_value(daily_adventure_complete_tms);
	cli_out.left_complete_tms = daily_tms < max_tms ? max_tms - daily_tms : 0;

	p->adventure_mgr->pack_client_adventure_list(cli_out);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 完成奇遇事件
 */
int cli_complete_adventure(Player *p, Cmessage *c_in)
{
	cli_complete_adventure_in *p_in = P_IN;

	int ret = p->adventure_mgr->complete_adventure(p_in->time, p_in->type);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	uint32_t max_tms = p->adventure_mgr->calc_daily_max_complete_tms();
	uint32_t daily_tms = p->res_mgr->get_res_value(daily_adventure_complete_tms);

	cli_complete_adventure_out cli_out;
	cli_out.time = p_in->time;
	cli_out.type = p_in->type;
	cli_out.left_complete_tms = daily_tms < max_tms ? max_tms - daily_tms : 0;

	T_KDEBUG_LOG(p->user_id, "COMPLETE ADVENTURE\t[time=%u, type=%u]", p_in->time, p_in->type);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}


/********************************************************************************/
/*								DB Return										*/
/********************************************************************************/
/* @brief 拉取奇遇列表
 */
int db_get_adventure_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);

	db_get_adventure_list_out *p_in = P_IN;
	if (p->wait_cmd == cli_proto_login_cmd) {
		p->adventure_mgr->init_adventure_list(p_in);

		p->login_step++;
		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u GET ADVENTURE LIST", p->login_step);

		//先检查延时性的奇遇是否过期，若是则删除
		p->adventure_mgr->del_expire_adventure();
		cli_get_adventure_list_out cli_out;
		p->adventure_mgr->pack_client_adventure_list(cli_out);
		p->send_to_self(cli_get_adventure_list_cmd, &cli_out, 0);

		//拉取任务信息
		return send_msg_to_dbroute(p, db_get_task_list_cmd, 0, p->user_id);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}
