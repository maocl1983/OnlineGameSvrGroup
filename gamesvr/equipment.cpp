/*
 * =====================================================================================
 *
 *  @file  equipment.cpp 
 *
 *  @brief  处理装备相关信息
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

#include "equipment.hpp"
#include "player.hpp"
#include "dbroute.hpp"
#include "hero.hpp"

using namespace project;
using namespace std;

EquipmentXmlManager equip_xml_mgr;
EquipRefiningXmlManager equip_refining_xml_mgr;
EquipCompoundXmlManager equip_compound_xml_mgr;
EquipLevelXmlManager equip_level_xml_mgr;

/********************************************************************************/
/*								Equipment Class									*/
/********************************************************************************/
Equipment::Equipment(Player *p, uint32_t equip_id) : owner(p), id(equip_id)
{
	hero_id = 0;
	get_tm = 0;
	lv = 0;
	exp = 0;
	refining_lv = 0;
	memset(gem, 0, sizeof(gem));
	gem_stat = 1;
}

Equipment::~Equipment()
{

}

int
Equipment::calc_equip_max_lv()
{
	const level_xml_info_t *p_info = level_xml_mgr.get_level_xml_info(owner->lv);
	if (p_info) {
		return p_info->equip_max_lv;
	}

	return 0;
}

int
Equipment::get_max_level_up_exp()
{
	uint32_t max_equip_lv = calc_equip_max_lv();
	uint32_t max_level_exp = get_level_up_exp(lv) - exp;
	for (uint32_t i = lv + 1; i <= max_equip_lv; i++) {
		max_level_exp += get_level_up_exp(i);
	}

	return max_level_exp;
}

int
Equipment::add_exp(uint32_t add_value)
{
	if (!add_value) {
		return 0;
	}

	uint32_t max_equip_lv = calc_equip_max_lv();

	if (!max_equip_lv || lv >= max_equip_lv) {
		return 0;
	}
	uint32_t old_lv = lv;
	exp += add_value;
	uint32_t levelup_exp = get_level_up_exp(lv);
	while (exp >= levelup_exp) {
		exp -= levelup_exp;
		lv++;
		if (lv >= max_equip_lv) {
			uint32_t max_exp = get_level_up_exp(max_equip_lv);
			if (exp > max_exp) {
				exp = max_exp;
			}
			break;
		}
		levelup_exp = get_level_up_exp(lv);
	}

	//更新DB
	db_set_equip_strength_info_in db_in;
	db_in.get_tm = this->get_tm;
	db_in.lv = this->lv;
	db_in.exp = this->exp;
	send_msg_to_dbroute(0, db_set_equip_strength_info_cmd, &db_in, owner->user_id);

	//如果佩戴在英雄身上，重新计算所有属性
	if (lv > old_lv) {
		Hero *p_hero = owner->hero_mgr->get_hero(hero_id);
		if (p_hero) {
			p_hero->calc_all();
		}
	}

	T_KDEBUG_LOG(owner->user_id, "ADD EQUIP EXP\t[get_tm=%u, add_exp=%u, old_lv=%u, lv=%u, exp=%u]", get_tm, add_value, old_lv, lv, exp);

	return 0;
}

int
Equipment::get_level_up_exp(uint32_t lv)
{
	if (!base_info || !base_info->rank || base_info->rank > 5) {
		return -1;
	}
	const equip_level_xml_info_t *p_xml_info = equip_level_xml_mgr.get_equip_level_xml_info(lv + 1);
	if (p_xml_info) {
		return p_xml_info->rank_exp[base_info->rank - 1];
	}

	return -1;
}

int
Equipment::unlock_gem_hole(uint32_t pos)
{
	uint32_t need_diamond = 0;
	if (pos == 2) {
		need_diamond = 100;
	} else if (pos == 3) {
		need_diamond = 200;
	} else {
		return 0;
	}
	if (test_bit_on(gem_stat, pos)) {
		T_KWARN_LOG(owner->user_id, "equip gem hole already unlocked\t[pos=%u]", pos);
		return cli_equip_gem_hole_already_unlock_err;
	}

	if (owner->diamond < need_diamond) {
		T_KWARN_LOG(owner->user_id, "unlock gem hole need diamond not enough\t[diamond=%u, need_diamond=%u]", owner->diamond, need_diamond);
		return cli_not_enough_diamond_err;
	}
	//扣除钻石
	owner->chg_diamond(-need_diamond);

	//解锁宝石孔
	gem_stat = set_bit_on(gem_stat, pos);

	//更新DB
	db_set_equip_gem_stat_in db_in;
	db_in.get_tm = this->get_tm;
	db_in.gem_stat = this->gem_stat;
	send_msg_to_dbroute(0, db_set_equip_gem_stat_cmd, &db_in, owner->user_id);

	return 0;
}

int
Equipment::calc_equip_total_exp()
{
	if (!base_info || !base_info->rank || base_info->rank > 5) {
		return -1;
	}
	uint32_t total_exp = 0;
	for (uint32_t i = 0; i <= lv; i++) {
		const equip_level_xml_info_t *p_xml_info = equip_level_xml_mgr.get_equip_level_xml_info(lv + 1);
		if (p_xml_info) {
			total_exp += p_xml_info->rank_exp[base_info->rank - 1];
		}
	}

	return total_exp;
}

