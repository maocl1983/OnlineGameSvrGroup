/*
 * =====================================================================================
 *
 *  @file  common_fight.cpp 
 *
 *  @brief  处理一些通用玩法
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
/*
#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"
#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"

#include "global_data.hpp"
#include "player.hpp"
#include "restriction.hpp"
*/

#include "stdafx.hpp"
using namespace std;
using namespace project;

//CommonFightXmlManager common_fight_xml_mgr;
//CommonFightDropXmlManager common_fight_drop_xml_mgr;

/********************************************************************************/
/*								CommonFight Class								*/
/********************************************************************************/

CommonFight::CommonFight(Player *p) : owner(p)
{
	kill_guard_idx = 0;
	common_fight_idx = 0;
	rewards.clear();
}

CommonFight::~CommonFight()
{

}

int
CommonFight::get_kill_guard_unlock_idx()
{
	uint32_t role_lv = owner->lv;
	if (role_lv >= 90) {
		return 10;
	} else if (role_lv >= 80) {
		return 9;
	} else if (role_lv >= 70) {
		return 8;
	} else if (role_lv >= 60) {
		return 7;
	} else if (role_lv >= 50) {
		return 6;
	} else if (role_lv >= 40) {
		return 5;
	} else if (role_lv >= 30) {
		return 4;
	} else if (role_lv >= 22) {
		return 3;
	}

	return 0;
}

int
CommonFight::get_common_fight_unlock_idx(uint32_t type)
{
	int unlock_idx = common_fight_xml_mgr->get_common_fight_unlock_step(type, owner->lv);

	return unlock_idx;
}

bool
CommonFight::check_common_fight_tms_is_enough(uint32_t type)
{
	uint32_t res_tms = 0;
	if (type == em_common_fight_trial) {
		res_tms = daily_trial_fight_tms;
	} else if (type == em_common_fight_soldier) {
		res_tms = daily_soldier_fight_tms;
	} else if (type == em_common_fight_defend) {
		res_tms = daily_defend_fight_tms;
	} else if (type == em_common_fight_golds) {
		res_tms = daily_golds_fight_tms;
	} else if (type == em_common_fight_riding_alone) {
		res_tms = daily_riding_alone_fight_tms;
	} else {
		return false;
	}

	uint32_t daily_tms = owner->res_mgr->get_res_value(res_tms);
	if (daily_tms >= 2) {
		return false;
	}

	return true;
}

int
CommonFight::get_common_fight_cd_tm(uint32_t type)
{
	uint32_t res_tm = 0;
	if (type == em_common_fight_trial) {
		res_tm = daily_trial_fight_last_tm;
	} else if (type == em_common_fight_soldier) {
		res_tm = daily_soldier_fight_last_tm;
	} else if (type == em_common_fight_defend) {
		res_tm = daily_defend_fight_last_tm;
	} else if (type == em_common_fight_golds) {
		res_tm = daily_golds_fight_last_tm;
	} else if (type == em_common_fight_riding_alone) {
		res_tm = daily_riding_alone_fight_last_tm;
	} else {
		return false;
	}

	uint32_t last_tm = owner->res_mgr->get_res_value(res_tm);
	uint32_t now_sec = get_now_tv()->tv_sec;
	uint32_t next_tm = last_tm + 15 * 60;
	uint32_t left_tm = now_sec < next_tm ? next_tm - now_sec : 0;

	return left_tm;
}

void
CommonFight::set_common_fight_cd_tm(uint32_t type)
{
	uint32_t res_tm = 0;
	if (type == em_common_fight_trial) {
		res_tm = daily_trial_fight_last_tm;
	} else if (type == em_common_fight_soldier) {
		res_tm = daily_soldier_fight_last_tm;
	} else if (type == em_common_fight_defend) {
		res_tm = daily_defend_fight_last_tm;
	} else if (type == em_common_fight_golds) {
		res_tm = daily_golds_fight_last_tm;
	} else if (type == em_common_fight_riding_alone) {
		res_tm = daily_riding_alone_fight_last_tm;
	} else {
		return;
	}

	uint32_t now_sec = get_now_tv()->tv_sec;
	owner->res_mgr->set_res_value(res_tm, now_sec);

	return;
}

