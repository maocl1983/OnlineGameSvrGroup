/*
 * =====================================================================================
 *
 *  @file trial_tower.cpp 
 *
 *  @brief  试练塔系统
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
#include "trial_tower.hpp"
#include "player.hpp"
#include "dbroute.hpp"

using namespace std;
using namespace project;

//TrialTowerRewardXmlManager trial_tower_reward_xml_mgr;

/********************************************************************************/
/*							TrialTowerManager Class								*/
/********************************************************************************/
TrialTowerManager::TrialTowerManager(Player *p) : owner(p)
{

}

TrialTowerManager::~TrialTowerManager()
{

}

void
TrialTowerManager::update_trial_tower()
{
	uint32_t finish_floor = owner->res_mgr->get_res_value(forever_trial_tower_finish_floor);
	uint32_t history_max_floor = owner->res_mgr->get_res_value(forever_trial_tower_history_max_floor);
	uint32_t sweep_tm = owner->res_mgr->get_res_value(forever_trial_tower_sweep_tm);
	uint32_t sweep_start_floor = owner->res_mgr->get_res_value(forever_trial_tower_sweep_start_floor);
	if (sweep_tm && finish_floor < history_max_floor) {
		uint32_t now_sec = get_now_tv()->tv_sec;
		uint32_t diff_tm = sweep_tm < now_sec ? now_sec - sweep_tm : 0;
		uint32_t cur_floor = sweep_start_floor + diff_tm / 10;
		if (cur_floor > history_max_floor) {
			cur_floor = history_max_floor;
		}
		if (cur_floor > finish_floor) {
			owner->res_mgr->set_res_value(forever_trial_tower_finish_floor, cur_floor);
		}
	}

	if (sweep_tm) {
		uint32_t finish_floor = owner->res_mgr->get_res_value(forever_trial_tower_finish_floor);
		uint32_t history_max_floor = owner->res_mgr->get_res_value(forever_trial_tower_finish_floor);
		if (finish_floor == history_max_floor) {//扫荡结束
			owner->res_mgr->set_res_value(forever_trial_tower_sweep_tm, 0);
			owner->res_mgr->set_res_value(forever_trial_tower_life, 20);
			
			//发送扫荡奖励
			send_sweep_reward(sweep_start_floor + 1, finish_floor);
		}
	}
}

int
TrialTowerManager::send_sweep_reward(uint32_t start_floor, uint32_t end_floor)
{
	if (start_floor > end_floor || !start_floor || end_floor > 100) {
		return -1;
	}

	std::map<uint32_t, uint32_t> items_reward;
	uint32_t golds = 0;
	for (uint32_t floor = start_floor; floor <= end_floor; floor++) {
		const trial_tower_reward_xml_info_t *p_xml_info = trial_tower_reward_xml_mgr->get_trial_tower_reward_xml_info(floor);
		if (p_xml_info) {
			golds += p_xml_info->golds;
			for (int i = 0; i < 3; i++) {
				if (p_xml_info->items[i][0]) {
					uint32_t item_id = p_xml_info->items[i][0];
					uint32_t item_cnt = p_xml_info->items[i][1];
					std::map<uint32_t, uint32_t>::iterator it = items_reward.find(item_id);
					if (it != items_reward.end()) {
						it->second += item_cnt;
					} else {
						items_reward.insert(make_pair(item_id, item_cnt));
					}
				}
			}
		}
	}

	cli_send_get_common_bonus_noti_out noti_out;
	if (golds) {
		owner->chg_golds(golds);
		noti_out.golds = golds;
		T_KDEBUG_LOG(owner->user_id, "SEND SWEEP REWARD GOLDS\t[golds=%u]", golds);
	}

	std::map<uint32_t, uint32_t>::iterator it = items_reward.begin();
	for (; it != items_reward.end(); ++it) {
		owner->items_mgr->add_reward(it->first, it->second);
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, it->first, it->second);
		T_KDEBUG_LOG(owner->user_id, "SEND SWEEP REWARD ITEMS\t[item_id=%u, item_cnt=%u]", it->first, it->second);
	}

	owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	return 0;
}

int
TrialTowerManager::get_sweep_left_tm()
{
	uint32_t finish_floor = owner->res_mgr->get_res_value(forever_trial_tower_finish_floor);
	uint32_t history_max_floor = owner->res_mgr->get_res_value(forever_trial_tower_history_max_floor);
	uint32_t sweep_tm = owner->res_mgr->get_res_value(forever_trial_tower_sweep_tm);
	uint32_t sweep_start_floor = owner->res_mgr->get_res_value(forever_trial_tower_sweep_start_floor);
	if (sweep_tm && finish_floor < history_max_floor) {
		uint32_t now_sec = get_now_tv()->tv_sec;
		uint32_t sweep_end_tm = sweep_start_floor < history_max_floor ? sweep_tm + (history_max_floor - sweep_start_floor) * 10 : 0;
		uint32_t left_tm = now_sec < sweep_end_tm ? sweep_end_tm - now_sec : 0;
		return left_tm;
	}

	return 0;
}