int
Equipment::strength_equipment(vector<cli_item_info_t>& irons, vector<uint32_t>& equips)
{
	if (lv >= MAX_EQUIPMENT_LEVEL) {
		return cli_equip_reach_max_lv_err;
	}
	uint32_t add_exp = 0;
	for (uint32_t i = 0; i < irons.size(); i++) {
		cli_item_info_t *p_item_info = &(irons[i]);
		const item_xml_info_t *p_info = items_xml_mgr.get_item_xml_info(p_item_info->item_id);
		if (!p_info) {
			T_KWARN_LOG(owner->user_id, "invalid item id\t[item_id=%u]", p_item_info->item_id);
			return cli_invalid_item_err;
		}
		if (p_info->type != em_item_type_for_equip_exp) {
			T_KWARN_LOG(owner->user_id, "invalid equip exp item type\t[type=%u]", p_info->type);
			return cli_invalid_equip_exp_item_err;
		}

		uint32_t cur_cnt = owner->items_mgr->get_item_cnt(p_item_info->item_id);
		if (cur_cnt < p_item_info->item_cnt) {
			T_KWARN_LOG(owner->user_id, "item not enough\t[cur_cnt=%u, need_cnt=%u]", cur_cnt, p_item_info->item_cnt);
			return cli_not_enough_item_err;
		}

		add_exp += p_info->effect * p_item_info->item_cnt;
	}

	for (uint32_t i = 0; i < equips.size(); i++) {
		Equipment *p_equip = owner->equip_mgr->get_equip(equips[i]);
		if (!p_equip) {
			T_KWARN_LOG(owner->user_id, "equip not exists\t[get_tm=%u]", equips[i]);
			return cli_equip_not_exist_err;
		}
		if (p_equip->hero_id) {
			return cli_equip_already_put_on_err;
		}
		if (p_equip->get_tm == this->get_tm) {
			T_KWARN_LOG(owner->user_id, "cannot use self strength equipment\t[get_tm=%u]", p_equip->get_tm);
			return cli_invalid_equip_exp_item_err;
		}
		
		add_exp += p_equip->calc_equip_total_exp();
	}

	//检查金币是否足够
	uint32_t max_level_up_exp = get_max_level_up_exp();
	uint32_t real_add_exp = add_exp < max_level_up_exp ? add_exp : max_level_up_exp;
	uint32_t need_golds = real_add_exp * 20;
	if (owner->golds < need_golds) {
		T_KWARN_LOG(owner->user_id, "strenght equipment need golds not enough\t[golds=%u, need_golds=%u]", owner->golds, need_golds);
		return cli_not_enough_golds_err;
	}

	//扣除金币
	if (need_golds) {
		owner->chg_golds(-need_golds);
	}

	//开始扣除物品
	for (uint32_t i = 0; i < irons.size(); i++) {
		cli_item_info_t *p_item_info = &(irons[i]);
		owner->items_mgr->del_item_without_callback(p_item_info->item_id, p_item_info->item_cnt);
	}
	for (uint32_t i = 0; i < equips.size(); i++) {
		owner->equip_mgr->del_equip(equips[i]);
	}

	//增加经验
	this->add_exp(add_exp);

	//检查任务
	owner->task_mgr->check_task(em_task_type_equip_strength);

	//检查成就
	owner->achievement_mgr->check_achievement(em_achievement_type_equip_strength);

	return 0;
}

int
Equipment::refining_equipment()
{
	if (refining_lv >= MAX_EQUIPMENT_REFINING_LEVEL) {
		return cli_equip_reach_max_refining_lv_err;
	}

	uint32_t item_cnt = owner->items_mgr->get_item_cnt(120000);
	const equip_refining_sub_xml_info_t *p_info = equip_refining_xml_mgr.get_equip_refining_xml_info(this->id, this->refining_lv + 1);
	if (!p_info) {
		T_KWARN_LOG(owner->user_id, "cannot refining equipment\t[equip_id=%u, refining_lv=%u]", this->id, this->refining_lv + 1);
		return cli_equip_cannot_refining_err;
	}

	if (item_cnt < p_info->stone_num) {
		T_KWARN_LOG(owner->user_id, "item not enough\t[cur_cnt=%u, need_cnt=%u]", item_cnt, p_info->stone_num);
		return cli_not_enough_item_err;
	}

	if (owner->golds < p_info->cost) {
		T_KWARN_LOG(owner->user_id, "golds not enough\t[cur_golds=%u, need_golds=%u]", owner->golds, p_info->cost);
		return cli_not_enough_golds_err;
	}

	//扣除精炼石、金币
	owner->items_mgr->del_item_without_callback(120000, p_info->stone_num);

	owner->chg_golds(0 - p_info->cost);

	//增加精炼等级
	refining_lv++;

	//修改DB
	db_set_equip_refining_lv_in db_in;
	db_in.get_tm = this->get_tm;
	db_in.refining_lv = this->refining_lv;
	send_msg_to_dbroute(0, db_set_equip_refining_lv_cmd, &db_in, owner->user_id);

	//如果装备挂在英雄身上 重新计算属性
	if (this->hero_id) {
		Hero *p_hero = owner->hero_mgr->get_hero(this->hero_id);
		if (p_hero) {
			p_hero->calc_all();
		}
	}

	//检查任务
	//owner->task_mgr->check_task(em_task_type_equip_refining, refining_lv);

	return 0;
}

