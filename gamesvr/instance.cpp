/*
 * =====================================================================================
 *
 *  @file  intance.hpp 
 *
 *  @brief  处理副本相关逻辑
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
#include "instance.hpp"
#include "player.hpp"
#include "dbroute.hpp"

using namespace std;
using namespace project;

//InstanceXmlManager instance_xml_mgr;
//MonsterXmlManager monster_xml_mgr;
//InstanceDropXmlManager instance_drop_xml_mgr;
//InstanceChapterXmlManager instance_chapter_xml_mgr;
//InstanceBagXmlManager instance_bag_xml_mgr;

/********************************************************************************/
/*						InstanceManager Class									*/
/********************************************************************************/
InstanceManager::InstanceManager(Player *p) : owner(p)
{
	cur_instance_id = 0;
	cache_map.clear();
	heros.clear();
	first_rewards.clear();
	random_rewards.clear();
}

InstanceManager::~InstanceManager()
{

}

int
InstanceManager::init_instance_list(vector<db_instance_info_t> &instance_list)
{
	for (uint32_t i = 0; i < instance_list.size(); i++) {
		db_instance_info_t *p_info = &(instance_list[i]);
	
		instance_cache_info_t info;
		info.instance_id = p_info->instance_id;
		info.daily_tms = p_info->daily_res;

		cache_map.insert(InstanceCacheMap::value_type(info.instance_id, info));

		T_KTRACE_LOG(owner->user_id, "init instance list\t[%u %u]", info.instance_id, info.daily_tms);
	}

	return 0;
}

int
InstanceManager::set_hero(uint32_t hero_id, uint32_t status)
{
	std::set<uint32_t>::iterator it = heros.find(hero_id);
	if (status == 0) {
		if (it != heros.end()) {
			heros.erase(it);
		} 
	} else {
		if (it == heros.end()) {
			heros.insert(hero_id);
		}
	}

	return 0;
}

int
InstanceManager::clear_heros()
{
	std::set<uint32_t>::iterator it = heros.begin();
	while (it != heros.end()) {
		uint32_t hero_id = *(it++);
		Hero *p_hero = owner->hero_mgr->get_hero(hero_id);
		if (p_hero) {
			p_hero->set_hero_status(0);
		}
	}

	return 0;
}

int
InstanceManager::set_soldier(uint32_t soldier_id, uint32_t status)
{
	std::set<uint32_t>::iterator it = soldiers.find(soldier_id);
	if (status == 0) {
		if (it != soldiers.end()) {
			soldiers.erase(it);
		}
	} else {
		if (it == heros.end()) {
			soldiers.insert(soldier_id);
		}
	}

	return 0;
}

int
InstanceManager::clear_soldiers()
{
	std::set<uint32_t>::iterator it = soldiers.begin();
	while (it != soldiers.end()) {
		uint32_t soldier_id = *(it++);
		Soldier *p_soldier = owner->soldier_mgr->get_soldier(soldier_id);
		if (p_soldier) {
			p_soldier->set_soldier_status(0);
		}
	}

	return 0;
}

instance_cache_info_t *
InstanceManager::get_instance_cache_info(uint32_t instance_id)
{
	InstanceCacheMap::iterator it = cache_map.find(instance_id);
	if (it != cache_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
InstanceManager::gen_first_rewards(uint32_t instance_id)
{
	first_rewards.clear();
	instance_cache_info_t *p_info = this->get_instance_cache_info(instance_id);
	if (p_info) {
		return -1;
	}

	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		return -1;
	}

	const instance_drop_xml_info_t * p_drop_xml_info = instance_drop_xml_mgr->get_instance_drop_xml_info(p_xml_info->drop_id);
	if (!p_drop_xml_info) {
		return -1;
	}

	for (uint32_t i = 0; i < 3; i++) {
		const instance_drop_item_info_t *p_item_info = &(p_drop_xml_info->first_items[i]);
		if (p_item_info->item_id && p_item_info->item_cnt) {
			item_info_t info = {};
			info.id = p_item_info->item_id;
			info.cnt = p_item_info->item_cnt;
			first_rewards.push_back(info);
		}
	}
	
	return 0;
}

int
InstanceManager::gen_random_rewards(uint32_t instance_id)
{
	random_rewards.clear();
	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		return -1;
	}
	const instance_drop_xml_info_t * p_drop_xml_info = instance_drop_xml_mgr->get_instance_drop_xml_info(p_xml_info->drop_id);
	if (!p_drop_xml_info) {
		return -1;
	}

	//随机奖励
	for (uint32_t i = 0; i < 3; i++) {
		const instance_drop_prob_item_info_t *p_item_info = &(p_drop_xml_info->random_items[i]);
		if (p_item_info->item_id && p_item_info->prob) {
			uint32_t r = rand() % 100;
			if (r < p_item_info->prob) {
				item_info_t info = {};
				info.id = p_item_info->item_id;
				info.cnt = p_item_info->item_cnt;
				random_rewards.push_back(info);
			}
		}
	}

	return 0;
}

int
InstanceManager::battle_request(uint32_t instance_id)
{
	int ret = check_instance_battle(instance_id);
	if (ret) {
		return ret;
	}

	//owner->res_mgr->set_res_value(forever_cur_battle_instance_id, instance_id);
	set_cur_instance_id(instance_id);

	//设置挑战次数
	instance_cache_info_t *p_info = get_instance_cache_info(instance_id);
	if (p_info) {
		p_info->daily_tms++;
	}

	//生成奖励信息
	gen_first_rewards(instance_id);
	gen_random_rewards(instance_id);

	return 0;
}

int
InstanceManager::battle_end(uint32_t instance_id, uint32_t time, uint32_t win, uint32_t lose, uint32_t kill_hero_id, cli_instance_battle_end_out &cli_out)
{
	//uint32_t cur_instance_id = owner->res_mgr->get_res_value(forever_cur_battle_instance_id);
	uint32_t cur_instance_id = get_cur_instance_id();
	if (instance_id != cur_instance_id) {
		T_KERROR_LOG(owner->user_id, "instance battle id err, cur_instance_id=%u, instance_id=%u", cur_instance_id, instance_id);
		return cli_instance_id_not_match_err;
	}

	cli_out.instance_id = instance_id;
	uint32_t is_win = 0;
	if (check_instance_battle_is_win(instance_id, time, win, lose)) {
		is_win = 1;
		give_instance_battle_reward(instance_id, kill_hero_id, cli_out);

		//检查任务
		owner->task_mgr->check_task(em_task_type_story_instance, instance_id);

		//检查成就
		owner->achievement_mgr->check_achievement(em_achievement_type_story_instance, instance_id);
	} else {
		pack_instance_lose_info(cli_out);
		is_win = 0;
	}
	cli_out.is_win = is_win;

	return 0;
}

