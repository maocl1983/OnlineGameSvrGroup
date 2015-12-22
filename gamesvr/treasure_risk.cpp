/*
 * =====================================================================================
 *
 *  @file  treasure_risk.cpp 
 *
 *  @brief  夺宝系统
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
#include "./proto/xseer_redis.hpp"
#include "./proto/xseer_redis_enum.hpp"

#include "treasure_risk.hpp"
#include "player.hpp"
#include "hero.hpp"
#include "item.hpp"
#include "soldier.hpp"
#include "equipment.hpp"
#include "dbroute.hpp"
#include "redis.hpp"
#include "utils.hpp"

using namespace std;
using namespace project;

TreasureAttrXmlManager treasure_attr_xml_mgr;
TreasureHeroXmlManager treasure_hero_xml_mgr;
TreasureRewardXmlManager treasure_reward_xml_mgr;

/********************************************************************************/
/*							TreasureManager Class								*/
/********************************************************************************/
TreasureManager::TreasureManager(Player *p) : owner(p)
{
	battle_user = 0;
	request_piece = 0;
	recommend_players.clear();
}

TreasureManager::~TreasureManager()
{
	recommend_players.clear();
}

int
TreasureManager::treasure_10_risk(uint32_t request_piece, cli_treasure_10_risk_out &cli_out)
{
	//检查耐力是否足够
	if (owner->endurance < 20) {
		T_KWARN_LOG(owner->user_id, "treasure 10 risk endurance not enough\t[endurance=%u, need_endurance=%u]", owner->endurance, 20);
		return cli_endurance_not_enough_err;
	}

	const item_piece_xml_info_t *p_piece_xml_info = item_piece_xml_mgr.get_item_piece_xml_info(request_piece);
	if (!p_piece_xml_info) {
		T_KWARN_LOG(owner->user_id, "treasure 10 risk piece err, piece_id=%u", request_piece);
		return cli_invalid_item_err;
	}

	uint32_t prob = get_request_piece_prob(0);

	int tms = 0;
	uint32_t piece_id = 0;
	for (int i = 0; i < 10; i++) {
		uint32_t cur_prob = rand() % 1000;
		tms++;
		if (cur_prob < prob) {
			piece_id = request_piece;
			break;
		}
	}

	cli_out.piece_id = request_piece;
	cli_out.win_tms = tms > 10 ? 0 : tms;
	if (piece_id > 0) {
		owner->items_mgr->add_item_without_callback(piece_id, 1);
		cli_send_get_common_bonus_noti_out noti_out;
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, piece_id, 1);
		owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);
	}

	for (int i = 0; i < tms; i++) {
		//发送奖励
		const treasure_reward_xml_info_t *p_xml_info = treasure_reward_xml_mgr.random_one_treasure_reward();
		if (p_xml_info) {
			owner->items_mgr->add_item_without_callback(p_xml_info->item_id, p_xml_info->num);
			cli_treasure_risk_info_t info;
			info.role_exp = 2 * owner->lv;
			info.draw_reward.item_id = p_xml_info->item_id;
			info.draw_reward.item_cnt = p_xml_info->num;
			if ((i == tms - 1) && piece_id) {
				info.piece_id = piece_id;
			}
			cli_out.rewards.push_back(info);
		}
		//触发奇遇
		owner->adventure_mgr->trigger_adventure();
	}

	//扣除耐力
	uint32_t cost_endurance = 2 * tms;
	owner->chg_endurance(-cost_endurance, false, true);

	return 0;
}

int
TreasureManager::open_protection()
{
	uint32_t protection_tm = owner->res_mgr->get_res_value(forever_treasure_risk_protection_tm);
	uint32_t now_sec = get_now_tv()->tv_sec;
	if (protection_tm + 2 * 3600 > now_sec) {
		T_KWARN_LOG(owner->user_id, "user already in protection\t[protection_tm=%u, now_sec=%u]", protection_tm, now_sec);
		return cli_treasure_already_in_protection_err;
	}

	//获取免战牌数量
	uint32_t cur_cnt = owner->items_mgr->get_item_cnt(120005);
	if (!cur_cnt) {
		T_KWARN_LOG(owner->user_id, "mian zhan pai not enough\t[cur_cnt=%u]", cur_cnt);
		return cli_not_enough_item_err;
	}

	//扣除免战牌
	owner->items_mgr->del_item_without_callback(120005, 1);

	//更新保护时间
	owner->res_mgr->set_res_value(forever_treasure_risk_protection_tm, now_sec);

	//同步更新redis
	redis_mgr.set_treasure_protection_time(owner, now_sec);

	return 0;
}