bool
Equipment::check_is_have_same_type_gem(uint32_t type)
{
	for (int i = 0; i < 3; i++) {
		if (gem[i]) {
			const item_xml_info_t *p_xml_info = items_xml_mgr.get_item_xml_info(gem[i]);
			if (p_xml_info && p_xml_info->type == type) {
				return true;
			}
		}
	}

	return false;
}

int
Equipment::inlaid_gem(uint32_t gem_id, uint32_t pos)
{
	if (!pos || pos > 3) {
		return 0;
	}

	if (!test_bit_on(gem_stat, pos)) {
		T_KWARN_LOG(owner->user_id, "gem hole not unlock\t[pos=%u]", pos);
		return cli_equip_gem_hole_not_unlock_err;
	}

	if (this->gem[pos - 1]) {
		T_KWARN_LOG(owner->user_id, "gem already inlaid on equip\t[pos=%u]", pos);
		return cli_equip_pos_already_inlaid_gem_err;
	}

	const item_xml_info_t *p_info = items_xml_mgr.get_item_xml_info(gem_id);
	if (!p_info) {
		T_KWARN_LOG(owner->user_id, "invalid gem id\t[gem_id=%u]", gem_id);
		return cli_invalid_item_err;
	}

	if (!owner->equip_mgr->check_is_valid_gem(gem_id)) {
		T_KWARN_LOG(owner->user_id, "invalid gem type\t[gem_id=%u]", gem_id);
		return cli_invalid_equip_gem_err;
	}

	//检查是否有同类型的宝石
	if (check_is_have_same_type_gem(p_info->type)) {
		return cli_equip_have_same_type_gem_err;
	}

	uint32_t gem_cnt = owner->items_mgr->get_item_cnt(gem_id);
	if (!gem_cnt) {
		T_KWARN_LOG(owner->user_id, "gem not exists\t[gem_id=%u]", gem_id);
		return cli_item_not_exist_err;
	}

	//扣除物品
	owner->items_mgr->del_item_without_callback(gem_id, 1);

	//镶嵌宝石
	this->gem[pos - 1] = gem_id;

	//更新DB
	db_set_equip_gem_info_in db_in;
	db_in.get_tm = this->get_tm;
	db_in.pos = pos;
	db_in.gem_id = gem_id;
	send_msg_to_dbroute(0, db_set_equip_gem_info_cmd, &db_in, owner->user_id);

	//如果装备挂在英雄身上 重新计算属性
	if (this->hero_id) {
		Hero *p_hero = owner->hero_mgr->get_hero(this->hero_id);
		if (p_hero) {
			p_hero->calc_all();
		}
	}

	//检查任务
	owner->task_mgr->check_task(em_task_type_inlaid_gem);

	return 0;
}

int
Equipment::put_off_gem(uint32_t pos)
{
	if (!pos || pos > 3) {
		return 0;
	}

	if (!this->gem[pos - 1]) {
		T_KWARN_LOG(owner->user_id, "equip gem not exists\t[pos=%u]", pos);
		return cli_equip_gem_already_put_off_err;
	}

	owner->items_mgr->add_item_without_callback(this->gem[pos-1], 1);

	//更新缓存
	this->gem[pos-1] = 0;

	//更新DB
	db_set_equip_gem_info_in db_in;
	db_in.get_tm = this->get_tm;
	db_in.pos = pos;
	db_in.gem_id = 0;
	send_msg_to_dbroute(0, db_set_equip_gem_info_cmd, &db_in, owner->user_id);

	//如果装备挂在英雄身上 重新计算属性
	if (this->hero_id) {
		Hero *p_hero = owner->hero_mgr->get_hero(this->hero_id);
		if (p_hero) {
			p_hero->send_hero_attr_change_noti();
		}
	}
	
	return 0;
}

int
Equipment::calc_equip_gem_attr(hero_attr_info_t &info)
{
	for (int i = 0; i < 3; i++) {
		if (gem[i]) {
			const item_xml_info_t *p_info = items_xml_mgr.get_item_xml_info(gem[i]);
			if (p_info) {
				switch (p_info->type) {
					case em_item_type_for_hero_maxhp:
						info.max_hp += p_info->effect;
						break;
					case em_item_type_for_hero_ad:
						info.ad += p_info->effect;
						break;
					case em_item_type_for_hero_armor:
						info.armor += p_info->effect;
						break;
					case em_item_type_for_hero_resist:
						info.resist += p_info->effect;
						break;
					default:
						break;
				}
			}
		}
	}

	return 0;
}

/********************************************************************************/
/*							EquipmentManager Class								*/
/********************************************************************************/

EquipmentManager::EquipmentManager(Player *p) : owner(p)
{

}

EquipmentManager::~EquipmentManager()
{
	hero_equip_map.clear();
	EquipmentMap::iterator it = equip_map.begin();	
	while (it != equip_map.end()) {
		Equipment *p_equip = it->second;
		equip_map.erase(it++);
		SAFE_DELETE(p_equip);
	}
}