int
InstanceManager::init_chapter_list(vector<db_chapter_info_t> &chapter_list)
{
	for (uint32_t i = 0; i < chapter_list.size(); i++) {
		db_chapter_info_t *p_info = &(chapter_list[i]);

		chapter_info_t info = {};
		info.chapter_id = p_info->chapter_id;
		info.star = p_info->star;
		info.bag_stat = p_info->bag_stat;

		chapter_map.insert(ChapterMap::value_type(info.chapter_id, info));

		T_KTRACE_LOG(owner->user_id, "init chapter list\t[%u %u %u]", info.chapter_id, info.star, info.bag_stat);
	}

	return 0;
}

int
InstanceManager::get_chapter_id(uint32_t instance_id)
{
	uint32_t chapter_id = instance_id / 100;

	return chapter_id;
}

const chapter_info_t *
InstanceManager::get_chapter_info(uint32_t chapter_id)
{
	ChapterMap::iterator it = chapter_map.find(chapter_id);
	if (it != chapter_map.end()) {
		return &(it->second);
	}

	return 0;
}

bool 
InstanceManager::check_chapter_is_completed(uint32_t chapter_id)
{
	uint32_t last_instance_id = instance_xml_mgr->get_chapter_last_instance_id(chapter_id);
	if (last_instance_id) {
		const instance_cache_info_t *p_info = this->get_instance_cache_info(last_instance_id);
		if (p_info) {
			return true;
		}
	}

	return false;
}

int
InstanceManager::update_instance_chapter_info(uint32_t instance_id)
{
	uint32_t chapter_id = get_chapter_id(instance_id);

	ChapterMap::iterator it = chapter_map.find(chapter_id);
	if (it == chapter_map.end()) {
		chapter_info_t info = {};
		info.chapter_id = chapter_id;
		info.star = 1;
		info.bag_stat = 0;
		chapter_map.insert(ChapterMap::value_type(chapter_id, info));
	} else {
		it->second.star++;
	}

	const chapter_info_t *p_info = get_chapter_info(chapter_id);
	if (!p_info) {
		return 0;
	}

	//更新DB
	db_set_chapter_info_in db_in;
	db_in.chapter_info.chapter_id = chapter_id;
	db_in.chapter_info.star = p_info->star;
	db_in.chapter_info.bag_stat = p_info->bag_stat;
	send_msg_to_dbroute(0, db_set_chapter_info_cmd, &db_in, owner->user_id);

	//通知前端 
	cli_instance_chapter_info_change_noti_out noti_out;
	noti_out.chapter_info.chapter_id = chapter_id;
	noti_out.chapter_info.star = p_info->star;
	const instance_chapter_xml_info_t *p_chapter_xml_info = instance_chapter_xml_mgr->get_instance_chapter_xml_info(chapter_id);
	if (p_chapter_xml_info) {
		for (int i = 0 ; i < 3; i++) {
			uint32_t bag_id = p_chapter_xml_info->bag[i];
			const instance_bag_xml_info_t *p_bag_xml_info = instance_bag_xml_mgr->get_instance_bag_xml_info(bag_id);
			if (p_bag_xml_info) {
				cli_chapter_bag_info_t bag_info;
				bag_info.bag_id = bag_id;
				bag_info.is_can = p_info->star >= p_bag_xml_info->star ? 1 : 0;
				bag_info.get_flag = test_bit_on(p_info->bag_stat, i + 1);
				noti_out.chapter_info.bag_list.push_back(bag_info);
			}
		}
	}
	owner->send_to_self(cli_instance_chapter_info_change_noti_cmd, &noti_out, 0);

	if (check_chapter_is_completed(chapter_id)) {//副本章节已完成
		cli_chapter_completed_noti_out noti_out;
		noti_out.chapter_id = chapter_id;
		owner->send_to_self(cli_chapter_completed_noti_cmd, &noti_out, 0);
	}

	T_KDEBUG_LOG(owner->user_id, "UPDATE INSTANCE CHAPTER INFO\t[chapter_id=%u, star=%u]", p_info->chapter_id, p_info->star);

	return 0;
}

int
InstanceManager::set_chapter_bag_stat(uint32_t chapter_id, uint32_t bag_stat)
{
	ChapterMap::iterator it = chapter_map.find(chapter_id);
	if (it == chapter_map.end()) {
		return 0;
	}

	it->second.bag_stat = bag_stat;

	//更新DB
	db_update_chapter_bag_stat_in db_in;
	db_in.chapter_id = chapter_id;
	db_in.bag_stat = bag_stat;
	send_msg_to_dbroute(0, db_update_chapter_bag_stat_cmd, &db_in, owner->user_id);

	return 0;
}

