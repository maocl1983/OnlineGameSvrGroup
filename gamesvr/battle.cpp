/*
 * =====================================================================================
 *
 *  @file  battle.cpp 
 *
 *  @brief  处理战斗相关逻辑
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
#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"
#include "./proto/common.hpp"

#include "global_data.hpp"
#include "battle.hpp"
#include "instance.hpp"
#include "restriction.hpp"
#include "item.hpp"
#include "player.hpp"
*/

#include "stdafx.hpp"
using namespace std;
using namespace project;

//BattleCacheMap g_battle_cache_map;

/********************************************************************************/
/*								Battle Class									*/
/********************************************************************************/
Battle::Battle(Player* p) : owner(p)
{
	stat = 0;
}

Battle::~Battle()
{

}

int
Battle::init_battle()
{
	BattleCacheMap::iterator it = g_battle_cache_map->find(owner->user_id);
	if (it == g_battle_cache_map->end()) {
		return 0;
	}

	stat = it->second.stat;

	return 0;
}

int
Battle::cache_battle_info(uint32_t battle_id, uint32_t stat)
{
	battle_cache_info_t info;
	info.user_id = owner->user_id;
	info.battle_id = battle_id;
	info.stat = stat;
	BattleCacheMap::iterator it = g_battle_cache_map->find(owner->user_id);
	if (it == g_battle_cache_map->end()) {
		g_battle_cache_map->insert(BattleCacheMap::value_type(owner->user_id, info));
	} else {
		it->second.battle_id = battle_id;
		it->second.stat = stat;
	}
	return 0;
}

int
Battle::cache_instance_reward_info(uint32_t exp, uint32_t golds, vector<cli_item_info_t> &reward_vec)
{
	BattleCacheMap::iterator it = g_battle_cache_map->find(owner->user_id);
	if (it == g_battle_cache_map->end()) {
		return 0;
	}
	it->second.exp = exp;
	it->second.golds = golds;
	for (uint32_t i = 0; i < reward_vec.size(); i++) {
		cli_item_info_t* p_info = &(reward_vec[i]);
		cli_item_info_t info;
		info.item_id = p_info->item_id;
		info.item_cnt = p_info->item_cnt;
		it->second.rewards.push_back(info);
	}

	return 0;
}

battle_cache_info_t*
Battle::get_battle_cache_info()
{
	BattleCacheMap::iterator it = g_battle_cache_map->find(owner->user_id);
	if (it == g_battle_cache_map->end()) {
		return 0;
	}

	return &(it->second);
}

int
Battle::battle_start(uint32_t battle_id)
{
	init_battle();
	if (stat) {
		return cli_battle_already_start_err;
	}

	stat = 1;
	cache_battle_info(battle_id, stat);

	return 0;
}

int
Battle::battle_end()
{
	battle_cache_info_t* p_info = get_battle_cache_info();
	if (!p_info) {
		return cli_battle_already_end_err;
	}

	p_info->battle_id = 0;
	p_info->stat = 0;
	p_info->exp = 0;
	p_info->golds = 0;
	p_info->rewards.clear();

	return 0;
}

int
Battle::pack_instance_monster_info(vector<cli_instance_monster_info_t> &out_vec, uint32_t instance_id)
{
	for (uint32_t i = 0; i < 3; i++) {
		uint32_t round = i + 1;
		const instance_troop_xml_info_t* p_info = instance_xml_mgr.get_instance_troop_xml_info(instance_id, round);
		if (p_info) {
			cli_instance_monster_info_t info;
			info.round = round;
			for (uint32_t pos = 1; pos <= 5; pos++) {
				uint32_t monster_id = troop_xml_mgr.get_troop_monster_id(p_info->troop_id, pos);
				if (monster_id) {
					const monster_xml_info_t* p_monster = monster_xml_mgr.get_monster_xml_info(monster_id);
					if (p_monster) {
						common_instance_monster_info_t monster_info;
						monster_info.monster_id = p_monster->id;
						monster_info.pos = pos;
						monster_info.atk_type = p_monster->atk_type;
						monster_info.lv = p_monster->lv;
						monster_info.grade = p_monster->grade;
						monster_info.star = p_monster->star;
						monster_info.attr._str = p_monster->attr._str;
						monster_info.attr._agi = p_monster->attr._agi;
						monster_info.attr._int = p_monster->attr._int;
						monster_info.attr._spd = p_monster->attr._spd;
						monster_info.attr.max_hp = p_monster->attr.max_hp;
						monster_info.attr.pa = p_monster->attr.pa;
						monster_info.attr.pd = p_monster->attr.pa;
						monster_info.attr.pcr = p_monster->attr.pa;
						monster_info.attr.pt = p_monster->attr.pa;
						monster_info.attr.ma = p_monster->attr.pa;
						monster_info.attr.md = p_monster->attr.pa;
						monster_info.attr.mcr = p_monster->attr.pa;
						monster_info.attr.mt = p_monster->attr.pa;
						monster_info.attr.hit = p_monster->attr.pa;
						monster_info.attr.miss = p_monster->attr.pa;
						monster_info.attr.hp_recovery = p_monster->attr.hp_recovery;
						monster_info.attr.ep_recovery = p_monster->attr.ep_recovery;
						info.monsters.push_back(monster_info);
					}
				}
			}
			out_vec.push_back(info);
		}
	}

	return 0;
}