int
EquipmentManager::init_equips_info(db_get_player_items_info_out *p_out)
{
	for (uint32_t i = 0; i < p_out->equips.size(); i++) {
		db_equip_info_t *p_info = &(p_out->equips[i]);
		const equip_xml_info_t *p_base_info = equip_xml_mgr.get_equip_xml_info(p_info->equip_id);
		if (p_base_info) {
			Equipment *p_equip = new Equipment(owner, p_base_info->id);
			p_equip->get_tm = p_info->get_tm;
			p_equip->lv = p_info->lv;
			p_equip->exp = p_info->exp;
			p_equip->hero_id = p_info->hero_id;
			p_equip->refining_lv = p_info->refining_lv;
			for (int i = 0; i < 3; i++) {
				p_equip->gem[i] = p_info->gem[i];
			}
			p_equip->gem_stat = p_info->gem_stat;
			p_equip->base_info = p_base_info;

			equip_map.insert(EquipmentMap::value_type(p_equip->get_tm, p_equip));

			//按hero_id分类
			if (p_equip->hero_id) {
				HeroEquipmentMap::iterator it = hero_equip_map.find(p_equip->hero_id);
				if (it == hero_equip_map.end()) {
					vector<Equipment*> equip_list;
					equip_list.push_back(p_equip);
					hero_equip_map.insert(HeroEquipmentMap::value_type(p_equip->hero_id, equip_list));
				} else {
					it->second.push_back(p_equip);
				}
			}
			T_KTRACE_LOG(owner->user_id, "init equips info\t[%u %u %u %u %u]", p_equip->id, p_equip->get_tm, p_equip->lv, p_equip->hero_id, p_equip->gem_stat);
		}	
	}

	return 0;
}

int 
EquipmentManager::init_heros_equip_info()
{
	HeroEquipmentMap::iterator it = hero_equip_map.begin();
	for (; it != hero_equip_map.end(); ++it) {
		Hero *p_hero = owner->hero_mgr->get_hero(it->first);
		if (p_hero) {
			vector<Equipment*>::iterator it_equip = it->second.begin();
			for (; it_equip != it->second.end(); ++it_equip) {
				Equipment *p_equip = *it_equip;
				p_hero->equips.insert(EquipmentMap::value_type(p_equip->get_tm, p_equip));
			}
			p_hero->calc_all();
			p_hero->calc_hero_btl_power();
		}
	}
	//更新装备后重新计算战斗力
	owner->calc_btl_power();

	return 0;
}

int
EquipmentManager::add_one_equip(uint32_t equip_id)
{
	const equip_xml_info_t *p_info = equip_xml_mgr.get_equip_xml_info(equip_id);
	if (!p_info) {
		KERROR_LOG(owner->user_id, "invalid equip id\t[equip_id=%u]", equip_id);
		return -1;
	}

	uint32_t now_sec = time(0);
	EquipmentMap::iterator it;
   	while ((it = equip_map.find(now_sec)) != equip_map.end()) {
		now_sec++;
	}

	Equipment *p_equip = new Equipment(owner, equip_id);
	p_equip->get_tm = now_sec;
	p_equip->lv = 0;
	p_equip->exp = 0;
	p_equip->refining_lv = 0;
	p_equip->gem_stat = 1;
	p_equip->base_info = p_info;
	equip_map.insert(EquipmentMap::value_type(now_sec, p_equip));

	//通知前端
	cli_equip_change_noti_out noti_out;
	noti_out.flag = 1;
	noti_out.equip_info.equip_id = equip_id;
	noti_out.equip_info.get_tm = p_equip->get_tm;
	noti_out.equip_info.lv = p_equip->lv;
	noti_out.equip_info.exp = p_equip->exp;
	noti_out.equip_info.refining_lv = p_equip->refining_lv;
	for (int i = 0; i < 3; i++) {
		noti_out.equip_info.gem[i] = p_equip->gem[i];
	}
	noti_out.equip_info.gem_stat = p_equip->gem_stat;
	owner->send_to_self(cli_equip_change_noti_cmd, &noti_out, 0);

	//更新DB
	db_add_equip_in db_in;
	db_in.equip_id = equip_id;
	db_in.get_tm = now_sec;
	db_in.lv = 1;

	T_KDEBUG_LOG(owner->user_id, "ADD ONE EQUIP\t[equip_id=%u]", equip_id);

	return send_msg_to_dbroute(0, db_add_equip_cmd, &db_in, owner->user_id);
}

int
EquipmentManager::del_equip(uint32_t get_tm)
{
	EquipmentMap::iterator it = equip_map.find(get_tm);
	if (it == equip_map.end()) {
		return -1;
	}

	//更新缓存
	Equipment *p_equip = it->second;
	equip_map.erase(get_tm);

	//通知前端
	cli_equip_change_noti_out noti_out;
	noti_out.flag = 2;
	noti_out.equip_info.equip_id = p_equip->id;
	noti_out.equip_info.get_tm = p_equip->get_tm;
	noti_out.equip_info.lv = p_equip->lv;
	noti_out.equip_info.exp = p_equip->exp;
	noti_out.equip_info.refining_lv = p_equip->refining_lv;
	for (int i = 0; i < 3; i++) {
		noti_out.equip_info.gem[i] = p_equip->gem[i];
	}
	owner->send_to_self(cli_equip_change_noti_cmd, &noti_out, 0);

	SAFE_DELETE(p_equip);

	//更新DB
	db_del_equip_in db_in;
	db_in.get_tm = get_tm;

	T_KDEBUG_LOG(owner->user_id, "DEL EQUIP\t[equip_id=%u]", noti_out.equip_info.equip_id);

	return send_msg_to_dbroute(owner, db_del_equip_cmd, &db_in, owner->user_id);
}