int
InstanceManager::get_chapter_bag_reward(uint32_t chapter_id, uint32_t bag_id)
{
	const chapter_info_t *p_info = get_chapter_info(chapter_id);
	if (!p_info) {
		T_KWARN_LOG(owner->user_id, "get chapter bag reward err, chapter not exists\t[chapter_id=%u]", chapter_id);
		return cli_instance_chapter_not_reach_err;
	}

	const instance_chapter_xml_info_t *p_chapter_xml_info = instance_chapter_xml_mgr->get_instance_chapter_xml_info(chapter_id);
	if (!p_chapter_xml_info) {
		T_KWARN_LOG(owner->user_id, "get chapter bag reward err, chapter config not exists\t[chapter_id=%u]", chapter_id);
		return cli_instance_chapter_config_not_exist_err;
	}

	int i = 0;
	for (; i < 3; i++) {
		if (p_chapter_xml_info->bag[i] == bag_id) {
			break;
		}
	}
	if (i == 3) {
		T_KWARN_LOG(owner->user_id, "get chapter bag reward err, chapter bag id not exists\t[chapter_id=%u, bag_id=%u]", chapter_id, bag_id);
		return cli_instance_chapter_bag_id_not_exist_err;
	}

	const instance_bag_xml_info_t *p_bag_xml_info = instance_bag_xml_mgr->get_instance_bag_xml_info(bag_id);
	if (!p_bag_xml_info) {
		T_KWARN_LOG(owner->user_id, "get chapter bag reward err, chapter bag id config not exists\t[chapter_id=%u, bag_id=%u]", chapter_id, bag_id);
		return cli_instance_chapter_bag_id_config_not_exist_err;
	}

	if (p_info->star < p_bag_xml_info->star) {
		T_KWARN_LOG(owner->user_id, "get chapter bag reward err, star not enough\t[chapter_id=%u, bag_id=%u, star=%u]", chapter_id, bag_id, p_info->star);
		return cli_instance_chapter_bag_reward_star_not_enough_err;
	}

	if (test_bit_on(p_info->bag_stat, i + 1)) {
		T_KWARN_LOG(owner->user_id, "get chapter bag reward err, reward already gotted\t[chapter_id=%u, bag_id=%u, stat=%u]", chapter_id, bag_id, p_info->bag_stat);
		return cli_instance_chapter_bag_reward_already_gotted_err;
	}

	cli_send_get_common_bonus_noti_out noti_out;
	//发送奖励
	if (p_bag_xml_info->golds) {
		owner->chg_golds(p_bag_xml_info->golds);
		noti_out.golds = p_bag_xml_info->golds;
	}

	if (p_bag_xml_info->diamond) {
		owner->chg_diamond(p_bag_xml_info->diamond);
		noti_out.diamond = p_bag_xml_info->diamond;
	}

	for (int j = 0; j < 3; j++) {
		if (p_bag_xml_info->items[j][0] && p_bag_xml_info->items[j][1]) {
			owner->items_mgr->add_reward(p_bag_xml_info->items[j][0], p_bag_xml_info->items[j][1]);
			owner->items_mgr->pack_give_items_info(noti_out.give_items_info, p_bag_xml_info->items[j][0], p_bag_xml_info->items[j][1]);
		}
	}

	//设置状态位
	uint32_t bag_stat = set_bit_on(p_info->bag_stat, i + 1);
	set_chapter_bag_stat(chapter_id, bag_stat);

	//发送奖励通知
	owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	return 0;
}

int
InstanceManager::get_instance_battle_left_tms(uint32_t instance_id)
{
	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		return 0;
	}
	if (instance_id < 100) {
		instance_cache_info_t *p_info = get_instance_cache_info(instance_id);
		if (p_info) {
			uint32_t left_tms = p_info->daily_tms < p_xml_info->daily_res ? p_xml_info->daily_res - p_info->daily_tms : 0;
			return left_tms;
		}
	}
	uint32_t instance[3] = {}; 
	instance[0] = instance_id / 100 * 100 + instance_id % 10;
	instance[1] = instance[0] + 10;
	instance[2] = instance[0] + 20;
	
	uint32_t daily_tms = 0;
	for (int i = 0; i < 3; i++) {
		instance_cache_info_t *p_info = get_instance_cache_info(instance[i]);
		if (p_info) {
			daily_tms += p_info->daily_tms;
		}
	}

	uint32_t left_tms = daily_tms < p_xml_info->daily_res ? p_xml_info->daily_res - daily_tms : 0;

	return left_tms;
}

int
InstanceManager::check_instance_battle(uint32_t instance_id)
{
	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "invalid instance id, id=%u", instance_id);
		return cli_invalid_instance_id_err;
	}

	if (p_xml_info->pre_instance) {
		const instance_cache_info_t *p_info = this->get_instance_cache_info(p_xml_info->pre_instance);
		if (!p_info) {
			T_KWARN_LOG(owner->user_id, "pre instance not completed\t[pre_instace=%u, instace_id=%u]", p_xml_info->pre_instance, instance_id);
			return cli_pre_instance_not_completed_err;
		}
	}

	if (owner->lv < p_xml_info->need_lv) {
		T_KWARN_LOG(owner->user_id, "instance need lv not enough\t[lv=%u, need_lv=%u]", owner->lv, p_xml_info->need_lv);
		return cli_instance_need_lv_not_enough_err;
	}

	if (owner->energy < p_xml_info->energy) {
		T_KWARN_LOG(owner->user_id, "instance need energy not enough\t[energy=%u, need_energy=%u]", owner->energy, p_xml_info->energy);
		return cli_not_enough_energy_err;
	}

	uint32_t left_tms = get_instance_battle_left_tms(instance_id);
	if (!left_tms) {
		T_KWARN_LOG(owner->user_id, "instance battle request tms not enough\t[instance_id=%u]", instance_id);
		return cli_instance_battle_request_tms_not_enough_err;
	}
	
	return 0;
}

bool
InstanceManager::check_instance_battle_is_win(uint32_t instance_id, uint32_t time, uint32_t win, uint32_t lose)
{
	return win ? true : false;
	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		return false;
	}

	if (time > p_xml_info->limit_time) {
		return false;
	}

	if (lose) {
		if ((p_xml_info->lose == 3 && lose <= p_xml_info->lose) || lose == p_xml_info->lose) {
			return false;
		}
	}

	if (!win ) {
		return false;
	} else {
		if (!((p_xml_info->win == 3 && win <= p_xml_info->win) || win == p_xml_info->win)) {
			return false;
		}
	}
	
	return true;
}

void
InstanceManager::pack_instance_lose_info(cli_instance_battle_end_out& cli_out)
{
	cli_out.exp = 0;
	cli_out.golds = 0;

	//先打包主公信息
	Hero *p_role_hero = owner->hero_mgr->get_hero(owner->role_id);
	if (p_role_hero) {
		cli_instance_hero_reward_info_t info;
		info.id = p_role_hero->id;
		info.old_lv = p_role_hero->lv;
		info.new_lv = p_role_hero->lv;
		info.add_exp = 0;
		info.exp = p_role_hero->exp;
		info.levelup_exp = p_role_hero->get_level_up_exp(p_role_hero->lv);

		cli_out.heros_info.push_back(info);
	}

	//英雄信息
	std::set<uint32_t>::iterator it = heros.begin();
	for (; it != heros.end(); ++it) {
		Hero *p_hero = owner->hero_mgr->get_hero(*it);
		if (!p_hero || !p_hero->state || p_hero->id == owner->role_id) {
			continue;
		}
		cli_instance_hero_reward_info_t info;
		info.id = *it;
		info.old_lv = p_hero->lv;
		info.new_lv = p_hero->lv;
		info.exp = 0;
		info.levelup_exp = p_hero->get_level_up_exp(p_hero->lv);
		info.honor = 0;
		cli_out.heros_info.push_back(info);
	}

	//小兵信息
	std::set<uint32_t>::iterator it2 = soldiers.begin();
	for (; it2 != soldiers.end(); ++it2) {
		Soldier *p_soldier = owner->soldier_mgr->get_soldier(*it2);
		if (!p_soldier || !p_soldier->state) {
			continue;
		}
		cli_instance_hero_reward_info_t info;
		info.id = *it;
		info.old_lv = p_soldier->lv;
		info.new_lv = p_soldier->lv;
		info.exp = 0;
		info.levelup_exp = p_soldier->get_level_up_exp();
		info.honor = 0;
		cli_out.soldiers_info.push_back(info);
	}
}