int
Battle::pack_instance_reward_info(cli_instance_battle_request_out &out, uint32_t drop_id)
{
	const instance_drop_xml_info_t* p_drop = instance_drop_xml_mgr.get_instance_drop_xml_info(drop_id);
	if (p_drop) {
		out.exp = p_drop->exp;
		out.golds = p_drop->golds;
		InstanceDropItemXmlMap::const_iterator it = p_drop->drop_items.begin();
		for (; it != p_drop->drop_items.end(); it++) {
			const instance_drop_item_info_t* p_info = &(it->second);
			uint32_t count = 0;
			for (uint32_t i = 0; i < p_info->item_cnt; i++) {
				uint32_t r = rand() % 100;
				if (r < p_info->prob) {
					count++;
				}
			}
			if (count) {
				cli_item_info_t info;
				info.item_id = p_info->item_id;
				info.item_cnt = count;
				out.reward_info.push_back(info);
			}
		}
	}
	return 0;
}

/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 副本战斗请求
 */
int cli_instance_battle_request(Player* p, Cmessage* c_in)
{
	cli_instance_battle_request_in* p_in = P_IN;

	const instance_xml_info_t* p_info = instance_xml_mgr.get_instance_xml_info(p_in->instance_id);
	if (!p_info) {
		KERROR_LOG(p->user_id, "invalid instance id, id=%u", p_in->instance_id);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_instance_id_err, 1);
	}
	int ret = p->battle->battle_start(p_in->instance_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	if (p->lv < p_info->need_lv) {
		return p->send_to_self_error(p->wait_cmd, cli_instance_need_lv_not_enough_err, 1);
	}

	uint32_t complete_instance = p->res_mgr->get_res_value(forever_complete_instance_id);
	if (complete_instance < p_info->pre_instance) {
		return p->send_to_self_error(p->wait_cmd, cli_pre_instance_not_completed_err, 1);
	}

	cli_instance_battle_request_out cli_out;
	p->battle->pack_instance_monster_info(cli_out.monster_info, p_in->instance_id);
	p->battle->pack_instance_reward_info(cli_out, p_info->drop_id);

	//缓存奖励信息
	p->battle->cache_instance_reward_info(cli_out.exp, cli_out.golds, cli_out.reward_info);

	KDEBUG_LOG(p->user_id, "INSTANCE BATTLE REQUEST\t[instance_id=%u]", p_in->instance_id);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 副本战斗结束
 */
int cli_instance_battle_end(Player* p, Cmessage* c_in)
{
	cli_instance_battle_end_in* p_in = P_IN;

	const instance_xml_info_t* p_info = instance_xml_mgr.get_instance_xml_info(p_in->instance_id);
	if (!p_info) {
		KERROR_LOG(p->user_id, "invalid instance id, id=%u", p_in->instance_id);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_instance_id_err, 1);
	}

	battle_cache_info_t* p_cache_info = p->battle->get_battle_cache_info();
	if (!p_cache_info) {
		KERROR_LOG(p->user_id, "instance battle end err, instance_id=%u", p_in->instance_id);
		return -1;
	}

	p->add_exp(p_cache_info->exp);
	p->chg_golds(p_cache_info->golds);

	for (uint32_t i = 0; i < p_cache_info->rewards.size(); i++) {
		cli_item_info_t* p_info = &(p_cache_info->rewards[i]);
		p->items_mgr->add_item_without_callback(p_info->item_id, p_info->item_cnt);
	}

	p->battle->battle_end();

	return p->send_to_self(p->wait_cmd, 0, 1);
}