int
CommonFight::random_rewards(uint32_t type, uint32_t idx)
{
	rewards.clear();
	const common_fight_drop_step_xml_info_t *p_xml_info = common_fight_drop_xml_mgr->get_common_fight_drop_xml_info(type, idx);
	if (!p_xml_info) {
		return -1;
	}

	for (int i = 0; i <4; i++) {
		const common_fight_drop_item_xml_info_t *p_item_xml_info = &(p_xml_info->rewards[i]);
		if (p_item_xml_info->item_id) {
			uint32_t r = rand() % 100;
			if (r < p_item_xml_info->prob) {
				item_info_t info;
				info.id = p_item_xml_info->item_id;
				info.cnt = p_item_xml_info->item_cnt;
				rewards.push_back(info);
			}
		}
	}

	return 0;
}

int
CommonFight::battle_end(uint32_t type, uint32_t is_win, uint32_t &left_tms, std::vector<cli_item_info_t> &items_vec)
{
	uint32_t res_tms = 0;
	if (type == em_common_fight_trial) {
		res_tms = daily_trial_fight_tms;
	} else if (type == em_common_fight_soldier) {
		res_tms = daily_soldier_fight_tms;
	} else if (type == em_common_fight_defend) {
		res_tms = daily_defend_fight_tms;
	} else if (type == em_common_fight_golds) {
		res_tms = daily_golds_fight_tms;
	} else if (type == em_common_fight_riding_alone) {
		res_tms = daily_riding_alone_fight_tms;
	} else {
		return -1;
	}

	uint32_t daily_tms = owner->res_mgr->get_res_value(res_tms);
	if (is_win) {
		daily_tms++;
		owner->res_mgr->set_res_value(res_tms, daily_tms);
		for (uint32_t i = 0; i < rewards.size(); i++) {
			const item_info_t *p_item_info = &(rewards[i]);
			owner->items_mgr->add_reward(p_item_info->id, p_item_info->cnt);
			cli_item_info_t info;
			info.item_id = p_item_info->id;
			info.item_cnt = p_item_info->cnt;
			items_vec.push_back(info);
		}
		//设置cd
		set_common_fight_cd_tm(type);

		//检查任务
		owner->task_mgr->check_task(em_task_type_common_fight, type);
	}

	left_tms = daily_tms < 2 ? 2 - daily_tms : 0;

	//清空奖励
	rewards.clear();

	return 0;
}

void
CommonFight::pack_common_fight_rewards(vector<cli_item_info_t> &vec)
{
	for (uint32_t i = 0; i < rewards.size(); i++) {
		item_info_t *p_info = &(rewards[i]);
		cli_item_info_t info;
		info.item_id = p_info->id;
		info.item_cnt = p_info->cnt;
		vec.push_back(info);
	}
}

void 
CommonFight::pack_common_fight_info(uint32_t type, cli_get_common_fight_panel_info_out &cli_out)
{
	uint32_t res_tms = 0;
	uint32_t res_tm = 0;
	if (type == em_common_fight_trial) {
		res_tms = daily_trial_fight_tms;
		res_tm = daily_trial_fight_last_tm;
	} else if (type == em_common_fight_soldier) {
		res_tms = daily_soldier_fight_tms;
		res_tm = daily_soldier_fight_last_tm;
	} else if (type == em_common_fight_defend) {
		res_tms = daily_defend_fight_tms;
		res_tm = daily_defend_fight_last_tm;
	} else if (type == em_common_fight_golds) {
		res_tms = daily_golds_fight_tms;
		res_tm = daily_golds_fight_last_tm;
	} else if (type == em_common_fight_riding_alone) {
		res_tms = daily_riding_alone_fight_tms;
		res_tm = daily_riding_alone_fight_last_tm;
	} else {
		return;
	}

	uint32_t unlock_idx = get_common_fight_unlock_idx(type);
	uint32_t daily_tms = owner->res_mgr->get_res_value(res_tms);
	uint32_t last_tm = owner->res_mgr->get_res_value(res_tm);
	uint32_t now_sec = get_now_tv()->tv_sec;

	cli_out.fight_info.type = type;
	cli_out.fight_info.unlock_idx = unlock_idx;
	cli_out.fight_info.left_tms = daily_tms < 2 ? 2 - daily_tms : 0;
	cli_out.fight_info.cd_tm = (last_tm + 15 * 60) > now_sec ? (last_tm + 15 * 60 - now_sec) : 0; 
}