void
InstanceManager::pack_instance_rewards_info(std::vector<cli_item_info_t> &items)
{
	for (uint32_t i = 0; i < first_rewards.size(); i++) {
		const item_info_t *p_item_info = &(first_rewards[i]);
		if (p_item_info) {
			cli_item_info_t info;
			info.item_id = p_item_info->id;
			info.item_cnt = p_item_info->cnt;
			items.push_back(info);
		}
	}

	for (uint32_t i = 0; i < random_rewards.size(); i++) {
		const item_info_t *p_item_info = &(random_rewards[i]);
		if (p_item_info) {
			cli_item_info_t info;
			info.item_id = p_item_info->id;
			info.item_cnt = p_item_info->cnt;
			items.push_back(info);
		}
	}
}

int
InstanceManager::give_instance_battle_role_reward(uint32_t instance_id, cli_instance_battle_end_out& cli_out)
{
	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		return -1;
	}

	//先打包主公信息
	Hero *p_role_hero = owner->hero_mgr->get_hero(owner->role_id);
	if (p_role_hero) {
		cli_instance_hero_reward_info_t info;
		info.id = p_role_hero->id;
		info.old_lv = p_role_hero->lv;
		uint32_t add_role_exp = p_xml_info->energy * p_role_hero->lv * 2;
		p_role_hero->add_exp(add_role_exp);
		//加完经验后
		info.new_lv = p_role_hero->lv;
		info.add_exp = add_role_exp;
		info.exp = p_role_hero->exp;
		info.levelup_exp = p_role_hero->get_level_up_exp(p_role_hero->lv);

		cli_out.heros_info.push_back(info);
		cli_out.exp = add_role_exp;
	}

	return 0;
}

int
InstanceManager::give_instance_battle_heros_reward(uint32_t instance_id, uint32_t kill_hero_id, cli_instance_battle_end_out& cli_out)
{
	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		return -1;
	}
	const instance_drop_xml_info_t * p_drop_xml_info = instance_drop_xml_mgr->get_instance_drop_xml_info(p_xml_info->drop_id);
	if (!p_drop_xml_info) {
		return -1;
	}
	//英雄信息
	instance_cache_info_t *p_info = this->get_instance_cache_info(instance_id);
	const troop_info_t *p_troop_info = owner->get_troop(em_troop_type_instance);
	if (!p_troop_info) {
		return 0;
	}
	uint32_t hero_cnt = p_troop_info->heros.size();
	uint32_t add_exp = 0;
	if (hero_cnt) {
		add_exp = ceil((double)p_drop_xml_info->hero_exp / hero_cnt);
	}
	
	for (uint32_t i =0; i < hero_cnt; i++) {
		uint32_t hero_id = p_troop_info->heros[i];
		Hero *p_hero = owner->hero_mgr->get_hero(hero_id);
		if (!p_hero || p_hero->id == owner->role_id) {
			continue;
		}
		cli_instance_hero_reward_info_t info;
		info.id = hero_id;
		info.old_lv = p_hero->lv;
		p_hero->add_exp(add_exp);
		//加完经验后
		info.new_lv = p_hero->lv;
		info.add_exp = p_drop_xml_info->hero_exp;
		info.exp = add_exp;
		info.levelup_exp = p_hero->get_level_up_exp(p_hero->lv);

		//添加武将战功
		if (!p_info) {//first time TODO
			if (kill_hero_id / 1000 == 1) {//英雄ID
				if (kill_hero_id == p_hero->id) {
					p_hero->add_honor(p_drop_xml_info->hero_honor);
					info.honor = p_drop_xml_info->hero_honor;
				}
			} else {//没有kill_hero_id则战功所有武将平分
				uint32_t add_honor = hero_cnt ? p_drop_xml_info->hero_honor / hero_cnt : 0;
				p_hero->add_honor(add_honor);
				info.honor = add_honor;
			}
		}
		
		cli_out.heros_info.push_back(info);
	}

	return 0;
}

int
InstanceManager::give_instance_battle_soldiers_reward(uint32_t instance_id, cli_instance_battle_end_out& cli_out)
{
	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		return -1;
	}
	const instance_drop_xml_info_t * p_drop_xml_info = instance_drop_xml_mgr->get_instance_drop_xml_info(p_xml_info->drop_id);
	if (!p_drop_xml_info) {
		return -1;
	}
	const troop_info_t *p_troop_info = owner->get_troop(em_troop_type_instance);
	if (!p_troop_info) {
		return 0;
	}

	uint32_t soldier_cnt = p_troop_info->soldiers.size();
	uint32_t add_exp = 0;
	if (soldier_cnt) {
		add_exp = ceil((double)p_drop_xml_info->soldier_exp / soldier_cnt);
	}
	for (uint32_t i = 0; i < soldier_cnt; i++) {
		uint32_t soldier_id = p_troop_info->soldiers[i];
		Soldier *p_soldier = owner->soldier_mgr->get_soldier(soldier_id);
		if (!p_soldier) {
			continue;
		}
		cli_instance_hero_reward_info_t info;
		info.id = soldier_id;
		info.old_lv = p_soldier->lv;
		p_soldier->add_exp(add_exp);
		//加完经验后
		info.new_lv = p_soldier->lv;
		info.add_exp = add_exp;
		info.exp = p_soldier->exp;
		info.levelup_exp = p_soldier->get_level_up_exp();

		cli_out.soldiers_info.push_back(info);
	}

	return 0;
}