int
TrialTowerManager::trial_tower_sweeping()
{
	uint32_t challenge_tms = owner->res_mgr->get_res_value(daily_trial_tower_challenge_tms);
	if (challenge_tms >= 2) {
		return cli_trial_tower_challenge_tms_not_enough_err;
	}
	uint32_t sweep_cd = get_sweep_left_tm();
	if (sweep_cd) {
		return cli_trial_tower_being_sweeping_err;
	}
	uint32_t finish_floor = owner->res_mgr->get_res_value(forever_trial_tower_finish_floor);
	uint32_t history_max_floor = owner->res_mgr->get_res_value(forever_trial_tower_history_max_floor);
	if (finish_floor >= history_max_floor) {
		return cli_trail_tower_sweep_already_reach_max_floor;
	}

	uint32_t now_sec = get_now_tv()->tv_sec;
	owner->res_mgr->set_res_value(forever_trial_tower_sweep_tm, now_sec);
	owner->res_mgr->set_res_value(forever_trial_tower_sweep_start_floor, finish_floor);

	challenge_tms++;
	owner->res_mgr->set_res_value(daily_trial_tower_challenge_tms, challenge_tms);

	return 0;
}

int
TrialTowerManager::reset_trial_tower()
{
	owner->res_mgr->set_res_value(forever_trial_tower_finish_floor, 0);
	owner->res_mgr->set_res_value(forever_trial_tower_sweep_start_floor, 0);
	owner->res_mgr->set_res_value(forever_trial_tower_life, 0);
	owner->res_mgr->set_res_value(forever_trial_tower_sweep_tm, 0);

	return 0;
}

int
TrialTowerManager::battle_request(uint32_t battle_floor)
{
	uint32_t cur_floor = owner->res_mgr->get_res_value(forever_trial_tower_finish_floor);
	if (cur_floor >= 100 || battle_floor != cur_floor + 1) {
		return cli_trial_tower_battle_floor_err;
	}

	uint32_t life = owner->res_mgr->get_res_value(forever_trial_tower_life);
	if (!life) {
		return cli_trial_tower_left_life_not_enough_err;
	}

	if (get_sweep_left_tm()) {
		return cli_trial_tower_being_sweeping_err;
	}

	return 0;
}

int
TrialTowerManager::battle_end(uint32_t battle_floor, uint32_t is_win) 
{
	uint32_t cur_floor = owner->res_mgr->get_res_value(forever_trial_tower_finish_floor);
	if (cur_floor >= 100 || battle_floor != cur_floor + 1) {
		return cli_trial_tower_battle_floor_err;
	}

	if (is_win) {
		owner->res_mgr->set_res_value(forever_trial_tower_finish_floor, battle_floor);
		uint32_t history_max_floor = owner->res_mgr->get_res_value(forever_trial_tower_history_max_floor);
		if (battle_floor > history_max_floor) {
			owner->res_mgr->set_res_value(forever_trial_tower_history_max_floor, battle_floor);
		}
		//发送奖励 
		const trial_tower_reward_xml_info_t *p_xml_info = trial_tower_reward_xml_mgr->get_trial_tower_reward_xml_info(battle_floor);
		if (p_xml_info) {
			cli_send_get_common_bonus_noti_out noti_out;
			if (p_xml_info->golds) {
				owner->chg_golds(p_xml_info->golds);
				noti_out.golds = p_xml_info->golds;
			}
			for (int i = 0; i < 3; i++) {
				if (p_xml_info->items[i][0]) {
					owner->items_mgr->add_reward(p_xml_info->items[i][0], p_xml_info->items[i][1]);
					owner->items_mgr->pack_give_items_info(noti_out.give_items_info, p_xml_info->items[i][0], p_xml_info->items[i][1]);
				}
			}
			owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);
		}
	} else {
		uint32_t life = owner->res_mgr->get_res_value(forever_trial_tower_life);
		if (life) {
			life--;
		}
		owner->res_mgr->set_res_value(forever_trial_tower_life, life);
	}

	return 0;
}

void
TrialTowerManager::pack_client_trial_tower_info(cli_get_trial_tower_panel_info_out &out)
{
	update_trial_tower();

	uint32_t life = owner->res_mgr->get_res_value(forever_trial_tower_life);
	uint32_t challenge_tms = owner->res_mgr->get_res_value(daily_trial_tower_challenge_tms);
	uint32_t cur_floor = owner->res_mgr->get_res_value(forever_trial_tower_finish_floor);
	uint32_t history_max_floor = owner->res_mgr->get_res_value(forever_trial_tower_history_max_floor);

	out.cur_floor = cur_floor;
	out.history_max_floor = history_max_floor;
	out.max_floor = 100;
	out.left_life = life;
	out.left_challenge_tms = challenge_tms < 2 ? 2 - challenge_tms : 0;
	out.sweep_cd = get_sweep_left_tm();
}

/********************************************************************************/
/*					TrialTowerRewardXmlManager Class							*/
/********************************************************************************/
TrialTowerRewardXmlManager::TrialTowerRewardXmlManager()
{

}

TrialTowerRewardXmlManager::~TrialTowerRewardXmlManager()
{

}

