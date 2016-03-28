/*
 * =====================================================================================
 *
 *  @file  arena.cpp 
 *
 *  @brief  竞技场系统
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
#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"

extern "C" {
#include <math.h>
}

#include "global_data.hpp"
#include "arena.hpp"
#include "dbroute.hpp"
#include "player.hpp"
#include "hero.hpp"
#include "soldier.hpp"
#include "equipment.hpp"
#include "btl_soul.hpp"
*/

#include "stdafx.hpp"

using namespace std;
using namespace project;

//ArenaManager arena_mgr;
//ArenaAttrXmlManager arena_attr_xml_mgr;
//ArenaHeroXmlManager arena_hero_xml_mgr;
//ArenaBonusXmlManager arena_bonus_xml_mgr;
//ArenaLevelAttrXmlManager arena_level_attr_xml_mgr;

/********************************************************************************/
/*							ArenaManager Class									*/
/********************************************************************************/
ArenaManager::ArenaManager()
{
	init_flag = false;
	arena_map.clear();
	ranking_map.clear();
}

ArenaManager::~ArenaManager()
{
	init_flag = false;
	arena_map.clear();
	ranking_map.clear();
}

int
ArenaManager::get_arena_count()
{
	return send_msg_to_dbroute(0, db_get_arena_count_cmd, 0, 0);
}

int
ArenaManager::first_init_arena()
{
	char robot_nick[5][NICK_LEN] = {"Mark", "Frankie", "Rock", "MoMo", "Buono"};
	db_add_arena_list_in db_in;
	for (int i = 1; i <= 10000; i++) {
		const arena_attr_xml_info_t *p_xml_info = arena_attr_xml_mgr->get_arena_attr_xml_info(i);
		if (!p_xml_info) {
			continue;
		}
		db_arena_info_t info;
		info.ranking = i;
		info.user_id = i;
		memset(info.nick, 0, NICK_LEN);
		const char *nick = robot_nick[rand() % 5];
		memcpy(info.nick, nick, strlen(nick));
		info.lv = p_xml_info->lv;
		info.rank = p_xml_info->rank;
		info.hero1 = arena_hero_xml_mgr->random_one_hero(1);
		info.hero2 = arena_hero_xml_mgr->random_one_hero(2);
		info.hero3 = arena_hero_xml_mgr->random_one_hero(3);
		info.soldier1 = 2003;
		info.soldier2 = 2008;
		info.soldier3 = 2013;
		info.btl_power = ranged_random(p_xml_info->min_btl_power, p_xml_info->max_btl_power);

		db_in.arena_list.push_back(info);

		//插入缓存 
		arena_info_t cache_info = {};
		cache_info.ranking = i;
		cache_info.user_id = info.user_id;
		memcpy(cache_info.nick, info.nick, NICK_LEN);
		cache_info.lv = info.lv;
		cache_info.rank = info.rank;
		cache_info.hero[0] = info.hero1;
		cache_info.hero[1] = info.hero2;
		cache_info.hero[2] = info.hero3;
		cache_info.soldier[0] = info.soldier1;
		cache_info.soldier[1] = info.soldier2;
		cache_info.soldier[2] = info.soldier3;
		cache_info.state = 0;
		arena_map.insert(ArenaMap::value_type(info.user_id, cache_info));

		ranking_map.insert(make_pair(info.ranking, info.user_id));
	}

	DEBUG_LOG("FIRST INIT ARENA!");

	return send_msg_to_dbroute(0, db_add_arena_list_cmd, &db_in, 0);
}

int
ArenaManager::init_arena(vector<db_arena_info_t> &arena_list)
{
	for (uint32_t i = 0; i < arena_list.size(); i++) {
		db_arena_info_t *p_info = &(arena_list[i]);
		
		arena_info_t info = {};
		info.ranking = p_info->ranking;
		info.user_id = p_info->user_id;
		memcpy(info.nick, p_info->nick, NICK_LEN);
		info.lv = p_info->lv;
		info.rank = p_info->rank;
		info.hero[0] = p_info->hero1;
		info.hero[1] = p_info->hero2;
		info.hero[2] = p_info->hero3;
		info.soldier[0] = p_info->soldier1;
		info.soldier[1] = p_info->soldier2;
		info.soldier[2] = p_info->soldier3;
		info.btl_power = p_info->btl_power;
		info.state = 0;

		arena_map.insert(ArenaMap::value_type(info.user_id, info));
		ranking_map.insert(make_pair(p_info->ranking, p_info->user_id));

		TRACE_LOG("init arena[%u %u %u]", p_info->ranking, p_info->user_id, p_info->btl_power);
	}

	init_flag = true;

	DEBUG_LOG("INIT ARENA!");
	
	return 0;
}

bool
ArenaManager::check_arena_is_init()
{
	return init_flag;
}

int
ArenaManager::first_add_to_arena(Player *p)
{
	Hero *p_hero = p->hero_mgr->get_hero(p->role_id);
	if (p_hero) {
		arena_info_t info = {};
		info.user_id = p->user_id;
		info.ranking = 0; 
		info.lv = p_hero->lv;
		info.rank = p_hero->rank;
		info.hero[0] = 1001;
		info.hero[1] = 1004;
		info.hero[2] = 1008;
		info.soldier[0] = 2003;
		info.soldier[1] = 2013;
		info.soldier[2] = 2028;
		info.btl_power = p->calc_arena_btl_power(&info);
		add_to_arena(p, &info);
	}

	return 0;
}