int
InstanceManager::give_instance_battle_first_reward(uint32_t instance_id, cli_instance_battle_end_out& cli_out)
{
	//首次奖励
	instance_cache_info_t *p_info = this->get_instance_cache_info(instance_id);
	if (p_info) {
		return -1;
	}

	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		return -1;
	}

	const instance_drop_xml_info_t * p_drop_xml_info = instance_drop_xml_mgr->get_instance_drop_xml_info(p_xml_info->drop_id);
	if (!p_drop_xml_info) {
		return -1;
	}

	for (uint32_t i = 0; i < first_rewards.size(); i++) {
		item_info_t *p_item_info = &(first_rewards[i]);
		if (p_item_info->id && p_item_info->cnt) {
			owner->items_mgr->add_reward(p_item_info->id, p_item_info->cnt);
			cli_item_info_t info;
			info.item_id = p_item_info->id;
			info.item_cnt = p_item_info->cnt;
			cli_out.give_items_info.push_back(info);
		}
	}

	first_rewards.clear();

	//更新缓存
	instance_cache_info_t info = {};
	info.instance_id = instance_id;
	info.daily_tms = 1;
	cache_map.insert(InstanceCacheMap::value_type(instance_id, info));

	//更新章节星数
	update_instance_chapter_info(instance_id);

	return 0;
}

int
InstanceManager::give_instance_battle_random_reward(uint32_t drop_id, cli_instance_battle_end_out& cli_out)
{
	const instance_drop_xml_info_t * p_drop_xml_info = instance_drop_xml_mgr->get_instance_drop_xml_info(drop_id);
	if (!p_drop_xml_info) {
		return -1;
	}

	//随机奖励
	for (uint32_t i = 0; i < random_rewards.size(); i++) {
		item_info_t *p_item_info = &(random_rewards[i]);
		if (p_item_info->id && p_item_info->cnt) {
			owner->items_mgr->add_item_without_callback(p_item_info->id, p_item_info->cnt);
			cli_item_info_t info;
			info.item_id = p_item_info->id;
			info.item_cnt = p_item_info->cnt;
			cli_out.give_items_info.push_back(info);
		}
	}

	random_rewards.clear();

	return 0;
}

int
InstanceManager::update_instance_db_info(uint32_t instance_id)
{
	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		return -1;
	}
	instance_cache_info_t *p_info = get_instance_cache_info(instance_id);
	if (!p_info) {
		return 0;
	}

	//更新DB
	db_set_instance_info_in db_in;
	db_in.instance_info.instance_id = instance_id;
	db_in.instance_info.daily_res = p_info->daily_tms;

	send_msg_to_dbroute(0, db_set_instance_info_cmd, &db_in, owner->user_id);

	//通知前端
	cli_instance_info_change_noti_out noti_out;
	noti_out.instance_info.instance_id = instance_id;
	noti_out.instance_info.daily_tms = p_info->daily_tms;
	owner->send_to_self(cli_instance_info_change_noti_cmd, &noti_out, 0);

	return 0;
}

int
InstanceManager::give_instance_battle_reward(uint32_t instance_id, uint32_t kill_hero_id, cli_instance_battle_end_out& cli_out)
{
	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		return -1;
	}

	const instance_drop_xml_info_t * p_drop_xml_info = instance_drop_xml_mgr->get_instance_drop_xml_info(p_xml_info->drop_id);
	if (!p_drop_xml_info) {
		return -1;
	}

	//扣除体力
	owner->chg_energy(-p_xml_info->energy);

	//金币 经验
	owner->chg_golds(p_drop_xml_info->golds);
	cli_out.golds = p_drop_xml_info->golds;

	//主公奖励
	give_instance_battle_role_reward(instance_id, cli_out);
	
	//武将奖励
	give_instance_battle_heros_reward(instance_id, kill_hero_id, cli_out);

	//小兵奖励
	give_instance_battle_soldiers_reward(instance_id, cli_out);

	//首次通关奖励
	give_instance_battle_first_reward(instance_id, cli_out);

	//随机奖励
	give_instance_battle_random_reward(p_xml_info->drop_id, cli_out);

	//更新DB
	update_instance_db_info(instance_id);

	T_KDEBUG_LOG(owner->user_id, "GIVE INSTANCE BATTLE REWARD\t[golds=%u, exp=%u]", cli_out.golds, cli_out.exp);
	
	return 0;
}

int
InstanceManager::send_instance_clean_up_reward(uint32_t instance_id)
{
	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		return -1;
	}
	const instance_drop_xml_info_t * p_drop_xml_info = instance_drop_xml_mgr->get_instance_drop_xml_info(p_xml_info->drop_id);
	if (!p_drop_xml_info) {
		return -1;
	}
	cli_send_get_common_bonus_noti_out noti_out;
	owner->chg_golds(p_drop_xml_info->golds);
	noti_out.golds = p_drop_xml_info->golds;
	//随机奖励
	for (uint32_t i = 0; i < 3; i++) {
		const instance_drop_prob_item_info_t *p_item_info = &(p_drop_xml_info->random_items[i]);
		if (p_item_info->item_id && p_item_info->prob) {
			uint32_t r = rand() % 100;
			if (r < p_item_info->prob) {
				uint32_t item_id = p_item_info->item_id;
				uint32_t item_cnt = p_item_info->item_cnt;
				owner->items_mgr->add_item_without_callback(item_id, item_cnt);
				owner->items_mgr->pack_give_items_info(noti_out.give_items_info, item_id, item_cnt);
			}
		}
	}

	//发送通知
	owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);
	
	return 0;
}

int
InstanceManager::instance_clean_up(uint32_t instance_id)
{
	int ret = check_instance_battle(instance_id);
	if (ret) {
		return ret;
	}
	const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(instance_id);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "invalid instance id, id=%u", instance_id);
		return cli_invalid_instance_id_err;
	}

	instance_cache_info_t *p_info = get_instance_cache_info(instance_id);
	if (!p_info) {
		T_KWARN_LOG(owner->user_id, "instance cannot clean up, id=%u", instance_id);
		return cli_instance_cannot_clean_up_err;
	}

	//检查扫荡券
	uint32_t item_cnt = owner->items_mgr->get_item_cnt(103000);
	if (!item_cnt) {
		T_KWARN_LOG(owner->user_id, "instance clean up ticket not enough err");
		return cli_instance_clean_up_ticket_not_enough_err;
	}

	//扣除体力
	owner->chg_energy(-p_xml_info->energy, 0, 1);

	//扣除扫荡券
	owner->items_mgr->del_item_without_callback(103000, 1);

	//发送奖励
	send_instance_clean_up_reward(instance_id);

	//更新副本信息
	p_info->daily_tms++;

	update_instance_db_info(instance_id);

	return 0;
}