uint32_t
TreasureManager::get_request_piece_prob(uint32_t user_id)
{
	//玩家
	if (user_id >= 100000) {
		return 500;
	}	

	const item_piece_xml_info_t *p_piece_xml_info = item_piece_xml_mgr.get_item_piece_xml_info(request_piece);
	if (!p_piece_xml_info) {
		return 0;
	}

	uint32_t prob = 0;
	if (p_piece_xml_info->relation_id == 120001 || p_piece_xml_info->relation_id == 120003) {//陨铁
		prob = 200;
	} else if (p_piece_xml_info->relation_id == 120002) {//高级陨铁
		prob = 120;
	} else {
		const equip_xml_info_t *p_equip_xml_info = equip_xml_mgr.get_equip_xml_info(p_piece_xml_info->relation_id);
		if (!p_equip_xml_info) {
			return 0;
		}
		if (p_equip_xml_info->rank == 2) {//绿色
			prob = 500;
		} else if (p_equip_xml_info->rank == 3) {//蓝色
			prob = 500;
		} else if (p_equip_xml_info->rank == 4) {//紫色
			prob = 200;
		}
	}
	
	return prob;
}

int
TreasureManager::get_treasure_risk_prob_lv(uint32_t user_id, uint32_t piece_id)
{
	uint32_t prob = get_request_piece_prob(user_id);
	uint32_t prob_lv = 1;
	if (prob < 50) {//极低概率
		prob_lv = 1;
	} else if (prob < 100) {//低概率
		prob_lv = 2;
	} else if (prob < 150) {//较低概率
		prob_lv = 3;
	} else if (prob < 300) {//一般概率
		prob_lv = 4;
	} else if (prob < 500) {//较高概率
		prob_lv = 5;
	} else {//高概率
		prob_lv = 6;
	}

	return prob_lv;
}

int
TreasureManager::winner_draw(cli_treasure_risk_winner_draw_out &out)
{
	if (!is_win) {
		T_KWARN_LOG(owner->user_id, "treasure risk not win");
		return cli_treasure_risk_not_win_err;
	}

	if (reward_stat) {
		T_KWARN_LOG(owner->user_id, "treasure risk reward already gotted!");
		return cli_treasure_risk_reward_already_draw_err;
	}

	reward_stat = true;

	const treasure_reward_xml_info_t *p_xml_info = treasure_reward_xml_mgr.random_one_treasure_reward();
	if (p_xml_info) {
		owner->items_mgr->add_item_without_callback(p_xml_info->item_id, p_xml_info->num);
		out.item_info.item_id = p_xml_info->item_id;
		out.item_info.item_cnt = p_xml_info->num;

		cli_send_get_common_bonus_noti_out noti_out;
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, p_xml_info->item_id, p_xml_info->num);
		owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);
	}

	return 0;
}

int
TreasureManager::battle_request(uint32_t user_id, uint32_t piece_id)
{
	if (get_battle_user_id()) {
		T_KWARN_LOG(owner->user_id, "treasure battle request err, battle_user=%u", user_id);
		return cli_treasure_battle_request_user_err;
	}

	if (owner->endurance < 2) {
		T_KWARN_LOG(owner->user_id, "treasure battle endurence not enough\t[endurance=%u, need_endurance=%u]", owner->endurance, 2);
		return cli_endurance_not_enough_err;
	}

	//扣除耐力
	owner->chg_endurance(-2, false, true);
	

	set_battle_user_id(user_id);

	return 0;
}