/********************************************************************************/
/*							CommonFightXmlManager Class							*/
/********************************************************************************/
CommonFightXmlManager::CommonFightXmlManager()
{

}

CommonFightXmlManager::~CommonFightXmlManager()
{

}

int
CommonFightXmlManager::get_common_fight_unlock_step(uint32_t type, uint32_t lv)
{
	CommonFightXmlMap::iterator it = common_fight_xml_map.find(type);
	if (it == common_fight_xml_map.end()) {
		return -1;
	}

	uint32_t max_unlock_idx = 0;
	CommonFightStepXmlMap::iterator it2 = it->second.step_map.begin();
	for (; it2 != it->second.step_map.end(); ++it2) {
		if (lv >= it2->second.unlock_lv ){
			if (max_unlock_idx < it2->second.step) {
				max_unlock_idx = it2->second.step;
			}
		}
	}

	return max_unlock_idx;
}

int
CommonFightXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_common_fight_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

int
CommonFightXmlManager::load_common_fight_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("commonFight"))) {
			uint32_t type = 0;
			get_xml_prop(type, cur, "type")	;
			common_fight_step_xml_info_t sub_info = {};
			get_xml_prop(sub_info.step, cur, "step");
			get_xml_prop(sub_info.unlock_lv, cur, "unlock_lv");

			CommonFightXmlMap::iterator it = common_fight_xml_map.find(type);
			if (it != common_fight_xml_map.end()) {
				CommonFightStepXmlMap::iterator it2 = it->second.step_map.find(sub_info.step);
				if (it2 != it->second.step_map.end()) {
					ERROR_LOG("load common fight xml info err, type=%u, step=%u", type, sub_info.step);
					return -1;
				}
				it->second.step_map.insert(CommonFightStepXmlMap::value_type(sub_info.step, sub_info));
			} else {
				common_fight_xml_info_t info = {};
				info.type = type;
				info.step_map.insert(CommonFightStepXmlMap::value_type(sub_info.step, sub_info));
				common_fight_xml_map.insert(CommonFightXmlMap::value_type(type, info));
			}
			TRACE_LOG("load common fight xml info\t[%u %u %u]", type, sub_info.step, sub_info.unlock_lv);
		}
		cur = cur->next;
	}

	return 0;
}
/********************************************************************************/
/*							CommonFightDropXmlManager Class						*/
/********************************************************************************/
CommonFightDropXmlManager::CommonFightDropXmlManager()
{

}

CommonFightDropXmlManager::~CommonFightDropXmlManager()
{

}

const common_fight_drop_step_xml_info_t *
CommonFightDropXmlManager::get_common_fight_drop_xml_info(uint32_t type, uint32_t step)
{
	CommonFightDropXmlMap::iterator it = common_fight_drop_xml_map.find(type);
	if (it == common_fight_drop_xml_map.end()) {
		return 0;
	}

	CommonFightDropStepXmlMap::iterator it2 = it->second.step_map.find(step);
	if (it2 == it->second.step_map.end()) {
		return 0;
	}

	return &(it2->second);
}

int
CommonFightDropXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_common_fight_drop_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