void
InstanceManager::pack_instance_list_info(cli_get_instance_list_out &cli_out)
{
	InstanceCacheMap::iterator it = cache_map.begin();
	for (; it != cache_map.end(); ++it) {
		instance_cache_info_t *p_info = &(it->second);
		cli_instance_info_t info;
		info.instance_id = p_info->instance_id;
		const instance_xml_info_t *p_xml_info = instance_xml_mgr->get_instance_xml_info(info.instance_id);
		if (p_xml_info) {
			info.daily_tms = p_info->daily_tms;
		} else {
			info.daily_tms = 0;
		}
		cli_out.instance_info.push_back(info);
	}
}

void
InstanceManager::pack_client_instance_chapter_list(cli_get_instance_chapter_list_out &cli_out)
{
	ChapterMap::const_iterator it = chapter_map.begin();
	for (; it != chapter_map.end(); ++it) {
		const chapter_info_t *p_info = &(it->second);
		cli_chapter_info_t info;
		info.chapter_id = p_info->chapter_id;
		info.star = p_info->star;
		const instance_chapter_xml_info_t *p_xml_info = instance_chapter_xml_mgr->get_instance_chapter_xml_info(p_info->chapter_id);
		if (p_xml_info) {
			for (int i = 0; i < 3; i++) {
				uint32_t bag_id = p_xml_info->bag[i];
				if (bag_id > 0) {
					const instance_bag_xml_info_t *p_bag_xml_info = instance_bag_xml_mgr->get_instance_bag_xml_info(bag_id);
					if (p_bag_xml_info) {
						cli_chapter_bag_info_t bag_info;
						bag_info.bag_id = bag_id;
						bag_info.is_can = p_info->star >= p_bag_xml_info->star ? 1 : 0;
						bag_info.get_flag = test_bit_on(p_info->bag_stat, i+1);
						info.bag_list.push_back(bag_info);
					}
				}
			}
		}
		cli_out.chapter_list.push_back(info);

		T_KTRACE_LOG(owner->user_id, "pack client instance chapter list\t[%u %u]", info.chapter_id, info.star);
	}
}

/********************************************************************************/
/*						InstanceXmlManager Class								*/
/********************************************************************************/
InstanceXmlManager::InstanceXmlManager()
{

}

InstanceXmlManager::~InstanceXmlManager()
{

}

int
InstanceXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_instance_xml_info(cur); 
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);
	return ret;
}

int
InstanceXmlManager::load_instance_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("instance"))) {
			uint32_t fb_id = 0;
			get_xml_prop(fb_id, cur, "fb_id");
			InstanceXmlMap::iterator it = instance_xml_map.find(fb_id);
			if (it != instance_xml_map.end()) {
				ERROR_LOG("load instance xml info err, fb_id exists, fb_id=%u", fb_id);
				return -1;
			}
			instance_xml_info_t info;
			info.id = fb_id;
			get_xml_prop_def(info.pre_instance, cur, "pre_fb_id", 0);
			get_xml_prop_def(info.need_lv, cur, "need_lv", 0);
			get_xml_prop_def(info.daily_res, cur, "daily_res", 0);
			get_xml_prop_def(info.limit_time, cur, "limit_time", 0);
			get_xml_prop_def(info.lose, cur, "lose", 0);
			get_xml_prop_def(info.win, cur, "win", 0);
			get_xml_prop_def(info.enemy_power, cur, "enmey_power", 0);
			get_xml_prop_def(info.drop_id, cur, "drop_id", 0);
			get_xml_prop_def(info.energy, cur, "energy", 0);

			TRACE_LOG("load instance xml info\t[%u %u %u %u %u %u %u %u %u %u]", 
					info.id, info.pre_instance, info.need_lv, info.daily_res, info.limit_time,
					info.lose, info.win, info.enemy_power, info.drop_id, info.energy);

			instance_xml_map.insert(InstanceXmlMap::value_type(fb_id, info));
		}
		cur = cur->next;
	}
	return 0;
}