int
ArenaManager::add_to_arena(Player *p, arena_info_t *p_info)
{
	ArenaMap::iterator it = arena_map.find(p_info->user_id);
	if (it != arena_map.end()) {
		T_KWARN_LOG(p_info->user_id, "arena user already exist");
		return cli_user_already_add_to_arena_ranking_err;
	}
	
	uint32_t sz = ranking_map.size();

	arena_info_t info = {};
	info.user_id = p_info->user_id;
	memcpy(info.nick, p->nick, NICK_LEN);
	info.ranking = sz + 1;
	info.lv = p_info->lv;
	info.rank = p_info->rank;
	for (int i = 0; i < 3; i++) {
		info.hero[i] = p_info->hero[i];
		info.soldier[i] = p_info->soldier[i];
	}
	info.btl_power = p_info->btl_power;

	ranking_map.insert(make_pair(info.ranking, info.user_id));
	arena_map.insert(ArenaMap::value_type(info.user_id, info));

	//更新DB
	db_add_arena_info_in db_in;
	db_in.arena_info.ranking = info.ranking;
	db_in.arena_info.user_id = info.user_id;
	memcpy(db_in.arena_info.nick, info.nick, NICK_LEN);
	db_in.arena_info.lv = info.lv;
	db_in.arena_info.rank = info.rank;
	db_in.arena_info.hero1 = info.hero[0];
	db_in.arena_info.hero2 = info.hero[1];
	db_in.arena_info.hero3 = info.hero[2];
	db_in.arena_info.soldier1 = info.soldier[0];
	db_in.arena_info.soldier2 = info.soldier[1];
	db_in.arena_info.soldier3 = info.soldier[2];
	db_in.arena_info.btl_power = info.btl_power;
	send_msg_to_dbroute(0, db_add_arena_info_cmd, &db_in, 0);

	//设置历史排名
	p->res_mgr->set_res_value(forever_arena_history_ranking, info.ranking);

	return 0;
}

/* @brief 计算服务器开服时间
 */
int
ArenaManager::calc_serv_day()
{
	//TODO
	return 0;
}

/* @brief 计算机器人等级 */
int 
ArenaManager::calc_robot_lv(uint32_t lv)
{
	uint32_t serv_day = calc_serv_day();
	uint32_t add_lv = 0;
	if (serv_day <= 7) {
		add_lv = serv_day * 1;
	} else {
		add_lv = 7 + (serv_day - 7) / 7;
	}

	uint32_t cur_lv = lv + add_lv;
	if (cur_lv > MAX_HERO_LEVEL) {
		cur_lv = MAX_HERO_LEVEL;
	}

	return cur_lv;
}

/* @brief 计算机器人战斗力
 */
int
ArenaManager::calc_robot_btl_power(uint32_t lv, uint32_t btl_power)
{
	uint32_t cur_lv = calc_robot_lv(lv);
	uint32_t add_lv = cur_lv - lv;
	uint32_t add_btl_power = add_lv * 10000;

	uint32_t cur_btl_power = btl_power + add_btl_power;

	return cur_btl_power;
}

/* @brief 更新竞技场主角等级信息
 */
int
ArenaManager::update_ranking_info_lv(uint32_t user_id, uint32_t lv)
{
	ArenaMap::iterator it = arena_map.find(user_id);
	if (it == arena_map.end()) {
		T_KWARN_LOG(user_id, "update ranking info lv err, user not exist");
	 	return cli_user_not_in_arena_ranking_err;
	}

	it->second.lv = lv;

	//更新DB
	db_update_arena_ranking_info_lv_in db_in;
	db_in.ranking = it->second.ranking;
	db_in.lv = it->second.lv;
	send_msg_to_dbroute(0, db_update_arena_ranking_info_lv_cmd, &db_in, 0);

	return 0;
}

/* @brief 更新竞技场主角品阶信息
 */
int
ArenaManager::update_ranking_info_rank(uint32_t user_id, uint32_t rank)
{
	ArenaMap::iterator it = arena_map.find(user_id);
	if (it == arena_map.end()) {
		T_KWARN_LOG(user_id, "update ranking info rank err, user not exist");
	 	return cli_user_not_in_arena_ranking_err;
	}

	it->second.rank = rank;

	//更新DB
	db_update_arena_ranking_info_rank_in db_in;
	db_in.ranking = it->second.ranking;
	db_in.rank = it->second.rank;
	send_msg_to_dbroute(0, db_update_arena_ranking_info_rank_cmd, &db_in, 0);

	return 0;
}

int
ArenaManager::update_ranking_info_btl_power(Player *p)
{
	if (!p) {
		return 0;
	}
	
	ArenaMap::iterator it = arena_map.find(p->user_id);
	if (it == arena_map.end()) {
		return 0;
	}

	const arena_info_t *p_info = &(it->second);
	uint32_t btl_power = p->calc_arena_btl_power(p_info);

	it->second.btl_power = btl_power;

	//更新DB
	db_update_arena_ranking_info_btl_power_in db_in;
	db_in.ranking = it->second.ranking;
	db_in.btl_power = it->second.btl_power;
	send_msg_to_dbroute(0, db_update_arena_ranking_info_btl_power_cmd, &db_in, 0);

	return 0;
}

bool
ArenaManager::is_defend_hero(uint32_t user_id, uint32_t hero_id)
{
	const arena_info_t *p_info = get_arena_ranking_info_by_userid(user_id);
	if (!p_info) {
		return false;
	}

	for (int i = 0; i < 3; i++) {
		if (hero_id == p_info->hero[i]) {
			return true;
		}
	}

	return false;
}

bool
ArenaManager::is_defend_soldier(uint32_t user_id, uint32_t soldier_id)
{
	const arena_info_t *p_info = get_arena_ranking_info_by_userid(user_id);
	if (!p_info) {
		return false;
	}

	for (int i = 0; i < 3; i++) {
		if (soldier_id == p_info->soldier[i]) {
			return true;
		}
	}

	return false;
}

const arena_info_t *
ArenaManager::get_arena_ranking_info_by_ranking(uint32_t ranking)
{
	std::map<uint32_t, uint32_t>::iterator it = ranking_map.find(ranking);
	if (it == ranking_map.end()) {
		return 0;
	}
	ArenaMap::iterator it2 = arena_map.find(it->second);
	if (it2 != arena_map.end()) {
		return &(it2->second);
	}

	return 0;
}