int
CommonFightDropXmlManager::load_common_fight_drop_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("commonFight"))) {
			uint32_t type = type;
			common_fight_drop_step_xml_info_t sub_info = {};
			get_xml_prop(type, cur, "type");
			get_xml_prop(sub_info.step, cur, "step");
			get_xml_prop_def(sub_info.rewards[0].item_id, cur, "item1_id", 0);
			get_xml_prop_def(sub_info.rewards[0].item_cnt, cur, "item1_cnt", 0);
			get_xml_prop_def(sub_info.rewards[0].prob, cur, "item1_prob", 0);
			get_xml_prop_def(sub_info.rewards[1].item_id, cur, "item2_id", 0);
			get_xml_prop_def(sub_info.rewards[1].item_cnt, cur, "item2_cnt", 0);
			get_xml_prop_def(sub_info.rewards[1].prob, cur, "item2_prob", 0);
			get_xml_prop_def(sub_info.rewards[2].item_id, cur, "item3_id", 0);
			get_xml_prop_def(sub_info.rewards[2].item_cnt, cur, "item3_cnt", 0);
			get_xml_prop_def(sub_info.rewards[2].prob, cur, "item3_prob", 0);
			get_xml_prop_def(sub_info.rewards[3].item_id, cur, "item4_id", 0);
			get_xml_prop_def(sub_info.rewards[3].item_cnt, cur, "item4_cnt", 0);
			get_xml_prop_def(sub_info.rewards[3].prob, cur, "item4_prob", 0);

			CommonFightDropXmlMap::iterator it = common_fight_drop_xml_map.find(type);
			if (it != common_fight_drop_xml_map.end()) {
				CommonFightDropStepXmlMap::iterator it2 = it->second.step_map.find(sub_info.step);
				if (it2 != it->second.step_map.end()) {
					ERROR_LOG("load common fight drop xml info err, type=%u, step=%u", type, sub_info.step);
					return -1;
				}
				it->second.step_map.insert(CommonFightDropStepXmlMap::value_type(sub_info.step, sub_info));
			} else {
				common_fight_drop_xml_info_t info = {};
				info.type = type;
				info.step_map.insert(CommonFightDropStepXmlMap::value_type(sub_info.step, sub_info));
				common_fight_drop_xml_map.insert(CommonFightDropXmlMap::value_type(type, info));
			}

			TRACE_LOG("load common fight drop xml info\t[%u %u %u %u %u %u %u %u %u %u %u %u %u %u]", type, sub_info.step, 
					sub_info.rewards[0].item_id, sub_info.rewards[0].item_cnt, sub_info.rewards[0].prob, 
					sub_info.rewards[1].item_id, sub_info.rewards[1].item_cnt, sub_info.rewards[1].prob, 
					sub_info.rewards[2].item_id, sub_info.rewards[2].item_cnt, sub_info.rewards[2].prob, 
					sub_info.rewards[3].item_id, sub_info.rewards[3].item_cnt, sub_info.rewards[3].prob 
					);
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 拉取过关斩将玩法信息
 */
int cli_get_kill_guard_info(Player *p, Cmessage *c_in)
{
	uint32_t lose_tms = p->res_mgr->get_res_value(daily_kill_guard_lose_tms);
	uint32_t cur_idx = p->res_mgr->get_res_value(daily_kill_guard_cur_idx);
	uint32_t last_tm = p->res_mgr->get_res_value(daily_kill_guard_last_tm);
	uint32_t now_sec = get_now_tv()->tv_sec;
	uint32_t next_tm = last_tm + 15 * 60;
	cli_get_kill_guard_info_out cli_out;
	cli_out.cur_idx = cur_idx;
	cli_out.unlock_idx = p->common_fight->get_kill_guard_unlock_idx();
	cli_out.left_tms = lose_tms < 2 ? 2 - lose_tms : 0;
	cli_out.cd_tm = now_sec < next_tm ? next_tm - now_sec : 0;

	for (uint32_t i = 0; i < cur_idx; i++) {
		uint32_t res_type = daily_kill_guard_hero_1 + i;
		uint32_t res_value = p->res_mgr->get_res_value(res_type);
		if (res_value) {
			cli_out.kill_heros.push_back(res_value);
		}
	}

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 过关斩将-战斗请求
 */
int cli_kill_guard_battle_request(Player *p, Cmessage *c_in)
{
	cli_kill_guard_battle_request_in *p_in = P_IN;
	if (!p_in->idx || p_in->idx > 10) {
		T_KERROR_LOG(p->user_id, "kill guard battle request err, idx=%u", p_in->idx);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_input_arg_err, 1);
	}

	uint32_t unlock_idx = p->common_fight->get_kill_guard_unlock_idx();
	if (p_in->idx > unlock_idx) {
		T_KERROR_LOG(p->user_id, "kill guard battle idx not unlock, idx=%u, unlock_idx=%u", p_in->idx, unlock_idx);
		return p->send_to_self_error(p->wait_cmd, cli_kill_guard_not_unlock_err, 1);
	}

	uint32_t lose_tms = p->res_mgr->get_res_value(daily_kill_guard_lose_tms);
	if (lose_tms >= 2) {
		return p->send_to_self_error(p->wait_cmd, cli_kill_guard_request_tms_not_enough_err, 1);
	}

	uint32_t last_tm = p->res_mgr->get_res_value(daily_kill_guard_last_tm);
	uint32_t now_sec = get_now_tv()->tv_sec;
	uint32_t next_tm = last_tm + 15 * 60;
	if (now_sec < next_tm) {
		return p->send_to_self_error(p->wait_cmd, cli_kill_guard_being_cd_err, 1);
	}

	//缓存idx
	p->common_fight->set_kill_guard_idx(p_in->idx);

	T_KDEBUG_LOG(p->user_id, "KILL GUARD BATTLE REQUEST\t[idx=%u]", p_in->idx);

	return p->send_to_self(p->wait_cmd, 0, 1);
} 

/* @brief 过关斩将-战斗结束
 */
int cli_kill_guard_battle_end(Player *p, Cmessage *c_in)
{
	cli_kill_guard_battle_end_in *p_in = P_IN;

	if (!p_in->idx || p_in->idx > 10) {
		T_KWARN_LOG(p->user_id, "kill guard battle end err, idx=%u", p_in->idx);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_input_arg_err, 1);
	}

	uint32_t unlock_idx = p->common_fight->get_kill_guard_unlock_idx();
	if (p_in->idx > unlock_idx) {
		T_KWARN_LOG(p->user_id, "kill guard battle idx not unlock, idx=%u, unlock_idx=%u", p_in->idx, unlock_idx);
		return p->send_to_self_error(p->wait_cmd, cli_kill_guard_not_unlock_err, 1);
	}

	uint32_t lose_tms = p->res_mgr->get_res_value(daily_kill_guard_lose_tms);
	if (lose_tms >= 2) {
		return p->send_to_self_error(p->wait_cmd, cli_kill_guard_request_tms_not_enough_err, 1);
	}

	uint32_t idx = p->common_fight->get_kill_guard_idx();
	if (idx != p_in->idx) {
		T_KWARN_LOG(p->user_id, "kill guard battle idx not match[idx=%u, in_idx=%u]", idx, p_in->idx);
		return p->send_to_self_error(p->wait_cmd, cli_kill_guard_idx_not_match_err, 1);
	}

	if (p_in->is_win) {
		p->res_mgr->set_res_value(daily_kill_guard_cur_idx, p_in->idx);
		uint32_t res_type = daily_kill_guard_hero_1 + p_in->idx - 1;
		p->res_mgr->set_res_value(res_type, p_in->kill_hero_id);

		//设置CD
		uint32_t now_sec = get_now_tv()->tv_sec;
		p->res_mgr->set_res_value(daily_kill_guard_last_tm, now_sec);

		//奖励 TODO
		
		//检查任务
		p->task_mgr->check_task(em_task_type_common_fight, em_common_fight_kill_guard);
	} else {
		lose_tms++;
		p->res_mgr->set_res_value(daily_kill_guard_lose_tms, lose_tms);
	}

	T_KDEBUG_LOG(p->user_id, "KILL GUARD BATTLE END\t[idx=%u, is_win=%u, kill_hero_id=%u]", p_in->idx, p_in->is_win, p_in->kill_hero_id);

	uint32_t last_tm = p->res_mgr->get_res_value(daily_kill_guard_last_tm);
	uint32_t now_sec = get_now_tv()->tv_sec;
	uint32_t next_tm = last_tm + 15 * 60;

	cli_kill_guard_battle_end_out cli_out;
	cli_out.idx = p_in->idx;
	cli_out.is_win = p_in->is_win;
	cli_out.left_tms = lose_tms < 2 ? 2 - lose_tms : 0;
	cli_out.cd_tm = now_sec < next_tm ? next_tm - now_sec : 0;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 通用玩法-拉取面板信息
 */
int cli_get_common_fight_panel_info(Player *p, Cmessage *c_in)
{
	cli_get_common_fight_panel_info_in *p_in = P_IN;
	cli_get_common_fight_panel_info_out cli_out;

	p->common_fight->pack_common_fight_info(p_in->type, cli_out);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 通用玩法-战斗请求
 */
int cli_common_fight_battle_request(Player *p, Cmessage *c_in)
{
	cli_common_fight_battle_request_in *p_in = P_IN;
	if (!p_in->idx || p_in->idx > 10) {
		T_KERROR_LOG(p->user_id, "common fight battle request err, idx=%u", p_in->idx);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_input_arg_err, 1);
	}

	uint32_t unlock_idx = p->common_fight->get_common_fight_unlock_idx(p_in->type);
	if (p_in->idx > unlock_idx) {
		T_KERROR_LOG(p->user_id, "common fight battle idx not unlock, idx=%u, unlock_idx=%u", p_in->idx, unlock_idx);
		return p->send_to_self_error(p->wait_cmd, cli_common_fight_not_unlock_err, 1);
	}

	if (!p->common_fight->check_common_fight_tms_is_enough(p_in->type)) {
		return p->send_to_self_error(p->wait_cmd, cli_common_fight_tms_not_enough_err, 1);
	}

	if (p->common_fight->get_common_fight_cd_tm(p_in->type) > 0) {
		return p->send_to_self_error(p->wait_cmd, cli_common_fight_being_cd_err, 1);
	}

	p->common_fight->set_common_fight_idx(p_in->idx);
	p->common_fight->random_rewards(p_in->type, p_in->idx);

	cli_common_fight_battle_request_out cli_out;
	cli_out.type = p_in->type;
	cli_out.idx = p_in->idx;
	p->common_fight->pack_common_fight_rewards(cli_out.give_items_info);

	T_KDEBUG_LOG(p->user_id, "COMMON FIGHT BATTLE REQUEST\t[type=%u, idx=%u]", p_in->type, p_in->idx);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 通用玩法-战斗结束
 */
int cli_common_fight_battle_end(Player *p, Cmessage *c_in)
{
	cli_common_fight_battle_end_in *p_in = P_IN;
	uint32_t cur_idx = p->common_fight->get_common_fight_idx();
	if (cur_idx != p_in->idx) {
		T_KWARN_LOG(p->user_id, "common fight battle idx not mathch\t[idx=%u, cur_idx=%u]", p_in->idx, cur_idx);
		return p->send_to_self_error(p->wait_cmd, cli_common_fight_idx_not_match_err, 1);
	}

	uint32_t unlock_idx = p->common_fight->get_common_fight_unlock_idx(p_in->type);
	if (p_in->idx > unlock_idx) {
		T_KERROR_LOG(p->user_id, "common fight battle idx not unlock, type=%u, idx=%u, unlock_idx=%u", p_in->type, p_in->idx, unlock_idx);
		return p->send_to_self_error(p->wait_cmd, cli_common_fight_not_unlock_err, 1);
	}

	if (!p->common_fight->check_common_fight_tms_is_enough(p_in->type)) {
		return p->send_to_self_error(p->wait_cmd, cli_common_fight_tms_not_enough_err, 1);
	}

	uint32_t left_tms = 0;
	cli_common_fight_battle_end_out cli_out;
	p->common_fight->battle_end(p_in->type, p_in->is_win, left_tms, cli_out.give_items_info);

	T_KDEBUG_LOG(p->user_id, "COMMON FIGHT BATTLE END\t[type=%u, idx=%u, is_win=%u]", p_in->type, p_in->idx, p_in->is_win);

	cli_out.type = p_in->type;
	cli_out.idx = p_in->idx;
	cli_out.is_win = p_in->is_win;
	cli_out.left_tms = left_tms;
	cli_out.cd_tm = p->common_fight->get_common_fight_cd_tm(p_in->type);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);

}