const instance_xml_info_t *
InstanceXmlManager::get_instance_xml_info(uint32_t instance_id)
{
	InstanceXmlMap::iterator it = instance_xml_map.find(instance_id);
	if (it != instance_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

uint32_t 
InstanceXmlManager::get_chapter_last_instance_id(uint32_t chapter_id)
{
	uint32_t max_id = 0;
	InstanceXmlMap::iterator it = instance_xml_map.begin();
	for (; it != instance_xml_map.end(); ++it) {
		if (it->second.id / 100 == chapter_id + 9) {
			if (it->second.id > max_id) {
				max_id = it->second.id;
			}
		}
	}

	return max_id;
}


/********************************************************************************/
/*							MonsterXmlManager Class								*/
/********************************************************************************/
MonsterXmlManager::MonsterXmlManager()
{

}

MonsterXmlManager::~MonsterXmlManager()
{

}

int
MonsterXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_monster_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;

}

int
MonsterXmlManager::load_monster_xml_info(xmlNodePtr cur)
{
	return 0;
}

const monster_xml_info_t*
MonsterXmlManager::get_monster_xml_info(uint32_t monster_id)
{
	MonsterXmlMap::iterator it = monster_xml_map.find(monster_id);
	if (it != monster_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

/********************************************************************************/
/*						InstanceDropXmlManager Class							*/
/********************************************************************************/
InstanceDropXmlManager::InstanceDropXmlManager()
{

}

InstanceDropXmlManager::~InstanceDropXmlManager()
{

}

int
InstanceDropXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_instance_drop_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
InstanceDropXmlManager::load_instance_drop_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("drop"))) {
			uint32_t drop_id = 0;
			get_xml_prop(drop_id, cur, "drop_id");
			InstanceDropXmlMap::iterator it = instance_drop_xml_map.find(drop_id);
			if (it != instance_drop_xml_map.end()) {
				ERROR_LOG("load instance drop xml info err, drop_id exists, drop_id=%u", drop_id);
				return -1;
			}

			instance_drop_xml_info_t info;
			info.id = drop_id;
			get_xml_prop_def(info.golds, cur, "pass_coin", 0);
			get_xml_prop_def(info.hero_exp, cur, "pass_exp", 0);
			get_xml_prop_def(info.soldier_exp, cur, "soldier_exp", 0);
			get_xml_prop_def(info.hero_honor, cur, "honor", 0);
			get_xml_prop_def(info.repeat_num, cur, "repeat_num", 0);

			get_xml_prop_def(info.first_items[0].item_id, cur, "first_item_1", 0);
			get_xml_prop_def(info.first_items[0].item_cnt, cur, "first_item_1_num", 0);
			get_xml_prop_def(info.first_items[1].item_id, cur, "first_item_2", 0);
			get_xml_prop_def(info.first_items[1].item_cnt, cur, "first_item_2_num", 0);
			get_xml_prop_def(info.first_items[2].item_id, cur, "first_item_3", 0);
			get_xml_prop_def(info.first_items[2].item_cnt, cur, "first_item_3_num", 0);
			get_xml_prop_def(info.first_items[3].item_id, cur, "first_item_4", 0);
			get_xml_prop_def(info.first_items[3].item_cnt, cur, "first_item_4_num", 0);


			get_xml_prop_def(info.random_items[0].item_id, cur, "random_item_1", 0);
			get_xml_prop_def(info.random_items[0].item_cnt, cur, "random_item_1_num", 0);
			get_xml_prop_def(info.random_items[0].prob, cur, "random_item_1_chance", 0);
			get_xml_prop_def(info.random_items[1].item_id, cur, "random_item_2", 0);
			get_xml_prop_def(info.random_items[1].item_cnt, cur, "random_item_2_num", 0);
			get_xml_prop_def(info.random_items[1].prob, cur, "random_item_2_chance", 0);
			get_xml_prop_def(info.random_items[2].item_id, cur, "random_item_3", 0);
			get_xml_prop_def(info.random_items[2].item_cnt, cur, "random_item_3_num", 0);
			get_xml_prop_def(info.random_items[2].prob, cur, "random_item_3_chance", 0);
			get_xml_prop_def(info.random_items[3].item_id, cur, "random_item_4", 0);
			get_xml_prop_def(info.random_items[3].item_cnt, cur, "random_item_4_num", 0);
			get_xml_prop_def(info.random_items[3].prob, cur, "random_item_4_chance", 0);
			get_xml_prop_def(info.random_items[4].item_id, cur, "random_item_5", 0);
			get_xml_prop_def(info.random_items[4].item_cnt, cur, "random_item_5_num", 0);
			get_xml_prop_def(info.random_items[4].prob, cur, "random_item_5_chance", 0);

			TRACE_LOG("load instance drop xml info\t[%u %u %u %u %u %u]", 
					info.id, info.golds, info.hero_exp, info.soldier_exp, info.hero_honor, info.repeat_num);

			instance_drop_xml_map.insert(InstanceDropXmlMap::value_type(drop_id, info));
		}
		cur = cur->next;
	}

	return 0;
}