int
TreasureManager::give_win_honor(uint32_t kill_hero_id)
{
	const troop_info_t* p_troop_info = owner->get_troop(em_troop_type_treasure);
	if (p_troop_info) {
		uint32_t hero_cnt = p_troop_info->heros.size();
		for (uint32_t i = 0; i < hero_cnt; i++) {
			if (kill_hero_id) {
				if (p_troop_info->heros[i] == kill_hero_id) {
					Hero *p_hero = owner->hero_mgr->get_hero(kill_hero_id); 
					if (p_hero){
						p_hero->add_honor(300);//TODO
					}
				}
			} else {
				Hero *p_hero = owner->hero_mgr->get_hero(p_troop_info->heros[i]);
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
TreasureManager::battle_end(uint32_t user_id, uint32_t is_win, uint32_t kill_hero_id, uint32_t &piece_id)
{
	if (get_battle_user_id() != user_id) {
		T_KWARN_LOG(owner->user_id, "treasure battle end err\t[battle_user=%u, user=%u]", get_battle_user_id(), user_id);
		return cli_treasure_battle_user_not_match_err;
	}

	set_battle_user_id(0);

	uint32_t add_role_exp = owner->lv;
	if (is_win) {
		add_role_exp *= 2;
		uint32_t need_piece_id = get_request_piece();
		if (!need_piece_id) {
			T_KWARN_LOG(owner->user_id, "treasure battle end request piece not exist\t[piece_id=%u]", need_piece_id);
			return cli_treasuer_battle_request_piece_not_exist_err;
		}
		uint32_t need_prob = get_request_piece_prob(user_id);
		uint32_t cur_prob = rand() % 1000;
		if (cur_prob < need_prob) {
			piece_id = need_piece_id;
			owner->items_mgr->add_item_without_callback(piece_id, 1);

			cli_send_get_common_bonus_noti_out noti_out;
			owner->items_mgr->pack_give_items_info(noti_out.give_items_info, piece_id, 1);
			owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

			if (user_id >= 100000) {//玩家 删除被抢夺碎片
				Player *p = g_player_mgr.get_player_by_uid(user_id);
				if (p) {//在线
					p->items_mgr->del_item_without_callback(piece_id, 1);
				} else {//不在线
					db_change_items_in db_in;
					db_in.flag = 0;
					db_in.item_id = piece_id; 
					db_in.chg_cnt = 1;
					send_msg_to_dbroute(0, db_change_items_cmd, &db_in, owner->user_id);

					rs_del_treasure_piece_user_in rs_in;
					rs_in.piece_id = piece_id;
					send_msg_to_redis(0, rs_del_treasure_piece_user_cmd, &rs_in, user_id);
				}
			}
		}

		//添加金币
		owner->chg_golds(5000);

		//奖励战功
		give_win_honor(kill_hero_id);

		//清除之前抽奖奖励状态
		reward_stat = false;
	}

	//添加主角经验
	owner->add_role_exp(add_role_exp);

	//set_request_piece(0);
	this->is_win = is_win;

	return 0;
}

int
TreasureManager::handle_treasure_piece_user_list_redis_return(rs_get_treasure_piece_user_list_out *p_in)
{
	vector<uint32_t> user_list;
	for (uint32_t i = 0; i < p_in->users.size(); i++) {
		uint32_t r = rand() % 100;
		if (r < 50) {//50%概率抽到玩家，最多4个
			user_list.push_back(p_in->users[i]);
			if (user_list.size() >= 4) {
				break;
			}
		}
	}

	uint32_t user_cnt = user_list.size();
	if (user_cnt > 0) {//如果有玩家
		for (uint32_t i = 0; i < user_cnt; i++) {
			uint32_t user_id = user_list[i];
			Player *p = g_player_mgr.get_player_by_uid(user_id);
			if (p) {//如果玩家在线
				owner->treasure_mgr->pack_recommend_online_player(p);
			} else {//不在线 拉DB
				send_msg_to_dbroute(owner, db_get_treasure_risk_opp_player_info_cmd, 0, user_id);
			}
		}
		uint32_t robot_cnt = user_cnt < 4 ? 4 - user_cnt : 0;
		if (robot_cnt > 0) {
			pack_recommend_robot(request_piece, robot_cnt);
		}
	} else {//机器人 
		pack_recommend_robot(request_piece, 4);
	}

	if (recommend_players.size() >= 4) {
		cli_get_treasure_risk_recommend_players_out cli_out;
		pack_client_recommend_players(cli_out);
		owner->send_to_self(owner->wait_cmd, &cli_out, 1);
		T_KDEBUG_LOG(owner->user_id, "===Frankie====");
	}

	return 0;
}

int
TreasureManager::get_recommend_players_cnt()
{
	return recommend_players.size();
}

int
TreasureManager::pack_recommend_offline_player(db_get_treasure_risk_opp_player_info_out *p_in)
{
	cli_treasure_risk_player_info_t info;
	info.user_id = p_in->user_id;
	memcpy(info.nick, p_in->nick, NICK_LEN);
	info.lv = p_in->lv;

	Player p(p_in->user_id, 0);
	//英雄信息
	for (uint32_t i = 0; i < p_in->heros.size(); i++) {
		db_treasure_risk_hero_info_t *p_info = &(p_in->heros[i]);
		if (!p_info->hero_info.hero_id) {
			continue;
		}
		Hero hero(&p, p_info->hero_info.hero_id);
		hero.init_hero_db_info(&(p_info->hero_info));
		
		for (uint32_t j = 0; j < p_info->equips.size(); j++) {
			db_equip_info_t *p_equip_info = &(p_info->equips[j]);
			const equip_xml_info_t *base_info = equip_xml_mgr.get_equip_xml_info(p_equip_info->equip_id);
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
			const btl_soul_xml_info_t *base_info = btl_soul_xml_mgr.get_btl_soul_xml_info(p_btl_soul_info->id);
			if (base_info) {
				BtlSoul *p_btl_soul = new BtlSoul(&p, base_info->id);
				p_btl_soul->get_tm = p_btl_soul_info->get_tm;
				p_btl_soul->hero_id = p_btl_soul_info->hero_id;
				p_btl_soul->lv = p_btl_soul_info->lv;
				p_btl_soul->exp = p_btl_soul_info->exp;
				p_btl_soul->tmp = p_btl_soul_info->tmp;
				p_btl_soul->base_info = btl_soul_xml_mgr.get_btl_soul_xml_info(p_btl_soul_info->id);

				hero.btl_souls.insert(BtlSoulMap::value_type(p_btl_soul->get_tm, p_btl_soul));
			}
		}

		//计算属性
		hero.calc_all(true);
		cli_treasure_risk_attr_info_t attr_info;
		attr_info.id = hero.id;
		attr_info.max_hp = hero.max_hp;
		attr_info.ad = hero.ad;
		attr_info.armor = hero.armor;
		attr_info.resist = hero.resist;
		info.heros.push_back(attr_info);

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
		if (!p_info->soldier_id) {
			continue;
		}
		Soldier soldier(&p, p_info->soldier_id);
		soldier.init_soldier_base_info();
		soldier.init_soldier_info(p_info);
		soldier.calc_all(true);

		cli_treasure_risk_attr_info_t attr_info;
		attr_info.id = soldier.id;
		attr_info.max_hp = soldier.max_hp;
		attr_info.ad = soldier.ad;
		attr_info.armor = soldier.armor;
		attr_info.resist = soldier.resist;
		info.soldiers.push_back(attr_info);
	}

	recommend_players.push_back(info);

	return 0;
}

int
TreasureManager::pack_recommend_online_player(Player *p)
{
	cli_treasure_risk_player_info_t info;
	info.user_id = p->user_id;
	memcpy(info.nick, p->nick, NICK_LEN);
	info.lv = p->lv;

	//打包主公信息
	cli_treasure_risk_attr_info_t attr_info;
	attr_info.id = p->role_id;
	Hero *p_hero = p->hero_mgr->get_hero(p->role_id);
	if (p_hero) {
		attr_info.max_hp = p_hero->max_hp;
		attr_info.ad = p_hero->ad;
		attr_info.armor = p_hero->armor;
		attr_info.resist = p_hero->resist;
		info.heros.push_back(attr_info);
	}

	const troop_info_t* p_troop_info = p->get_troop(em_troop_type_treasure);
	if (p_troop_info) {
		//打包英雄信息
		for (int i = 0; i < 3; i++) {
			uint32_t hero_id = p_troop_info->heros[i];
			Hero *p_hero = p->hero_mgr->get_hero(hero_id);
			if (p_hero) {
				cli_treasure_risk_attr_info_t attr_info;
				attr_info.max_hp = p_hero->max_hp;
				attr_info.ad = p_hero->ad;
				attr_info.armor = p_hero->armor;
				attr_info.resist = p_hero->resist;
				info.heros.push_back(attr_info);
			}
		}

		//打包小兵信息
		for (int i = 0; i < 3; i++) {
			uint32_t soldier_id = p_troop_info->soldiers[i];
			Soldier *p_soldier = p->soldier_mgr->get_soldier(soldier_id);
			if (p_soldier) {
				attr_info.max_hp = p_soldier->max_hp;
				attr_info.ad = p_soldier->ad;
				attr_info.armor = p_soldier->armor;
				attr_info.resist = p_soldier->resist;
				info.soldiers.push_back(attr_info);
			}
		}
	}

	recommend_players.push_back(info);

	return 0;
}

int
TreasureManager::pack_recommend_robot(uint32_t piece_id, uint32_t num)
{
	if (!num || num > 4) {
		return -1;
	}

	const item_piece_xml_info_t *p_piece_xml_info = item_piece_xml_mgr.get_item_piece_xml_info(piece_id);
	if (!p_piece_xml_info) {
		return -1;
	}

	Hero *main_hero = owner->hero_mgr->get_hero(owner->role_id);
	if (!main_hero) {
		return 0;
	}
	char robot_nick[5][NICK_LEN] = {"Mark", "Frankie", "Rock", "MoMo", "Buono"};
	for (uint32_t i = 0; i < num; i++) {
		uint32_t lv = ranged_random(main_hero->lv - 1, main_hero->lv + 1);
		const treasure_attr_xml_info_t *p_xml_info = treasure_attr_xml_mgr.get_treasure_attr_xml_info(lv);
		if (!p_xml_info) {
			return 0;
		}
		cli_treasure_risk_player_info_t info;
		info.user_id = 1 + i;
		const char *nick = robot_nick[rand() % 5];
		memcpy(info.nick, nick, strlen(nick));
		info.lv = lv;
		info.prob_lv = get_treasure_risk_prob_lv(info.user_id, piece_id);

		//打包主公
		cli_treasure_risk_attr_info_t attr_info;
		attr_info.id = 100;
		attr_info.max_hp = p_xml_info->max_hp;
		attr_info.ad = p_xml_info->ad;
		attr_info.armor = p_xml_info->armor;
		attr_info.resist = p_xml_info->resist;
		info.heros.push_back(attr_info);

		//打包英雄
		double maxhp_factor[] = {1.8, 1.25, 0.8};
		double ad_factor[] = {0.55, 0.8, 1};
		double armor_factor[] = {2.5, 1, 0.6};
		double resist_factor[] = {1.9, 0.6, 1};
		for (int j = 0; j < 3; j++) {
			cli_treasure_risk_attr_info_t attr_info;
			attr_info.id = treasure_hero_xml_mgr.random_one_hero(j + 1);
			const hero_xml_info_t* p_hero_xml_info = hero_xml_mgr.get_hero_xml_info(attr_info.id);
			if (!p_hero_xml_info) {
				continue;
			}
			attr_info.max_hp = p_xml_info->max_hp * maxhp_factor[j];
			attr_info.ad = p_xml_info->ad * ad_factor[j];
			attr_info.armor = p_xml_info->armor * armor_factor[j];
			attr_info.resist = p_xml_info->resist * resist_factor[j];
			info.heros.push_back(attr_info);
		}

		//打包小兵
		double star_factor = 1.5; 
		uint32_t rank = lv / 10;
		uint32_t train_lv = lv;
		uint32_t soldier_id[] = {2003, 2008, 2013};
		for (int j = 0; j < 3; j++) {
			cli_treasure_risk_attr_info_t attr_info;
			attr_info.id = soldier_id[j];
			const soldier_xml_info_t *p_xml_info = soldier_xml_mgr.get_soldier_xml_info(attr_info.id);
			if (!p_xml_info) {
				continue;
			}
			const soldier_rank_detail_xml_info_t *p_rank_info = soldier_rank_xml_mgr.get_soldier_rank_xml_info(attr_info.id, rank);
			
			attr_info.max_hp = (p_xml_info->max_hp + (p_rank_info ? p_rank_info->max_hp : 0) + p_xml_info->hp_up * train_lv) * star_factor;
			attr_info.ad = (p_xml_info->ad + (p_rank_info ? p_rank_info->ad : 0) + p_xml_info->ad_up * train_lv) * star_factor;
			attr_info.armor = (p_xml_info->armor + (p_rank_info ? p_rank_info->armor : 0) + p_xml_info->armor_up * train_lv) * star_factor;
			attr_info.resist = (p_xml_info->resist + (p_rank_info ? p_rank_info->resist : 0) + p_xml_info->resist_up * train_lv) * star_factor;
			info.soldiers.push_back(attr_info);
		}
		recommend_players.push_back(info);
	}

	return 0;
}

void
TreasureManager::pack_client_recommend_players(cli_get_treasure_risk_recommend_players_out &out)
{
	out.piece_id = request_piece;
	for (uint32_t i = 0; i < recommend_players.size(); i++) {
		cli_treasure_risk_player_info_t *p_info = &(recommend_players[i]);
		cli_treasure_risk_player_info_t info;
		info.user_id = p_info->user_id;
		memcpy(info.nick, p_info->nick, NICK_LEN);
		info.lv = p_info->lv;
		info.prob_lv = p_info->prob_lv;
		for (uint32_t j = 0; j < p_info->heros.size(); j++) {
			cli_treasure_risk_attr_info_t *p_attr_info = &(p_info->heros[j]);
			cli_treasure_risk_attr_info_t attr_info;
			attr_info.id = p_attr_info->id;
			attr_info.max_hp = p_attr_info->max_hp;
			attr_info.ad = p_attr_info->ad;
			attr_info.armor = p_attr_info->armor;
			attr_info.resist = p_attr_info->resist;
			info.heros.push_back(attr_info);
		}
		for (uint32_t j = 0; j < p_info->soldiers.size(); j++) {
			cli_treasure_risk_attr_info_t *p_attr_info = &(p_info->soldiers[j]);
			cli_treasure_risk_attr_info_t attr_info;
			attr_info.id = p_attr_info->id;
			attr_info.max_hp = p_attr_info->max_hp;
			attr_info.ad = p_attr_info->ad;
			attr_info.armor = p_attr_info->armor;
			attr_info.resist = p_attr_info->resist;
			info.soldiers.push_back(attr_info);
		}

		out.recommend_players.push_back(info);
	}

	//清空推荐玩家 
	recommend_players.clear();
}

/********************************************************************************/
/*							TreasureAttrXmlManager Class						*/
/********************************************************************************/
TreasureAttrXmlManager::TreasureAttrXmlManager()
{

}

TreasureAttrXmlManager::~TreasureAttrXmlManager()
{

}

const treasure_attr_xml_info_t*
TreasureAttrXmlManager::get_treasure_attr_xml_info(uint32_t lv)
{
	TreasureAttrXmlMap::iterator it = treasure_attr_xml_map.find(lv);
	if (it != treasure_attr_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
TreasureAttrXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_treasure_attr_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);
	
	return ret;
}

int
TreasureAttrXmlManager::load_treasure_attr_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("treasure"))) {
			uint32_t lv = 0;
			get_xml_prop(lv, cur, "lv");
			TreasureAttrXmlMap::iterator it = treasure_attr_xml_map.find(lv);
			if (it != treasure_attr_xml_map.end()) {
				ERROR_LOG("load treasure attr xml info err, lv=%u", lv);
				return -1;
			}

			treasure_attr_xml_info_t info;
			info.lv = lv;
			get_xml_prop(info.rank, cur, "rank");
			get_xml_prop(info.max_hp, cur, "maxhp");
			get_xml_prop(info.ad, cur, "ad");
			get_xml_prop(info.armor, cur, "armor");
			get_xml_prop(info.resist, cur, "resist");

			TRACE_LOG("load treasure attr xml info\t[%u %u %f %f %f %f]", info.lv, info.rank, info.max_hp, info.ad, info.armor, info.resist);

			treasure_attr_xml_map.insert(TreasureAttrXmlMap::value_type(lv, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							TreasureHeroXmlManager Class						*/
/********************************************************************************/
TreasureHeroXmlManager::TreasureHeroXmlManager()
{

}

TreasureHeroXmlManager::~TreasureHeroXmlManager()
{

}

uint32_t
TreasureHeroXmlManager::random_one_hero(uint32_t type)
{
    TreasureHeroXmlMap::iterator it = treasure_hero_xml_map.find(type);
    if (it == treasure_hero_xml_map.end()) {
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
TreasureHeroXmlManager::read_from_xml(const char *filename)
{
    XML_PARSE_FILE(filename);
    int ret = load_treasure_hero_xml_info(cur);

    BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

    return ret;
}

int
TreasureHeroXmlManager::load_treasure_hero_xml_info(xmlNodePtr cur)
{
    cur = cur->xmlChildrenNode;
    while (cur) {
        if (!xmlStrcmp(cur->name, XMLCHAR_CAST("treasure"))) {
            uint32_t type = 0;
            uint32_t hero_id = 0;
            get_xml_prop(type, cur, "type");
            get_xml_prop(hero_id, cur, "hero_id");

            TreasureHeroXmlMap::iterator it = treasure_hero_xml_map.find(type);
            if (it != treasure_hero_xml_map.end()) {
                it->second.heros.push_back(hero_id);
            } else {
                treasure_hero_xml_info_t info;
                info.type = type;
                info.heros.push_back(hero_id);
                treasure_hero_xml_map.insert(TreasureHeroXmlMap::value_type(type, info));
            }

            TRACE_LOG("load treasure hero xml info\t[%u %u]", type, hero_id);
        }

        cur = cur->next;
    }

    return 0;
}

/********************************************************************************/
/*							TreasureRewardXmlManager Class						*/
/********************************************************************************/
TreasureRewardXmlManager::TreasureRewardXmlManager()
{
	total_prob = 0;
	rewards.clear();
}

TreasureRewardXmlManager::~TreasureRewardXmlManager()
{

}

const treasure_reward_xml_info_t*
TreasureRewardXmlManager::random_one_treasure_reward()
{
	if (!total_prob) {
		return 0;
	}

	uint32_t r = rand() % total_prob;
	uint32_t cur_prob = 0;
	for(uint32_t i = 0; i < rewards.size(); i++) {
		cur_prob += rewards[i].prob;
		if (r < cur_prob) {
			return &(rewards[i]);
		}
	}	

	return 0;
}

int
TreasureRewardXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_treasure_reward_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
TreasureRewardXmlManager::load_treasure_reward_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("treasure"))) {
			treasure_reward_xml_info_t info;
			get_xml_prop(info.item_id, cur, "item_id");
			get_xml_prop(info.num, cur, "num");
			get_xml_prop(info.prob, cur, "item_prob");

			total_prob += info.prob;

			TRACE_LOG("load treasure reward xml info\t[%u %u %u]", info.item_id, info.num, info.prob);

			rewards.push_back(info);
		}
		cur = cur->next;
	}
	return 0;
}

/********************************************************************************/
/*								Client	Request									*/
/********************************************************************************/

/* @brief 夺宝-拉取面板信息
 */
int cli_treasure_risk_get_panel_info(Player *p, Cmessage *c_in)
{
	cli_treasure_risk_get_panel_info_out cli_out;
	uint32_t protection_tm = p->res_mgr->get_res_value(forever_treasure_risk_protection_tm);
	uint32_t now_sec = get_now_tv()->tv_sec;

	cli_out.is_protected = (protection_tm + 2 * 3600) < now_sec ? 0 : 1;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 夺宝-拉取推荐玩家
 */
int cli_get_treasure_risk_recommend_players(Player *p, Cmessage *c_in)
{
	cli_get_treasure_risk_recommend_players_in *p_in = P_IN;

	p->treasure_mgr->set_request_piece(p_in->piece_id);

	uint32_t now_sec = time(0);
	uint32_t now_hour = utils_mgr.get_hour(now_sec);
	if (now_hour >= 0 && now_hour < 10) {//免战时间 只能挑战机器人
		p->treasure_mgr->pack_recommend_robot(p_in->piece_id, 4);

		cli_get_treasure_risk_recommend_players_out cli_out;
		p->treasure_mgr->pack_client_recommend_players(cli_out);
		return p->send_to_self(p->wait_cmd, &cli_out, 1);
	}

	return redis_mgr.get_treasure_piece_user_list(p, p_in->piece_id);
}

/* @brief 夺宝-十连夺
 */
int cli_treasure_10_risk(Player *p, Cmessage *c_in)
{
	cli_treasure_10_risk_in *p_in = P_IN;
	cli_treasure_10_risk_out cli_out;
	int ret = p->treasure_mgr->treasure_10_risk(p_in->piece_id, cli_out);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "TREASURE 10 RISK\t[piece_id=%u]", p_in->piece_id);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 夺宝战斗胜利抽奖
 */
int cli_treasure_risk_winner_draw(Player *p, Cmessage *c_in)
{
	cli_treasure_risk_winner_draw_out cli_out;
	int ret = p->treasure_mgr->winner_draw(cli_out);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "TREASURE RISK WINNER DRAW!");

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 夺宝战斗请求
 */
int cli_treasure_risk_battle_request(Player *p, Cmessage *c_in)
{
	cli_treasure_risk_battle_request_in *p_in = P_IN;

	int ret = p->treasure_mgr->battle_request(p_in->user_id, p_in->piece_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "TREASURE RISK BATTLE REQUEST\t[%u]", p_in->user_id);

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 夺宝战斗结束
 */
int cli_treasure_risk_battle_end(Player *p, Cmessage *c_in)
{
	cli_treasure_risk_battle_end_in *p_in = P_IN;

	uint32_t piece_id = 0;
	int ret = p->treasure_mgr->battle_end(p_in->user_id, p_in->is_win, p_in->kill_hero_id, piece_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	//触发奇遇
	p->adventure_mgr->trigger_adventure();

	T_KDEBUG_LOG(p->user_id, "TREASURE RISK BATTLE END\t[%u %u %u]", p_in->user_id, p_in->is_win, piece_id);

	cli_treasure_risk_battle_end_out cli_out;
	cli_out.user_id = p_in->user_id;
	cli_out.is_win = p_in->is_win;
	cli_out.piece_id = piece_id;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 夺宝-开启保护
 */
int cli_treasure_risk_open_protection(Player *p, Cmessage *c_in)
{
	int ret = p->treasure_mgr->open_protection();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "TREASURE OPEN PROTECTION");

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/********************************************************************************/
/*								DB Return										*/
/********************************************************************************/

/* @brief 拉取夺宝对手信息
 */
int db_get_treasure_risk_opp_player_info(Player *p, Cmessage *c_in, uint32_t ret)
{
	db_get_treasure_risk_opp_player_info_out *p_in = P_IN;

	if (p->wait_cmd == cli_get_treasure_risk_recommend_players_cmd) {
		p->treasure_mgr->pack_recommend_offline_player(p_in);
		uint32_t recommend_cnt = p->treasure_mgr->get_recommend_players_cnt();
		if (recommend_cnt >= 4) {
			cli_get_treasure_risk_recommend_players_out cli_out;
			p->treasure_mgr->pack_client_recommend_players(cli_out);
			return p->send_to_self(p->wait_cmd, &cli_out, 1);
		}
		return 0;
	} 

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}