const arena_info_t *
ArenaManager::get_arena_ranking_info_by_userid(uint32_t user_id)
{
	ArenaMap::iterator it = arena_map.find(user_id);
	if (it != arena_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
ArenaManager::get_arena_ranking_list(uint32_t start, uint32_t end, vector<cli_arena_info_t> &arena_list)
{
	uint32_t sz = arena_map.size();
	if (!start || end > sz || start > end) {
		T_KWARN_LOG(0, "get arena ranking list input arg err\t[start=%u, end=%u]", start, end);
		return cli_invalid_input_arg_err;
	}


	for (uint32_t i = start; i <= end; i++) {
		const arena_info_t *p_info = get_arena_ranking_info_by_ranking(i);
		if (p_info) {
			cli_arena_info_t info;
			info.ranking = i;
			info.user_id = p_info->user_id;
			memcpy(info.nick, p_info->nick, NICK_LEN);
			info.lv = p_info->lv;
			info.rank = p_info->rank;
			info.hero1 = p_info->hero[0];
			info.hero2 = p_info->hero[1];
			info.hero3 = p_info->hero[2];
			info.soldier1 = p_info->soldier[0];
			info.soldier2 = p_info->soldier[1];
			info.soldier3 = p_info->soldier[2];
			info.btl_power = p_info->btl_power;

			arena_list.push_back(info);
		}
	}

	return 0;
}

void
ArenaManager::set_state(uint32_t user_id, int state)
{
	ArenaMap::iterator it = arena_map.find(user_id);
	if (it != arena_map.end()) {
		it->second.state = state;
	}
}

int
ArenaManager::swap_ranking(uint32_t ranking1, uint32_t ranking2)
{
	map<uint32_t, uint32_t>::iterator it1 = ranking_map.find(ranking1);
	map<uint32_t, uint32_t>::iterator it2 = ranking_map.find(ranking2);
	if (it1 == ranking_map.end() || it2 == ranking_map.end()) {
		return 0;
	}

	uint32_t user_id1 = it1->second;
	uint32_t user_id2 = it2->second;
	ArenaMap::iterator it3 = arena_map.find(user_id1);
	ArenaMap::iterator it4 = arena_map.find(user_id2);
	if (it3 == arena_map.end() || it4 == arena_map.end()) {
		return 0;
	}

	it1->second = user_id2;
	it2->second = user_id1;

	it3->second.ranking = ranking2;
	it4->second.ranking = ranking1;

	//更新DB
	db_update_arena_ranking_info_in db_in1;
	pack_arena_ranking_db_info(ranking1, db_in1.ranking_info);
	send_msg_to_dbroute(0, db_update_arena_ranking_info_cmd, &db_in1, 0);

	db_update_arena_ranking_info_in db_in2;
	pack_arena_ranking_db_info(ranking2, db_in2.ranking_info);
	send_msg_to_dbroute(0, db_update_arena_ranking_info_cmd, &db_in2, 0);

	return 0;
}

int
ArenaManager::battle_request(uint32_t user_id, uint32_t ranking)
{
	const arena_info_t *self_info = get_arena_ranking_info_by_userid(user_id);
	const arena_info_t *opp_info = get_arena_ranking_info_by_ranking(ranking);
	if (!self_info || !opp_info) {
		T_KWARN_LOG(user_id, "arena battle request err, ranking=%u", ranking);
		return cli_arena_battle_request_ranking_err;
	}

	if (self_info->ranking < opp_info->ranking) {
		T_KWARN_LOG(user_id, "arena battle request err, opp_raning=%u, ranking=%u", opp_info->ranking, ranking);
		return cli_arena_battle_opp_ranking_is_lower;
	}

	/* TODO
	uint32_t now_sec = get_now_tv()->tv_sec;
	if (self_info->state + 3 * 60 > now_sec) {
		T_KWARN_LOG(user_id, "arena battle request err, self being changllenge");
		return cli_arena_self_being_challenge_err;
	}

	if (opp_info->state + 3 * 60 > now_sec) {
		T_KWARN_LOG(user_id, "arena battle request err, opp being changllenge\t[opp_ranking=%u]", opp_info->ranking);
		return cli_arena_opp_being_challenge_err;
	}

	//设置状态
	set_state(self_info->user_id, now_sec);
	set_state(opp_info->user_id, now_sec);
	*/

	return 0;
}

int
ArenaManager::battle_end(Player *p, uint32_t ranking, uint32_t is_win, uint32_t kill_hero_id, cli_arena_battle_end_out &cli_out)
{
	const arena_info_t *self_info = get_arena_ranking_info_by_userid(p->user_id);
	const arena_info_t *opp_info = get_arena_ranking_info_by_ranking(ranking);
	if (!self_info || !opp_info) {
		T_KWARN_LOG(p->user_id, "arena battle request err, ranking=%u", ranking);
		return cli_arena_battle_request_ranking_err;
	}

	/*
	if (!self_info->state || !opp_info->state) {
		T_KWARN_LOG(user_id, "arena battle end err, ranking=%u", ranking);
		return cli_arena_battle_end_state_err;
	}*/

	//清除state
	set_state(self_info->user_id, 0);
	set_state(opp_info->user_id, 0);

	if (is_win) {//赢了调换排名
		cli_out.ranking = opp_info->ranking;
		swap_ranking(self_info->ranking, opp_info->ranking);
		give_ranking_bonus(p, ranking);
		give_win_honor(p, kill_hero_id);
	}

	//检查任务
	p->task_mgr->check_task(em_task_type_arena);
		
	//检查成就
	p->achievement_mgr->check_achievement(em_achievement_type_arena);

	return 0;
}

int
ArenaManager::give_win_honor(Player *p, uint32_t kill_hero_id)
{
	const troop_info_t* p_troop_info = p->get_troop(em_troop_type_arena);
	if (p_troop_info) {
		uint32_t hero_cnt = p_troop_info->heros.size();
		for (uint32_t i = 0; i < hero_cnt; i++) {
			if (kill_hero_id) {
				if (p_troop_info->heros[i] == kill_hero_id) {
					Hero *p_hero = p->hero_mgr->get_hero(kill_hero_id); 
					if (p_hero){
						p_hero->add_honor(300);//TODO
					}
				}
			} else {
				Hero *p_hero = p->hero_mgr->get_hero(p_troop_info->heros[i]);
				if (p_hero) {
					uint32_t add_honor = 300 / hero_cnt;
					p_hero->add_honor(add_honor);
				}
			}
		}
	}

	return 0;
}

int
ArenaManager::give_ranking_bonus(Player *p, uint32_t ranking)
{
	if (!p) {
		return -1;
	}

	uint32_t history_ranking = p->res_mgr->get_res_value(forever_arena_history_ranking);
	if (ranking >= history_ranking) {//比历史排名低 不给奖励
		return 0;
	}
	p->res_mgr->set_res_value(forever_arena_history_ranking, ranking);

	if (!history_ranking ) {//第一次进排行榜不给奖励
		return 0;
	}

	uint32_t give_diamond = arena_bonus_xml_mgr->calc_arena_bonus_diamond(history_ranking, ranking);
	
	p->chg_diamond(give_diamond);

	//奖励通知
	cli_send_get_common_bonus_noti_out reward_noti_out;
	reward_noti_out.diamond = give_diamond;
	p->send_to_self(cli_send_get_common_bonus_noti_cmd, &reward_noti_out, 0);

	//处理官衔分封状态变化通知
	p->hero_mgr->deal_hero_title_grant_state();

	//通知玩家
	cli_arena_ranking_bonus_noti_out noti_out;
	noti_out.history_ranking = history_ranking;
	noti_out.ranking = ranking;
	noti_out.diamond = give_diamond;

	T_KDEBUG_LOG(p->user_id, "GIVE RANKING BONUS\t[history_ranking=%u, ranking=%u, diamond=%u]", 
			history_ranking, ranking, give_diamond);

	return p->send_to_self(cli_arena_ranking_bonus_noti_cmd, &noti_out, 0);
}

int
ArenaManager::give_arena_daily_ranking_bonus()
{
	return 0;
	/*
	ArenaMap::iterator it = arena_map.begin();
	for (; it != arena_map.end(); ++it) {
		arnea_info_t *p_info = &(it->second);
		const arena_bonus_xml_info_t *p_xml_info = arena_bonus_xml_mgr->get_arena_bonus_xml_info(p_info->ranking);
		if (!p_xml_info) {
			continue;
		}
		uint32_t daily_diamond = p_xml_info->daily_diamond;
		uint32_t daily_golds = p_xml_info->daily_golds;

		//TODO

	}*/
}

int
ArenaManager::set_arena_defend_team(Player *p, uint32_t hero1, uint32_t hero2, uint32_t hero3, uint32_t soldier1, uint32_t soldier2, uint32_t soldier3)
{
	 ArenaMap::iterator it = arena_map.find(p->user_id);
	 if (it == arena_map.end()) {
		T_KWARN_LOG(p->user_id, "set arena defend team err, user not exist");
	 	return cli_user_not_in_arena_ranking_err;
	 }

	 it->second.hero[0] = hero1;
	 it->second.hero[1] = hero2;
	 it->second.hero[2] = hero3;
	 it->second.soldier[0] = soldier1;
	 it->second.soldier[1] = soldier2;
	 it->second.soldier[2] = soldier3;

	 //更新DB
	 db_update_arena_ranking_info_in db_in;
	 pack_arena_ranking_db_info(it->second.ranking, db_in.ranking_info);
	 send_msg_to_dbroute(0, db_update_arena_ranking_info_cmd, &db_in, 0);

	 //更新战斗力
	 update_ranking_info_btl_power(p);

	 return 0;
}

int
ArenaManager::pack_single_arena_info_by_ranking(uint32_t ranking, cli_arena_info_t &info)
{
	const arena_info_t *p_info = get_arena_ranking_info_by_ranking(ranking);
	if (p_info) {
		info.ranking = p_info->ranking;
		info.user_id = p_info->user_id;
		memcpy(info.nick, p_info->nick, NICK_LEN);
		if (p_info->user_id <= 10000) {//机器人
			info.lv = calc_robot_lv(p_info->lv);
			info.btl_power = calc_robot_btl_power(p_info->lv, p_info->btl_power);
		} else {
			info.lv = p_info->lv;
			info.btl_power = p_info->btl_power;
		}
		info.rank = p_info->rank;
		info.hero1 = p_info->hero[0];
		info.hero2 = p_info->hero[1];
		info.hero3 = p_info->hero[2];
		info.soldier1 = p_info->soldier[0];
		info.soldier2 = p_info->soldier[1];
		info.soldier3 = p_info->soldier[2];
	}
	
	return 0;
}

int
ArenaManager::pack_single_arena_info_by_userid(uint32_t user_id, cli_arena_info_t &info)
{
	const arena_info_t *p_info = get_arena_ranking_info_by_userid(user_id);
	if (p_info) {
		info.ranking = p_info->ranking;
		info.user_id = p_info->user_id;
		memcpy(info.nick, p_info->nick, NICK_LEN);
		info.lv = p_info->lv;
		info.rank = p_info->rank;
		info.hero1 = p_info->hero[0];
		info.hero2 = p_info->hero[1];
		info.hero3 = p_info->hero[2];
		info.soldier1 = p_info->soldier[0];
		info.soldier2 = p_info->soldier[1];
		info.soldier3 = p_info->soldier[2];
		info.btl_power = p_info->btl_power;
	}
	
	return 0;
}

int
ArenaManager::pack_recommend_players_info(uint32_t ranking, vector<cli_arena_info_t> &recommend_playes)
{
	uint32_t sz = ranking_map.size();
	if (ranking > sz) {
		return -1;
	}

	if (ranking == 1) {
		return 0;
	}
	if (ranking <= 4) {
		cli_arena_info_t info;
		pack_single_arena_info_by_ranking(1, info);
		recommend_playes.push_back(info);
		if (ranking >= 3) {
			cli_arena_info_t info;
			pack_single_arena_info_by_ranking(2, info);
			recommend_playes.push_back(info);
		} 
		if (ranking == 4) {
			cli_arena_info_t info;
			pack_single_arena_info_by_ranking(3, info);
			recommend_playes.push_back(info);
		}	
		return 0;
	}

	uint32_t r1 = ranged_random(3000, 5999);
	uint32_t r2 = ranged_random(6000, 9499);
	uint32_t r3 = ranged_random(9500, 9999);
	uint32_t ranking_1 = ranking * r1 / 10000;
	uint32_t ranking_2 = ranking * r2 / 10000;
	uint32_t ranking_3 = ranking * r3 / 10000;

	if (ranking_3 == ranking_2) {
		ranking_2--;
	} else if (ranking_2 == ranking_1) {
		ranking_1--;
	}

	cli_arena_info_t info[3];
	pack_single_arena_info_by_ranking(ranking_1, info[0]);
	pack_single_arena_info_by_ranking(ranking_2, info[1]);
	pack_single_arena_info_by_ranking(ranking_3, info[2]);

	for (int i = 0; i < 3; i++) {
		recommend_playes.push_back(info[i]);
	}

	return 0;
}

/* @brief 打包机器人对战信息
 */
int
ArenaManager::pack_arena_robot_battle_info(uint32_t ranking, cli_arena_battle_request_out &cli_out)
{
	const arena_info_t *opp_info = get_arena_ranking_info_by_ranking(ranking);
	if (!opp_info || opp_info->user_id > 10000) {
		return 0;
	}

	const arena_attr_xml_info_t *p_arena_attr_info = arena_attr_xml_mgr->get_arena_attr_xml_info(ranking);
	if (!p_arena_attr_info) {
		return 0;
	}

	//计算开服加成等级和战斗力
	uint32_t robot_lv = calc_robot_lv(opp_info->lv);
	const arena_level_attr_xml_info_t *p_level_attr_xml_info = arena_level_attr_xml_mgr->get_arena_level_attr_xml_info(robot_lv);

	//打包主公
	cli_hero_info_t main_hero_info;
	main_hero_info.hero_id = 100;
	main_hero_info.lv = robot_lv;
	if (robot_lv > opp_info->lv && p_level_attr_xml_info) {//NPC属性自动提升
		main_hero_info.base_attr_info.max_hp = p_level_attr_xml_info->max_hp;
		main_hero_info.base_attr_info.ad = p_level_attr_xml_info->ad;
		main_hero_info.base_attr_info.armor = p_level_attr_xml_info->armor;
		main_hero_info.base_attr_info.resist = p_level_attr_xml_info->resist;
	} else {
		main_hero_info.base_attr_info.max_hp = p_arena_attr_info->max_hp;
		main_hero_info.base_attr_info.ad = p_arena_attr_info->ad;
		main_hero_info.base_attr_info.armor = p_arena_attr_info->armor;
		main_hero_info.base_attr_info.resist = p_arena_attr_info->resist;
	}
	cli_out.opp_heros.push_back(main_hero_info);

	//打包英雄
	double maxhp_factor[] = {1.8, 1.25, 0.8};
	double ad_factor[] = {0.55, 0.8, 1};
	double armor_factor[] = {2.5, 1, 0.6};
	double resist_factor[] = {1.9, 0.6, 1};
	for (int i = 0; i < 3; i++) {
		const hero_xml_info_t* p_xml_info = hero_xml_mgr->get_hero_xml_info(opp_info->hero[i]);
		if (p_xml_info) {
			cli_hero_info_t info;
			info.hero_id = opp_info->hero[i];
			info.lv = robot_lv;
			info.base_attr_info.max_hp = main_hero_info.base_attr_info.max_hp * maxhp_factor[i];
			info.base_attr_info.ad = main_hero_info.base_attr_info.ad * ad_factor[i];
			info.base_attr_info.armor = main_hero_info.base_attr_info.armor * armor_factor[i];
			info.base_attr_info.resist = main_hero_info.base_attr_info.resist * resist_factor[i];
			cli_out.opp_heros.push_back(info);	
		}
	}

	//打包小兵
	uint32_t rank = opp_info->lv / 10;
	uint32_t train_lv = opp_info->lv;
	for (int i = 0; i < 3; i++) {
		Soldier soldier(0, opp_info->soldier[i]);
		soldier.lv = robot_lv;
		soldier.star = 3;
		soldier.rank = rank;
		for (int j = 0; j < 4; j++) {
			soldier.train_lv[j] = train_lv;
		}
		soldier.calc_all(true);

		cli_soldier_info_t info;
		info.soldier_id = opp_info->soldier[i];
		info.lv = robot_lv;
		info.star = 3;
		info.rank = rank;
		info.attr_info.max_hp = soldier.max_hp;
		info.attr_info.ad = soldier.ad;
		info.attr_info.armor = soldier.armor;
		info.attr_info.resist = soldier.resist;
		cli_out.opp_soldiers.push_back(info);
	}

	return 0;
}

/* @brief 打包在线对手对战信息
 */
int
ArenaManager::pack_arena_opp_player_battle_info(uint32_t ranking, cli_arena_battle_request_out &cli_out)
{
	const arena_info_t *opp_info = get_arena_ranking_info_by_ranking(ranking);
	if (!opp_info || opp_info->user_id <= 10000) {
		return 0;
	}

	//判断玩家是否在线
	Player *p = g_player_mgr->get_player_by_uid(opp_info->user_id);
	if (p) {//如果在线
		Hero *main_hero = p->hero_mgr->get_hero(p->role_id);
		if (main_hero) {
			cli_hero_info_t info;
			main_hero->pack_hero_client_info(info);
			cli_out.opp_heros.push_back(info);
		}
		for (int i = 0; i < 3; i++) {
			Hero *hero = p->hero_mgr->get_hero(opp_info->hero[i]);
			if (hero) {
				cli_hero_info_t info;
				hero->pack_hero_client_info(info);
				cli_out.opp_heros.push_back(info);
			}
		}
		for (int i = 0; i < 3; i++) {
			Soldier *soldier = p->soldier_mgr->get_soldier(opp_info->soldier[i]);
			if (soldier) {
				cli_soldier_info_t info;
				soldier->pack_soldier_client_info(info);
				cli_out.opp_soldiers.push_back(info);
			}
		}
	}

	return 0;
}

/* @brief 打包对手对战信息
 */
int
ArenaManager::pack_arena_opp_battle_info(uint32_t ranking, cli_arena_battle_request_out &cli_out)
{
	const arena_info_t *opp_info = get_arena_ranking_info_by_ranking(ranking);
	if (!opp_info) {
		return 0;
	}

	if (opp_info->user_id <= 10000) {//机器人
		return pack_arena_robot_battle_info(ranking, cli_out);
	} else {
		Player *p = g_player_mgr->get_player_by_uid(opp_info->user_id);
		if (p) {//在线
			return pack_arena_opp_player_battle_info(ranking, cli_out);
		} 
	}

	//不在线 
	return 1;
}

/* @brief 打包对手信息-从DB获取
 */
int
ArenaManager::pack_arena_opp_battle_info_from_db(db_get_arena_opp_battle_info_out *p_in, cli_arena_battle_request_out &cli_out)
{
	Player p(0, 0);
	//英雄信息
	for (uint32_t i = 0; i < p_in->heros.size(); i++) {
		db_arena_hero_info_t *p_info = &(p_in->heros[i]);
		if (p_info->hero_info.hero_id == 0) {
			continue;
		}
		Hero hero(&p, p_info->hero_info.hero_id);
		hero.init_hero_db_info(&(p_info->hero_info));
		
		for (uint32_t j = 0; j < p_info->equips.size(); j++) {
			db_equip_info_t *p_equip_info = &(p_info->equips[j]);
			const equip_xml_info_t *base_info = equip_xml_mgr->get_equip_xml_info(p_equip_info->equip_id);
			if (base_info) {
				Equipment *p_equip = new Equipment(&p, base_info->id);
				p_equip->get_tm = p_equip_info->get_tm;
				p_equip->lv = p_equip_info->lv;
				p_equip->exp = p_equip_info->exp;
				p_equip->hero_id = p_equip_info->hero_id;
				p_equip->refining_lv = p_equip_info->refining_lv;
				for (int i = 0; i < 3; i++) {
					p_equip->gem[i] = p_equip_info->gem[i];
				}   
				p_equip->base_info = base_info;
				hero.equips.insert(EquipmentMap::value_type(p_equip->get_tm, p_equip));
			}
		}

		for (uint32_t j = 0; j < p_info->btl_souls.size(); j++) {
			db_btl_soul_info_t *p_btl_soul_info = &(p_info->btl_souls[i]);
			const btl_soul_xml_info_t *base_info = btl_soul_xml_mgr->get_btl_soul_xml_info(p_btl_soul_info->id);
			if (base_info) {
				BtlSoul *p_btl_soul = new BtlSoul(&p, base_info->id);
				p_btl_soul->get_tm = p_btl_soul_info->get_tm;
				p_btl_soul->hero_id = p_btl_soul_info->hero_id;
				p_btl_soul->lv = p_btl_soul_info->lv;
				p_btl_soul->exp = p_btl_soul_info->exp;
				p_btl_soul->tmp = p_btl_soul_info->tmp;
				p_btl_soul->base_info = btl_soul_xml_mgr->get_btl_soul_xml_info(p_btl_soul_info->id);

				hero.btl_souls.insert(BtlSoulMap::value_type(p_btl_soul->get_tm, p_btl_soul));
			}
		}

		//计算属性
		hero.calc_all(true);
		cli_hero_info_t info;
		hero.pack_hero_client_info(info);
		cli_out.opp_heros.push_back(info);

		//释放内存
		EquipmentMap::iterator it = hero.equips.begin();
		while (it != hero.equips.end()) {
			Equipment *p_equip = it->second;
			hero.equips.erase(it++);
			SAFE_DELETE(p_equip);
		}

		BtlSoulMap::iterator it2 = hero.btl_souls.begin();
		while (it2 != hero.btl_souls.end()) {
			BtlSoul *p_btl_soul = it2->second;
			hero.btl_souls.erase(it2++);
			SAFE_DELETE(p_btl_soul);
		}
	}

	//小兵信息
	for (uint32_t i = 0; i < p_in->soldiers.size(); i++) {
		db_soldier_info_t *p_info = &(p_in->soldiers[i]);
		if (p_info->soldier_id == 0) {
			continue;
		}
		Soldier soldier(&p, p_info->soldier_id);
		soldier.init_soldier_base_info();
		soldier.init_soldier_info(p_info);
		soldier.calc_all(true);

		cli_soldier_info_t info;
		soldier.pack_soldier_client_info(info);
		cli_out.opp_soldiers.push_back(info);
	}

	return 0;
}

int
ArenaManager::pack_arena_ranking_db_info(uint32_t ranking, db_arena_info_t &ranking_info)
{
	const arena_info_t *p_info = get_arena_ranking_info_by_ranking(ranking);
	if (!p_info) {
		return 0;
	}

	ranking_info.ranking = ranking;
	ranking_info.user_id = p_info->user_id;
	memcpy(ranking_info.nick, p_info->nick, NICK_LEN);
	ranking_info.lv = p_info->lv;
	ranking_info.rank = p_info->rank;
	ranking_info.hero1 = p_info->hero[0];
	ranking_info.hero2 = p_info->hero[1];
	ranking_info.hero3 = p_info->hero[2];
	ranking_info.soldier1 = p_info->soldier[0];
	ranking_info.soldier2 = p_info->soldier[1];
	ranking_info.soldier3 = p_info->soldier[2];
	ranking_info.btl_power = p_info->btl_power;

	return 0;
}

/********************************************************************************/
/*							ArenaAttrXmlManager Class							*/
/********************************************************************************/
ArenaAttrXmlManager::ArenaAttrXmlManager()
{

}

ArenaAttrXmlManager::~ArenaAttrXmlManager()
{

}

const arena_attr_xml_info_t*
ArenaAttrXmlManager::get_arena_attr_xml_info(uint32_t ranking)
{
	ArenaAttrXmlMap::iterator it = arena_attr_xml_map.begin();
	for (; it != arena_attr_xml_map.end(); ++it) {
		if (ranking <= it->second.min_ranking && ranking >= it->second.max_ranking) {
			return &(it->second);
		}
	}

	return 0;
}

int
ArenaAttrXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_arena_attr_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
ArenaAttrXmlManager::load_arena_attr_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("arena"))) {
			uint32_t min_ranking = 0;
			get_xml_prop(min_ranking, cur, "min_rank");
			ArenaAttrXmlMap::iterator it = arena_attr_xml_map.find(min_ranking);
			if (it != arena_attr_xml_map.end()) {
				ERROR_LOG("load arena attr xml info err, min_ranking=%u", min_ranking);
				return -1;
			}

			arena_attr_xml_info_t info;
			info.min_ranking = min_ranking;
			get_xml_prop(info.max_ranking, cur, "max_rank");
			get_xml_prop(info.lv, cur, "lv");
			get_xml_prop(info.rank, cur, "rank");
			get_xml_prop(info.max_hp, cur, "maxhp");
			get_xml_prop(info.ad, cur, "ad");
			get_xml_prop(info.armor, cur, "armor");
			get_xml_prop(info.resist, cur, "resist");
			get_xml_prop(info.min_btl_power, cur, "min_power");
			get_xml_prop(info.max_btl_power, cur, "max_power");

			TRACE_LOG("load arena attr xml info\t[%u %u %u %u %f %f %f %f %u %u]",
					info.min_ranking, info.max_ranking, info.lv, info.rank, info.max_hp, info.ad, info.armor, info.resist, info.min_btl_power, info.max_btl_power);

			arena_attr_xml_map.insert(ArenaAttrXmlMap::value_type(min_ranking, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*						ArenaHeroXmlManager Class								*/
/********************************************************************************/
ArenaHeroXmlManager::ArenaHeroXmlManager()
{

}

ArenaHeroXmlManager::~ArenaHeroXmlManager()
{

}

uint32_t
ArenaHeroXmlManager::random_one_hero(uint32_t type)
{
	ArenaHeroXmlMap::iterator it = arena_hero_xml_map.find(type);
	if (it == arena_hero_xml_map.end()) {
		return 0;
	}

	uint32_t sz = it->second.heros.size();
	if (sz > 0) {
		uint32_t r = rand() % sz;
		uint32_t hero_id = it->second.heros[r];

		return hero_id;
	}

	return 0;
}

int
ArenaHeroXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_arena_hero_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int 
ArenaHeroXmlManager::load_arena_hero_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("arena"))) {
			uint32_t type = 0;
			uint32_t hero_id = 0;
			get_xml_prop(type, cur, "type");
			get_xml_prop(hero_id, cur, "hero_id");

			ArenaHeroXmlMap::iterator it = arena_hero_xml_map.find(type);
			if (it != arena_hero_xml_map.end()) {
				it->second.heros.push_back(hero_id);
			} else {
				arena_hero_xml_info_t info;
				info.type = type;
				info.heros.push_back(hero_id);
				arena_hero_xml_map.insert(ArenaHeroXmlMap::value_type(type, info));
			}

			TRACE_LOG("load arena hero xml info\t[%u %u]", type, hero_id);
		}

		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*						ArenaBonusXmlManager Class								*/
/********************************************************************************/
ArenaBonusXmlManager::ArenaBonusXmlManager()
{

}

ArenaBonusXmlManager::~ArenaBonusXmlManager()
{

}

const arena_bonus_xml_info_t*
ArenaBonusXmlManager::get_arena_bonus_xml_info(uint32_t ranking)
{
	ArenaBonusXmlMap::iterator it = arena_bonus_xml_map.begin();
	for (; it != arena_bonus_xml_map.end(); ++it) {
		arena_bonus_xml_info_t *p_info = &(it->second);
		if (ranking <= p_info->min_ranking && ranking >= p_info->max_ranking) {
			return p_info;
		}
	}

	return 0;
}

int
ArenaBonusXmlManager::calc_arena_bonus_diamond(uint32_t old_ranking, uint32_t new_ranking)
{
	if (old_ranking <= new_ranking) {
		return 0;
	}

	uint32_t diamond = 0;
	uint32_t flag = 0;
	ArenaBonusXmlMap::iterator it = arena_bonus_xml_map.begin();
	for (; it != arena_bonus_xml_map.end(); ++it) {
		arena_bonus_xml_info_t *p_info = &(it->second);
		if (new_ranking <= p_info->min_ranking && new_ranking >= p_info->max_ranking) {
			if (old_ranking <= p_info->min_ranking && old_ranking >= p_info->max_ranking) {
				diamond = ceil((old_ranking - new_ranking) * p_info->ranking_diamond);
				break;
			} else {
				diamond += ceil((p_info->min_ranking - new_ranking + 1) * p_info->ranking_diamond);
				flag = 1;
				continue;
			
			}
		}
		if (flag) {
			if (old_ranking <= p_info->min_ranking && old_ranking >= p_info->max_ranking) {
				diamond += ceil((old_ranking - p_info->max_ranking) * p_info->ranking_diamond);
				break;
			} else {
				diamond += ceil((p_info->min_ranking - p_info->max_ranking + 1) * p_info->ranking_diamond);
			}
		}
	}

	return diamond;
}

int
ArenaBonusXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_arena_bonus_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
ArenaBonusXmlManager::load_arena_bonus_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("arena"))) {
			uint32_t min_ranking = 0;
			get_xml_prop(min_ranking, cur, "min_rank");
			ArenaBonusXmlMap::iterator it = arena_bonus_xml_map.find(min_ranking);
			if (it != arena_bonus_xml_map.end()) {
				ERROR_LOG("load arena bonux xml info err, min_ranking=%u", min_ranking);
				return -1;
			}

			arena_bonus_xml_info_t info;
			info.min_ranking = min_ranking;
			get_xml_prop(info.max_ranking, cur, "max_rank");
			get_xml_prop(info.ranking_diamond, cur, "ranking_diamond");
			get_xml_prop(info.daily_diamond, cur, "daily_diamond");
			get_xml_prop(info.daily_golds, cur, "daily_golds");

			TRACE_LOG("load arena bonus xml info\t[%u %u %f %u %u]", 
					info.min_ranking, info.max_ranking, info.ranking_diamond, info.daily_diamond, info.daily_golds);

			arena_bonus_xml_map.insert(ArenaBonusXmlMap::value_type(min_ranking, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*						ArenaLevelAttrXmlManager Class							*/
/********************************************************************************/
ArenaLevelAttrXmlManager::ArenaLevelAttrXmlManager()
{
	arena_level_attr_xml_map.clear();
}

ArenaLevelAttrXmlManager::~ArenaLevelAttrXmlManager()
{

}

const arena_level_attr_xml_info_t *
ArenaLevelAttrXmlManager::get_arena_level_attr_xml_info(uint32_t lv)
{
	ArenaLevelAttrXmlMap::iterator it = arena_level_attr_xml_map.find(lv);
	if (it != arena_level_attr_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
ArenaLevelAttrXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_arena_level_attr_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

int
ArenaLevelAttrXmlManager::load_arena_level_attr_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("level"))) {
			uint32_t lv = 0;
			get_xml_prop(lv, cur, "lv");
			ArenaLevelAttrXmlMap::iterator it = arena_level_attr_xml_map.find(lv);
			if (it != arena_level_attr_xml_map.end()) {
				ERROR_LOG("load arena level attr xml info err, lv=%u", lv);
				return -1;
			}
			arena_level_attr_xml_info_t info = {};
			info.lv = lv;
			get_xml_prop(info.max_hp, cur, "maxhp");
			get_xml_prop(info.ad, cur, "ad");
			get_xml_prop(info.armor, cur, "armor");
			get_xml_prop(info.resist, cur, "resist");

			TRACE_LOG("load arena level attr xml info\t[%u %u %u %u %u]", lv, info.max_hp, info.ad, info.armor, info.resist);

			arena_level_attr_xml_map.insert(ArenaLevelAttrXmlMap::value_type(lv, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 拉取竞技场排行榜
 */
int cli_get_arena_ranking_list(Player *p, Cmessage *c_in)
{
	cli_get_arena_ranking_list_in *p_in = P_IN;

	cli_get_arena_ranking_list_out cli_out;
	int ret = arena_mgr->get_arena_ranking_list(p_in->start, p_in->end, cli_out.arena_list);

	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "GET ARENA RANKING LIST\t[start=%u, end=%u]", p_in->start, p_in->end);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 拉取竞技场面板信息
 */
int cli_get_arena_panel_info(Player *p, Cmessage *c_in)
{
	//检查竞技场是否已初始化完毕
	if (!arena_mgr->check_arena_is_init()) {
		T_KWARN_LOG(p->user_id, "arena has not been init!");
		return p->send_to_self_error(p->wait_cmd, cli_arena_has_not_been_init_err, 1);
	}
	
	//检查是否在竞技场中
	const arena_info_t *p_info = arena_mgr->get_arena_ranking_info_by_userid(p->user_id);
	if (!p_info) {
		if (p->lv < 10) {
			T_KWARN_LOG(p->user_id, "user in arena need lv not enough");
			return p->send_to_self_error(p->wait_cmd, cli_user_not_in_arena_ranking_err, 1);
		} else {//加入竞技场
			arena_mgr->first_add_to_arena(p);	
		}
	}
	
	cli_get_arena_panel_info_out cli_out;

	uint32_t daily_tms = p->res_mgr->get_res_value(daily_arena_challenge_tms);
	uint32_t prev_tm = p->res_mgr->get_res_value(daily_arena_challenge_tm);
	uint32_t now_sec = get_now_tv()->tv_sec;

	cli_out.left_tms = daily_tms < 5 ? (5 - daily_tms) : 0;
	cli_out.cd = (now_sec < prev_tm + 900) ? (prev_tm + 900 - now_sec) : 0;

	arena_mgr->pack_single_arena_info_by_userid(p->user_id, cli_out.self_info);

	arena_mgr->pack_recommend_players_info(cli_out.self_info.ranking, cli_out.recommend_players);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 竞技场战斗请求
 */
int cli_arena_battle_request(Player *p, Cmessage *c_in)
{
	cli_arena_battle_request_in *p_in = P_IN;

	uint32_t daily_tms = p->res_mgr->get_res_value(daily_arena_challenge_tms);
	uint32_t prev_tm = p->res_mgr->get_res_value(daily_arena_challenge_tm);
	uint32_t now_sec = get_now_tv()->tv_sec;
	if (daily_tms >= 5) {//检查金钱
		uint32_t need_golds = 10000; 
		CHECK_NEED_GOLDS_ERR(p, need_golds);
	}

	if (now_sec < prev_tm + 900) {
		return p->send_to_self_error(p->wait_cmd, cli_arena_challenge_being_cd_err, 1);
	}

	int ret = arena_mgr->battle_request(p->user_id, p_in->ranking);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	//扣除金币
	if (daily_tms >= 5) {
		uint32_t need_golds = 10000; 
		p->chg_golds(-need_golds);
	}

	//设置限制号
	p->res_mgr->set_res_value(daily_arena_challenge_tms, daily_tms + 1);
	p->res_mgr->set_res_value(daily_arena_challenge_tm, now_sec);
	
	//打包对战信息
	cli_arena_battle_request_out cli_out;
	cli_out.ranking = p_in->ranking;
	ret = arena_mgr->pack_arena_opp_battle_info(p_in->ranking, cli_out);
	if (ret) {//对手不在线 需拉取数据库
		const arena_info_t *p_info = arena_mgr->get_arena_ranking_info_by_ranking(p_in->ranking);
		if (p_info) {
			db_get_arena_opp_battle_info_in db_in;
			db_in.ranking = p_in->ranking;
			for (int i = 0; i < 3; i++) {
				db_in.heros[i] = p_info->hero[i];
				db_in.soldiers[i] = p_info->soldier[i];
			}
			return send_msg_to_dbroute(p, db_get_arena_opp_battle_info_cmd, &db_in, p_info->user_id);
		}
	}

	T_KDEBUG_LOG(p->user_id, "ARENA BATTLE REQUEST!\t[ranking=%u]", p_in->ranking);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 竞技场战斗结束
 */
int cli_arena_battle_end(Player *p, Cmessage *c_in)
{
	cli_arena_battle_end_in *p_in = P_IN;

	cli_arena_battle_end_out cli_out;
	cli_out.opp_ranking = p_in->opp_ranking;
	cli_out.is_win = p_in->is_win;
	int ret = arena_mgr->battle_end(p, p_in->opp_ranking, p_in->is_win, p_in->kill_hero_id, cli_out);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "ARENA BATTLE END\t[ranking=%u, is_win=%u]", p_in->opp_ranking, p_in->is_win);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 设置竞技场防守阵容
 */
int cli_set_arena_defend_team(Player *p, Cmessage *c_in)
{
	cli_set_arena_defend_team_in *p_in = P_IN;

	int ret = arena_mgr->set_arena_defend_team(p, p_in->hero1, p_in->hero2, p_in->hero3, p_in->soldier1, p_in->soldier2, p_in->soldier3);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "SET ARENA DEFEND TEAM\t[%u %u %u %u %u %u]", 
			p_in->hero1, p_in->hero2, p_in->hero3, p_in->soldier1, p_in->soldier2, p_in->soldier3);

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/********************************************************************************/
/*								DB Return										*/
/********************************************************************************/
/* @brief 拉取竞技场人数
 */
int db_get_arena_count(Player *p, Cmessage *c_in, uint32_t ret)
{
	if (ret) {
		return 0;
	}

	db_get_arena_count_out *p_in = P_IN;
	if (p_in->count == 0) {//竞技场为空则初始化竞技场
		arena_mgr->first_init_arena();
	} else {//拉取竞技场信息
		return send_msg_to_dbroute(0, db_get_arena_list_cmd, 0, 0);
	}

	return 0;
}

/* @brief 拉取竞技场列表
 */
int db_get_arena_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	if (ret) {
		return 0;
	}

	db_get_arena_list_out *p_in = P_IN;

	arena_mgr->init_arena(p_in->arena_list);

	return 0;
}

/* @brief 拉取竞技场对手信息
 */
int db_get_arena_opp_battle_info(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);

	db_get_arena_opp_battle_info_out *p_in = P_IN;
	if (p->wait_cmd == cli_arena_battle_request_cmd) {
		cli_arena_battle_request_out cli_out;
		
		cli_out.ranking = p_in->ranking;
		arena_mgr->pack_arena_opp_battle_info_from_db(p_in, cli_out);

		T_KDEBUG_LOG(p->user_id, "GET ARENA OPP BATTLE INFO FROM DB");
		
		return p->send_to_self(p->wait_cmd, &cli_out, 1);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}