const instance_drop_xml_info_t*
InstanceDropXmlManager::get_instance_drop_xml_info(uint32_t drop_id)
{
	InstanceDropXmlMap::iterator it = instance_drop_xml_map.find(drop_id);
	if (it != instance_drop_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

/********************************************************************************/
/*							InstanceChapterXmlManager Class							*/
/********************************************************************************/
InstanceChapterXmlManager::InstanceChapterXmlManager()
{

}

InstanceChapterXmlManager::~InstanceChapterXmlManager()
{

}

const instance_chapter_xml_info_t*
InstanceChapterXmlManager::get_instance_chapter_xml_info(uint32_t chapter_id)
{
	InstanceChapterXmlMap::iterator it = instance_chapter_xml_map.find(chapter_id);
	if (it != instance_chapter_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
InstanceChapterXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_instance_chapter_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
InstanceChapterXmlManager::load_instance_chapter_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("chapter"))) {
			uint32_t chapter_id = 0;
			get_xml_prop(chapter_id, cur, "chapter");
			InstanceChapterXmlMap::iterator it = instance_chapter_xml_map.find(chapter_id);
			if (it != instance_chapter_xml_map.end()) {
				ERROR_LOG("load instance chapter xml info err, chapter_id=%u", chapter_id);
				return -1;
			}

			instance_chapter_xml_info_t info = {};
			info.chapter_id = chapter_id;
			get_xml_prop_def(info.bag[0], cur, "bag1", 0);
			get_xml_prop_def(info.bag[1], cur, "bag2", 0);
			get_xml_prop_def(info.bag[2], cur, "bag3", 0);

			TRACE_LOG("load instacne chapter xml info\t[%u %u %u %u]", info.chapter_id, info.bag[0], info.bag[1], info.bag[2]);

			instance_chapter_xml_map.insert(InstanceChapterXmlMap::value_type(chapter_id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							InstanceBagXmlManager Class							*/
/********************************************************************************/
InstanceBagXmlManager::InstanceBagXmlManager()
{

}

InstanceBagXmlManager::~InstanceBagXmlManager()
{

}

int
InstanceBagXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_instance_bag_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

const instance_bag_xml_info_t* 
InstanceBagXmlManager::get_instance_bag_xml_info(uint32_t bag_id)
{
	InstanceBagXmlMap::iterator it = instance_bag_xml_map.find(bag_id);
	if (it != instance_bag_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
InstanceBagXmlManager::load_instance_bag_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("bag"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			InstanceBagXmlMap::iterator it = instance_bag_xml_map.find(id);
			if (it != instance_bag_xml_map.end()) {
				ERROR_LOG("load instance bag xml info err, id=%u", id);
				return -1;
			}

			instance_bag_xml_info_t info = {};
			info.id = id;
			get_xml_prop(info.star, cur, "star");
			get_xml_prop_def(info.items[0][0], cur, "item1", 0);
			get_xml_prop_def(info.items[0][1], cur, "item1_cnt", 0);
			get_xml_prop_def(info.items[1][0], cur, "item2", 0);
			get_xml_prop_def(info.items[1][1], cur, "item2_cnt", 0);
			get_xml_prop_def(info.items[2][0], cur, "item3", 0);
			get_xml_prop_def(info.items[2][1], cur, "item3_cnt", 0);
			get_xml_prop_def(info.golds, cur, "golds", 0);
			get_xml_prop_def(info.diamond, cur, "diamond", 0);

			TRACE_LOG("load instance bag xml info\t[%u %u %u %u %u %u %u %u %u %u]", 
					info.id, info.star, info.items[0][0], info.items[0][1], info.items[1][0], info.items[1][1], info.items[2][0], info.items[2][1], 
					info.golds, info.diamond);

			instance_bag_xml_map.insert(InstanceBagXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 拉取副本信息
 */
int cli_get_instance_list(Player *p, Cmessage *c_in) 
{
	cli_get_instance_list_out cli_out;
	p->instance_mgr->pack_instance_list_info(cli_out);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}


/* @brief 副本战斗请求
 */
int cli_instance_battle_request(Player *p, Cmessage *c_in)
{
	cli_instance_battle_request_in *p_in = P_IN;
	int ret = p->instance_mgr->battle_request(p_in->instance_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	//设置参战英雄 TODO
	for (uint32_t i = 0; i < p_in->heros.size(); i++) {
		Hero *p_hero = p->hero_mgr->get_hero(p_in->heros[i]);
		if (p_hero) {
			p_hero->set_hero_status(1);
		}
	}

	T_KDEBUG_LOG(p->user_id, "INSTANCE BATTLE REQUEST\t[instance_id=%u]", p_in->instance_id);

	cli_instance_battle_request_out cli_out;
	cli_out.instance_id = p_in->instance_id;
	p->instance_mgr->pack_instance_rewards_info(cli_out.give_items_info);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 副本战斗结束
 */
int cli_instance_battle_end(Player *p, Cmessage *c_in)
{
	cli_instance_battle_end_in *p_in = P_IN;

	cli_instance_battle_end_out cli_out;
	int ret = p->instance_mgr->battle_end(p_in->instance_id, p_in->time, p_in->win, p_in->lose, p_in->kill_hero_id, cli_out);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	//触发奇遇
	p->adventure_mgr->trigger_adventure();

	T_KDEBUG_LOG(p->user_id, "INSTANCE BATTLE END\t[instance_id=%u, time=%u, win=%u, lose=%u, kill_hero_id=%u]", 
			p_in->instance_id, p_in->time, p_in->win, p_in->lose, p_in->kill_hero_id);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 设置英雄和小兵出战
 */
int cli_set_heros_and_soldiers_ready_battle(Player *p, Cmessage *c_in)
{
	cli_set_heros_and_soldiers_ready_battle_in *p_in = P_IN;

	p->instance_mgr->clear_heros();
	p->instance_mgr->clear_soldiers();

	for (uint32_t i = 0; i < p_in->heros.size(); i++) {
		uint32_t hero_id = p_in->heros[i];
		Hero *p_hero = p->hero_mgr->get_hero(hero_id);
		if (p_hero) {
			p_hero->set_hero_status(1);
		}
	}

	for (uint32_t i = 0; i < p_in->soldiers.size(); i++) {
		uint32_t soldier_id = p_in->soldiers[i];
		Soldier *p_soldier = p->soldier_mgr->get_soldier(soldier_id);
		if (p_soldier) {
			p_soldier->set_soldier_status(1);
		}
	}

	T_KDEBUG_LOG(p->user_id, "SET HEROS AND SOLDIERS READY BATTLE!");

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 拉取副本章节信息列表
 */
int cli_get_instance_chapter_list(Player *p, Cmessage *c_in)
{
	cli_get_instance_chapter_list_out cli_out;

	p->instance_mgr->pack_client_instance_chapter_list(cli_out);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 领取副本章节宝箱
 */
int cli_get_instance_chapter_bag_reward(Player *p, Cmessage *c_in)
{
	cli_get_instance_chapter_bag_reward_in *p_in = P_IN;

	int ret = p->instance_mgr->get_chapter_bag_reward(p_in->chapter_id, p_in->bag_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_get_instance_chapter_bag_reward_out cli_out;
	cli_out.chapter_id = p_in->chapter_id;
	cli_out.bag_id = p_in->bag_id;

	T_KDEBUG_LOG(p->user_id, "GET INSTANCE CHAPTER BAG REWARD\t[chapter_id=%u, bag_id=%u]", p_in->chapter_id, p_in->bag_id);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 副本扫荡
 */
int cli_instance_clean_up(Player *p, Cmessage *c_in)
{
	cli_instance_clean_up_in *p_in = P_IN;

	int ret = p->instance_mgr->instance_clean_up(p_in->instance_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_instance_clean_up_out cli_out;
	cli_out.instance_id = p_in->instance_id;

	T_KDEBUG_LOG(p->user_id, "INSTANCE CLEAN UP\t[instance_id=%u]", p_in->instance_id);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/********************************************************************************/
/*								DB return										*/
/********************************************************************************/
/* @brief 拉取副本信息列表返回
 */
int db_get_instance_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);

	db_get_instance_list_out *p_in = P_IN;
	if (p->wait_cmd == cli_proto_login_cmd) {
		p->instance_mgr->init_instance_list(p_in->instance_list);

		p->login_step++;

		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u GET INSTANCE LIST", p->login_step);

		//副本信息
		cli_get_instance_list_out cli_out;
		p->instance_mgr->pack_instance_list_info(cli_out);
		p->send_to_self(cli_get_instance_list_cmd, &cli_out, 0);

		//拉取副本章节信息
		return send_msg_to_dbroute(p, db_get_chapter_list_cmd, 0, p->user_id);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}

/* @brief 拉取副本章节信息返回
 */
int db_get_chapter_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);

	db_get_chapter_list_out *p_in = P_IN;
	if (p->wait_cmd == cli_proto_login_cmd) {
		p->instance_mgr->init_chapter_list(p_in->chapter_list);

		p->login_step++;
		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u GET CHAPTER LIST", p->login_step);

		//发给前端
		cli_get_instance_chapter_list_out cli_out;
		p->instance_mgr->pack_client_instance_chapter_list(cli_out);
		p->send_to_self(cli_get_instance_chapter_list_cmd, &cli_out, 0);

		//拉取商城信息
		return send_msg_to_dbroute(p, db_get_shop_list_cmd, 0, p->user_id);

		//登陆完成，发送信息给客户端
		//return p->send_login_info_to_client();
	} 

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}