int
TrialTowerRewardXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_trial_tower_reward_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

const trial_tower_reward_xml_info_t *
TrialTowerRewardXmlManager::get_trial_tower_reward_xml_info(uint32_t floor)
{
	TrialTowerRewardXmlMap::iterator it = trial_tower_reward_xml_map.find(floor);
	if (it != trial_tower_reward_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
TrialTowerRewardXmlManager::load_trial_tower_reward_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("trialTower"))) {
			uint32_t floor = 0;
			get_xml_prop(floor, cur, "floor");
			TrialTowerRewardXmlMap::iterator it = trial_tower_reward_xml_map.find(floor);
			if (it != trial_tower_reward_xml_map.end()) {
				ERROR_LOG("load trial tower reward xml info err, floor=%u", floor);
				return -1;
			}

			trial_tower_reward_xml_info_t info = {};
			info.floor = floor;
			get_xml_prop_def(info.golds, cur, "golds", 0);
			get_xml_prop_def(info.items[0][0], cur, "item1_id", 0);
			get_xml_prop_def(info.items[0][1], cur, "item1_cnt", 0);
			get_xml_prop_def(info.items[1][0], cur, "item2_id", 0);
			get_xml_prop_def(info.items[1][1], cur, "item2_cnt", 0);
			get_xml_prop_def(info.items[2][0], cur, "item3_id", 0);
			get_xml_prop_def(info.items[2][1], cur, "item3_cnt", 0);

			TRACE_LOG("load trial tower reward xml info\t[%u %u %u %u %u %u %u %u]", 
					info.floor, info.golds, info.items[0][0], info.items[0][1], info.items[1][0], info.items[1][1], info.items[2][0], info.items[2][1]);

			trial_tower_reward_xml_map.insert(TrialTowerRewardXmlMap::value_type(floor, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 拉取试练塔面板信息
 */
int cli_get_trial_tower_panel_info(Player *p, Cmessage *c_in)
{
	cli_get_trial_tower_panel_info_out cli_out;
	p->trial_tower_mgr->pack_client_trial_tower_info(cli_out);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 试练塔开始挑战
 */
int cli_trial_tower_start_challenge(Player *p, Cmessage *c_in)
{
	uint32_t challenge_tms = p->res_mgr->get_res_value(daily_trial_tower_challenge_tms);
	if (challenge_tms >= 2) {
		return cli_trial_tower_challenge_tms_not_enough_err;
	}
	challenge_tms++;
	p->res_mgr->set_res_value(daily_trial_tower_challenge_tms, challenge_tms);
	p->res_mgr->set_res_value(forever_trial_tower_life, 20);

	cli_trial_tower_start_challenge_out cli_out;
	cli_out.left_challenge_tms = challenge_tms < 2 ? 2 - challenge_tms : 0;
	cli_out.left_life = 20;

	T_KDEBUG_LOG(p->user_id, "TRIAL TOWER START CHALLENGE");

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 试练塔扫荡
 */
int cli_trial_tower_sweeping(Player *p, Cmessage *c_in)
{
	int ret = p->trial_tower_mgr->trial_tower_sweeping();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "TRIAL TOWER SWEEPING");

	cli_trial_tower_sweeping_out cli_out;
	cli_out.sweep_cd = p->trial_tower_mgr->get_sweep_left_tm();

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 试练塔重置
 */
int cli_trial_tower_reset(Player *p, Cmessage *c_in)
{
	int ret = p->trial_tower_mgr->reset_trial_tower();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "TRIAL TOWER RESET");

	uint32_t challenge_tms = p->res_mgr->get_res_value(daily_trial_tower_challenge_tms);
	cli_trial_tower_reset_out cli_out;
	cli_out.cur_floor = 0;
	cli_out.left_life = 0;
	cli_out.left_challenge_tms = challenge_tms < 2 ? 2 - challenge_tms : 0;
	cli_out.sweep_cd = 0;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 试练塔战斗请求
 */
int cli_trial_tower_battle_request(Player *p, Cmessage *c_in)
{
	cli_trial_tower_battle_request_in *p_in = P_IN;
	int ret = p->trial_tower_mgr->battle_request(p_in->battle_floor);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "TRIAL TOWER BATTLE REQUEST\t[battle_floor=%u]", p_in->battle_floor);

	cli_trial_tower_battle_request_out cli_out;
	cli_out.battle_floor = p_in->battle_floor;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 试练塔战斗结束
 */
int cli_trial_tower_battle_end(Player *p, Cmessage *c_in)
{
	cli_trial_tower_battle_end_in *p_in = P_IN;

	int ret = p->trial_tower_mgr->battle_end(p_in->battle_floor, p_in->is_win);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "TRIAL TOWER BATTLE END\t[battle_floor=%u, is_win=%u]", p_in->battle_floor, p_in->is_win);

	uint32_t life = p->res_mgr->get_res_value(forever_trial_tower_life);

	cli_trial_tower_battle_end_out cli_out;
	cli_out.battle_floor = p_in->battle_floor;
	cli_out.is_win = p_in->is_win;
	cli_out.left_life = life;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}