int
EquipmentManager::compound_equipment(uint32_t equip_id, vector<uint32_t> &pieces)
{
	const equip_xml_info_t *p_xml_info = equip_xml_mgr.get_equip_xml_info(equip_id);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "invalid equip id\t[equip_id=%u]", equip_id);
		return cli_invalid_equip_err;
	}

	const equip_compound_xml_info_t *p_compound_xml_info = equip_compound_xml_mgr.get_equip_compound_xml_info(equip_id);
	if (!p_compound_xml_info) {
		T_KWARN_LOG(owner->user_id, "invalid compound equip id\t[equip_id=%u]", equip_id);
		return cli_invalid_compound_equip_err;
	}

	for (int i = 0; i < 8; i++) {
		uint32_t piece_id = p_compound_xml_info->piece[i];
		if (piece_id) {
			vector<uint32_t>::iterator it = std::find(pieces.begin(), pieces.end(), piece_id);
			if (it == pieces.end()) {
				T_KWARN_LOG(owner->user_id, "equip need piece not exists\t[piece_id=%u]", piece_id);
				return cli_invalid_compound_equip_piece_err;
			}
		}
	}

	for (uint32_t i = 0; i < pieces.size(); i++) {
		uint32_t cur_cnt = owner->items_mgr->get_item_cnt(pieces[i]);
		if (!cur_cnt) {
			T_KWARN_LOG(owner->user_id, "equip piece not exists\t[piece_id=%u]", pieces[i]);
			return cli_not_enough_item_err;
		}
	}

	//扣除物品
	for (uint32_t i = 0; i < pieces.size(); i++) {
		owner->items_mgr->del_item_without_callback(pieces[i], 1);
	}

	//增加装备
	if (equip_id == 120001 || equip_id == 120002 || equip_id == 120003) {//陨铁/高级陨铁
		owner->items_mgr->add_item_without_callback(equip_id, 1);
	} else {
		this->add_one_equip(equip_id);
	}

	return 0;
}

bool 
EquipmentManager::check_is_valid_gem(uint32_t gem_id)
{
	const item_xml_info_t *p_xml_info = items_xml_mgr.get_item_xml_info(gem_id);
	if (!p_xml_info) {
		return false;
	}

	if (p_xml_info->type < em_item_type_for_hero_maxhp || p_xml_info->type > em_item_type_for_hero_resist) {
		return false;
	}

	return true;
}

int
EquipmentManager::compound_gem(uint32_t gem_id, uint32_t gem_cnt, uint32_t &new_gem_id, uint32_t &new_gem_cnt)
{
	if (gem_cnt % 3 != 0) {
		T_KWARN_LOG(owner->user_id, "compound gem cnt is invalid\t[gem_cnt=%u]", gem_cnt);
		return cli_compund_gem_cnt_err;
	}

	const item_xml_info_t *p_xml_info = items_xml_mgr.get_item_xml_info(gem_id);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "compound gem invalid item\t[gem_id=%u]", gem_id);
		return cli_invalid_item_err;
	} 

	if (!check_is_valid_gem(gem_id)) {
		T_KWARN_LOG(owner->user_id, "invalid gem type\t[gem_id=%u]", gem_id);
		return cli_invalid_equip_gem_err;
	}

	new_gem_id = p_xml_info->relation_id;
	if (!check_is_valid_gem(new_gem_id)) {
		T_KWARN_LOG(owner->user_id, "invalid gem type\t[new_gem_id=%u]", new_gem_id);
		return cli_invalid_equip_gem_err;
	}
	new_gem_cnt = gem_cnt / 3;

	//扣除宝石
	owner->items_mgr->del_item_without_callback(gem_id, gem_cnt);

	//添加宝石
	owner->items_mgr->add_item_without_callback(new_gem_id, new_gem_cnt);

	return 0;
}

Equipment*
EquipmentManager::get_equip(uint32_t get_tm)
{
	EquipmentMap::iterator it = equip_map.find(get_tm);
	if (it == equip_map.end()) {
		return 0;
	}

	return it->second;
}

int
EquipmentManager::pack_all_equips_info(vector<cli_equip_info_t> &equip_vec)
{
	EquipmentMap::iterator it = equip_map.begin();
	for (; it != equip_map.end(); it++) {
		Equipment *p_equip = it->second;
		if (p_equip->hero_id == 0) {//未穿在英雄身上
			cli_equip_info_t info;
			info.equip_id = p_equip->id;
			info.get_tm = p_equip->get_tm;
			info.lv = p_equip->lv;
			info.exp = p_equip->exp;
			info.levelup_exp = p_equip->get_level_up_exp(p_equip->lv);
			info.refining_lv = p_equip->refining_lv;
			for (int i = 0; i < 3; i++) {
				info.gem[i] = p_equip->gem[i];
			}
			info.gem_stat = p_equip->gem_stat;

			T_KTRACE_LOG(owner->user_id, "PACK EQUIPS INFO\t[%u %u %u %u %u %u %u %u %u %u]", info.equip_id, info.get_tm, info.lv, info.exp, info.levelup_exp, 
					info.refining_lv, info.gem[0], info.gem[1], info.gem[2], info.gem_stat);
			equip_vec.push_back(info);
		}
	}   

	return 0;
}

/********************************************************************************/
/*						EquipmentXmlManager Class								*/
/********************************************************************************/
EquipmentXmlManager::EquipmentXmlManager()
{
	equip_xml_map.clear();
}

EquipmentXmlManager::~EquipmentXmlManager()
{
	
}

int
EquipmentXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_equip_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);
	return ret; 
}

int
EquipmentXmlManager::load_equip_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while(cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("equipment"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");

			EquipXmlMap::iterator it = equip_xml_map.find(id);
			if (it != equip_xml_map.end()) {
				KERROR_LOG(0, "equipment id existed! equipment=%u", id);
				return -1;
			}

			equip_xml_info_t info;
			info.id = id;
			get_xml_prop(info.rank, cur, "rank");
			get_xml_prop(info.equip_type, cur, "equip_type");
			get_xml_prop(info.class_type, cur, "class_type");
			get_xml_prop(info.class_effect, cur, "class_effect");
			get_xml_prop(info.suit_id, cur, "suit_id");
			get_xml_prop(info.suit_effect, cur, "suit_effect");

			get_xml_prop(info.attr.max_hp, cur, "maxhp");
			get_xml_prop(info.attr.ad, cur, "ad");
			get_xml_prop(info.attr.armor, cur, "armor");
			get_xml_prop(info.attr.resist, cur, "resist");

			get_xml_prop(info.up_attr.max_hp, cur, "up_maxhp");
			get_xml_prop(info.up_attr.ad, cur,"up_ad");
			get_xml_prop(info.up_attr.armor, cur,"up_armor");
			get_xml_prop(info.up_attr.resist, cur,"up_resist");

			KTRACE_LOG(0,"equipment info[%u %u %u %u %u %u %u %f %f %f %f %f %f %f %f]", info.id, info.rank, info.equip_type, info.class_type, 
					info.class_effect, info.suit_id, info.suit_effect, 
					info.attr.max_hp, info.attr.ad, info.attr.armor, info.attr.resist, 
					info.up_attr.max_hp, info.up_attr.ad, info.up_attr.armor, info.up_attr.resist);

			equip_xml_map.insert(EquipXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}

	return 0;
}


const equip_xml_info_t*
EquipmentXmlManager::get_equip_xml_info(uint32_t equip_id)
{
	EquipXmlMap::iterator it = equip_xml_map.find(equip_id);
	if (it != equip_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}


/********************************************************************************/
/*						EquipRefiningXmlManager Class							*/
/********************************************************************************/
EquipRefiningXmlManager::EquipRefiningXmlManager()
{
	equip_refining_xml_map.clear();
}

EquipRefiningXmlManager::~EquipRefiningXmlManager()
{

}

int
EquipRefiningXmlManager::read_from_xml(const char *filename) 
{
	XML_PARSE_FILE(filename);

	int ret = load_equip_refining_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

const equip_refining_sub_xml_info_t*
EquipRefiningXmlManager::get_equip_refining_xml_info(uint32_t equip_id, uint32_t refining_lv)
{
	EquipRefiningXmlMap::iterator it = equip_refining_xml_map.find(equip_id);
	if (it == equip_refining_xml_map.end()) {
		return 0;
	}

	EquipRefiningSubXmlMap::iterator it2 = it->second.level_map.find(refining_lv);
	if (it2 == it->second.level_map.end()) {
		return 0;
	}
	
	return &(it2->second);
}


int
EquipRefiningXmlManager::load_equip_refining_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("equipRefining"))) {
			uint32_t equip_id = 0;
			get_xml_prop(equip_id, cur, "id");
			equip_refining_sub_xml_info_t sub_info;
			get_xml_prop(sub_info.lv, cur, "lv");
			get_xml_prop(sub_info.maxhp, cur, "maxhp");
			get_xml_prop(sub_info.ad, cur, "ad");
			get_xml_prop(sub_info.armor, cur, "armor");
			get_xml_prop(sub_info.resist, cur, "resist");
			get_xml_prop(sub_info.extra_maxhp, cur, "maxhp2");
			get_xml_prop(sub_info.extra_ad, cur, "ad2");
			get_xml_prop(sub_info.extra_armor, cur, "armor2");
			get_xml_prop(sub_info.extra_resist, cur, "resist2");
			get_xml_prop(sub_info.stone_num, cur, "stone_num");
			get_xml_prop(sub_info.cost, cur, "cost");

			EquipRefiningXmlMap::iterator it = equip_refining_xml_map.find(equip_id);
			if (it != equip_refining_xml_map.end()) {
				EquipRefiningSubXmlMap *p_map = &(it->second.level_map);
				EquipRefiningSubXmlMap::iterator it2 = p_map->find(sub_info.lv);
				if (it2 != p_map->end()) {
					ERROR_LOG("load equip refining xml info err, lv exists, equip_id=%u, lv=%u", equip_id, sub_info.lv);
					return -1;
				}
				p_map->insert(EquipRefiningSubXmlMap::value_type(sub_info.lv, sub_info));
			} else {
				equip_refining_xml_info_t info;
				info.equip_id = equip_id;
				info.level_map.insert(EquipRefiningSubXmlMap::value_type(sub_info.lv, sub_info));
				equip_refining_xml_map.insert(EquipRefiningXmlMap::value_type(equip_id, info));
			}

			TRACE_LOG("load equip refiing xml info\t[%u %u %f %f %f %f %f %f %f %f %u %u]", equip_id, sub_info.lv, 
					sub_info.maxhp, sub_info.ad, sub_info.armor, sub_info.resist, sub_info.extra_maxhp, sub_info.extra_ad, 
					sub_info.extra_armor, sub_info.extra_resist, sub_info.stone_num, sub_info.cost);

		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*						EquipCompoundXmlManager Class							*/
/********************************************************************************/
EquipCompoundXmlManager::EquipCompoundXmlManager()
{

}

EquipCompoundXmlManager::~EquipCompoundXmlManager()
{

}

const equip_compound_xml_info_t*
EquipCompoundXmlManager::get_equip_compound_xml_info(uint32_t id)
{
	EquipCompoundXmlMap::iterator it = equip_compound_xml_map.find(id);
	if (it != equip_compound_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
EquipCompoundXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_equip_compound_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int 
EquipCompoundXmlManager::load_equip_compound_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("equip"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			EquipCompoundXmlMap::iterator it = equip_compound_xml_map.find(id);
			if (it != equip_compound_xml_map.end()) {
				ERROR_LOG("load equip compound xml info err, id=%u", id);
				return -1;
			}

			equip_compound_xml_info_t info;
			info.id = id;
			for (int i = 0; i < 8; i++) {
				char str[16] = {0};
				sprintf(str, "piece%01d", i+1);
				get_xml_prop_def(info.piece[i], cur, str, 0);
			}

			TRACE_LOG("load equip compound xml info\t[%u %u %u %u %u %u %u %u %u]", 
					info.id, info.piece[0], info.piece[1], info.piece[2], info.piece[3], info.piece[4], info.piece[5], info.piece[6], info.piece[7]);

			equip_compound_xml_map.insert(EquipCompoundXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}
	return 0;
}

/********************************************************************************/
/*						EquipLevelXmlManager Class								*/
/********************************************************************************/
EquipLevelXmlManager::EquipLevelXmlManager()
{

}

EquipLevelXmlManager::~EquipLevelXmlManager()
{

}

const equip_level_xml_info_t*
EquipLevelXmlManager::get_equip_level_xml_info(uint32_t equip_lv)
{
	EquipLevelXmlMap::const_iterator it = equip_level_xml_map.find(equip_lv);
	if (it != equip_level_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
EquipLevelXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_equip_level_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);
	
	return ret;
}

int
EquipLevelXmlManager::load_equip_level_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("equip"))) {
			uint32_t lv = 0;
			get_xml_prop(lv, cur, "lv");
			EquipLevelXmlMap::iterator it = equip_level_xml_map.find(lv);
			if (it != equip_level_xml_map.end()) {
				ERROR_LOG("load equip level xml info err, lv=%u", lv);
				return -1;
			}

			equip_level_xml_info_t info = {};
			info.lv = lv;
			get_xml_prop(info.rank_exp[0], cur, "rank1_exp");
			get_xml_prop(info.rank_exp[1], cur, "rank2_exp");
			get_xml_prop(info.rank_exp[2], cur, "rank3_exp");
			get_xml_prop(info.rank_exp[3], cur, "rank4_exp");
			get_xml_prop(info.rank_exp[4], cur, "rank5_exp");

			TRACE_LOG("load equip level xml info\t[%u %u %u %u %u %u]", 
					info.lv, info.rank_exp[0],  info.rank_exp[1], info.rank_exp[2], info.rank_exp[3], info.rank_exp[4]);

			equip_level_xml_map.insert(EquipLevelXmlMap::value_type(lv, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*									Client Request								*/
/********************************************************************************/
/* @brief 强化装备
 */
int cli_strengthen_equipment(Player *p, Cmessage *c_in)
{
	cli_strengthen_equipment_in *p_in = P_IN;
	Equipment *p_equip = p->equip_mgr->get_equip(p_in->get_tm);
	if (!p_equip) {
		T_KWARN_LOG(p->user_id, "equip not exists\t[get_tm=%u]", p_in->get_tm);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_equip_err, 1);
	}

	uint32_t old_lv = p_equip->lv;
	uint32_t old_exp = p_equip->exp;
	int ret = p_equip->strength_equipment(p_in->irons, p_in->equips);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_strengthen_equipment_out cli_out;
	cli_out.get_tm = p_in->get_tm;
	cli_out.old_lv = old_lv;
	cli_out.old_exp = old_exp;
	cli_out.lv = p_equip->lv;
	cli_out.exp = p_equip->exp;
	cli_out.levelup_exp = p_equip->get_level_up_exp(p_equip->lv);

	T_KDEBUG_LOG(p->user_id, "STRENGTHEN EQUIPMENT\t[get_tm=%u, iron_num=%u, equip_num=%u, lv=%u, exp=%u]", 
			p_in->get_tm, (uint32_t)p_in->irons.size(), (uint32_t)p_in->equips.size(), p_equip->lv, p_equip->exp);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 精炼装备
 */
int cli_refining_equipment(Player *p, Cmessage *c_in)
{
	cli_refining_equipment_in *p_in = P_IN;
	Equipment *p_equip = p->equip_mgr->get_equip(p_in->get_tm);
	if (!p_equip) {
		T_KWARN_LOG(p->user_id, "equip not exists\t[get_tm=%u]", p_in->get_tm);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_equip_err, 1);
	}

	int ret = p_equip->refining_equipment();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_refining_equipment_out cli_out;
	cli_out.get_tm = p_in->get_tm;
	cli_out.refining_lv = p_equip->refining_lv;

	T_KDEBUG_LOG(p->user_id, "REFINING EQUIPMENT\t[get_tm=%u, refining_lv=%u]", p_in->get_tm, p_equip->refining_lv);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 装备镶嵌宝石
 */
int cli_equipment_inlaid_gem(Player *p, Cmessage *c_in)
{
	cli_equipment_inlaid_gem_in *p_in = P_IN;
	Equipment *p_equip = p->equip_mgr->get_equip(p_in->get_tm);
	if (!p_equip) {
		T_KWARN_LOG(p->user_id, "equip not exists\t[get_tm=%u]", p_in->get_tm);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_equip_err, 1);
	}

	if (!p_in->pos || p_in->pos > 3) {
		return p->send_to_self_error(p->wait_cmd, cli_invalid_input_arg_err, 1);
	}

	int ret = p_equip->inlaid_gem(p_in->gem_id, p_in->pos);

	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_equipment_inlaid_gem_out cli_out;
	cli_out.get_tm = p_in->get_tm;
	cli_out.gem_id = p_in->gem_id;
	cli_out.pos = p_in->pos;

	T_KDEBUG_LOG(p->user_id, "EQUIPMENT INLAID GEM\t[get_tm=%u, gem_id=%u, pos=%u]", p_in->get_tm, p_in->gem_id, p_in->pos);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 装备卸下宝石
 */
int cli_equipment_put_off_gem(Player *p, Cmessage *c_in)
{
	cli_equipment_put_off_gem_in *p_in = P_IN;
	Equipment *p_equip = p->equip_mgr->get_equip(p_in->get_tm);
	if (!p_equip) {
		T_KWARN_LOG(p->user_id, "equip not exists\t[get_tm=%u]", p_in->get_tm);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_equip_err, 1);
	}

	if (!p_in->pos || p_in->pos > 3) {
		return p->send_to_self_error(p->wait_cmd, cli_invalid_input_arg_err, 1);
	}

	int ret = p_equip->put_off_gem(p_in->pos);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_equipment_put_off_gem_out cli_out;
	cli_out.get_tm = p_in->get_tm;
	cli_out.pos = p_in->pos;

	T_KDEBUG_LOG(p->user_id, "EQUIPMENT PUT OFF GEM\t[get_tm=%u, pos=%u]", p_in->get_tm, p_in->pos);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 装备合成
 */
int cli_compound_equipment(Player *p, Cmessage *c_in)
{
	cli_compound_equipment_in *p_in = P_IN;

	int ret = p->equip_mgr->compound_equipment(p_in->equip_id, p_in->piece_list);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "COMPOUND EQUIPMENT!\t[equip_id=%u]", p_in->equip_id);

	cli_compound_equipment_out cli_out;
	cli_out.equip_id = p_in->equip_id;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 宝石合成
 */
int cli_compound_gem(Player *p, Cmessage *c_in)
{
	cli_compound_gem_in *p_in = P_IN;

	uint32_t new_gem_id = 0;
	uint32_t new_gem_cnt = 0;
	int ret = p->equip_mgr->compound_gem(p_in->gem_id, p_in->gem_cnt, new_gem_id, new_gem_cnt);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_compound_gem_out cli_out;
	cli_out.gem_id = p_in->gem_id;
	cli_out.gem_cnt = p_in->gem_cnt;
	cli_out.new_gem_id = new_gem_id;
	cli_out.new_gem_cnt = new_gem_cnt;

	T_KDEBUG_LOG(p->user_id, "COMPOUND GENM\t[gem_id=%u, gem_cnt=%u, new_gem_id=%u, new_gem_cnt=%u]", 
			p_in->gem_id, p_in->gem_cnt, new_gem_id, new_gem_cnt);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 解锁装备宝石镶嵌位
 */
int cli_unlock_gem_inlaid_hole(Player *p, Cmessage *c_in)
{
	cli_unlock_gem_inlaid_hole_in *p_in = P_IN;

	Equipment *p_equip = p->equip_mgr->get_equip(p_in->get_tm);
	if (!p_equip) {
		T_KWARN_LOG(p->user_id, "equip not exists\t[get_tm=%u]", p_in->get_tm);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_equip_err, 1);
	}

	if (p_in->pos < 2 || p_in->pos > 3) {
		return p->send_to_self_error(p->wait_cmd, cli_invalid_input_arg_err, 1);
	}

	int ret = p_equip->unlock_gem_hole(p_in->pos);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_unlock_gem_inlaid_hole_out cli_out;
	cli_out.get_tm = p_in->get_tm;
	cli_out.pos = p_in->pos;
	
	T_KDEBUG_LOG(p->user_id, "UNLOCK GEM INLAID HOLE\t[get_tm=%u, pos=%u]", p_in->get_tm, p_in->pos);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}
