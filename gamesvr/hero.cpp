/*
 * =====================================================================================
 *
 *  @file  hero.cpp 
 *
 *  @brief  英雄系统
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
#include "hero.hpp"
#include "player.hpp"
#include "dbroute.hpp"
#include "talent.hpp"
#include "instance.hpp"
#include "skill.hpp"
#include "arena.hpp"

using namespace project;
using namespace std;

//HeroXmlManager hero_xml_mgr;
//HeroRankXmlManager hero_rank_xml_mgr;
//HeroRankStuffXmlManager hero_rank_stuff_xml_mgr;
//HeroLevelAttrXmlManager hero_level_attr_xml_mgr;
//LevelXmlManager level_xml_mgr;
//HeroHonorXmlManager hero_honor_xml_mgr;
//HeroHonorExpXmlManager hero_honor_exp_xml_mgr;
//HeroTitleXmlManager hero_title_xml_mgr;

/********************************************************************************/
/*							Hero Class										*/
/********************************************************************************/
Hero::Hero(Player *p, uint32_t hero_id) : id(hero_id), owner(p)
{
	get_tm = 0;
	rank = 0;
	lv = 0;
	lv_factor = 0;
	exp = 0;
	honor_lv = 0;
	honor = 0;
	btl_power = 0;
	trip_id = 0;
	title = 0;
	state = 0;
	is_in_affairs = 0;

	str_growth = 0;
	int_growth = 0;
	agi_growth = 0;
	
	strength = 0;
	intelligence = 0;
	agility = 0;
	speed = 0;

	hp = 0;
	max_hp = 0;
	hp_regain = 0;
	ad = 0;
	armor = 0;
	ad_cri = 0;
	ad_ren = 0;
	resist = 0;
	ad_chuan = 0;
	ap_chuan = 0;
	hit = 0;
	miss = 0;

	skill_lv = 0;
	equips.clear();
	btl_souls.clear();
	talents.clear();

	memset(&base_attr, 0x0, sizeof(base_attr));
	memset(&equip_attr, 0x0, sizeof(equip_attr));
	memset(&talent_attr, 0x0, sizeof(talent_attr));
	memset(&btl_soul_attr, 0x0, sizeof(btl_soul_attr));
	memset(&honor_attr, 0x0, sizeof(honor_attr));
	memset(&horse_attr, 0x0, sizeof(horse_attr));

	base_info = hero_xml_mgr->get_hero_xml_info(hero_id);
}

Hero::~Hero()
{
	equips.clear();
	btl_souls.clear();
	talents.clear();
}

int
Hero::get_level_up_exp(uint32_t lv)
{
	const level_xml_info_t * p_info = level_xml_mgr->get_level_xml_info(lv);
	if (!p_info) {
		return -1;
	}
	if (id == owner->role_id) {
		return p_info->master_exp;
	}

	return p_info->hero_exp;
}

int
Hero::add_exp(uint32_t add_value)
{
	if (!add_value) {
		return 0;
	}

	uint32_t orig_add_value = add_value;
	uint32_t old_lv = lv;

	uint32_t max_lv = calc_hero_max_lv();
	if (!max_lv) {
		return 0;
	}

	if (lv > max_lv) {
		return 0;
	}    

	uint32_t old_exp = exp;
	uint32_t real_add_exp = 0;

	exp += add_value;

	if (lv == max_lv) {
		uint32_t need_exp = get_level_up_exp(max_lv);
		if (exp >= need_exp) {
			exp = need_exp;
		}
		real_add_exp = exp - old_exp;
	} else {
		uint32_t level_up_exp = get_level_up_exp(lv);
		while (exp >= level_up_exp) {//升级
			exp -= level_up_exp;
			lv++;
			if (lv >= max_lv) {
				uint32_t max_exp = get_level_up_exp(max_lv);
				if (exp > max_exp) {
					exp = max_exp;
				}
				real_add_exp += exp;
				break;
			}
			level_up_exp = get_level_up_exp(lv);
			real_add_exp += level_up_exp;
		}
	}

	//更新DB
	db_update_hero_exp_in db_in;
	db_in.hero_id = this->id;
	db_in.lv = lv;
	db_in.exp = exp;
	send_msg_to_dbroute(0, db_update_hero_exp_cmd, &db_in, owner->user_id);

	//英雄经验变化通知
	cli_hero_exp_chg_noti_out noti_out;
	noti_out.hero_id = id;
	noti_out.old_lv = old_lv;
	noti_out.lv = lv;
	noti_out.add_exp = add_value;
	noti_out.exp = exp;
	noti_out.levelup_exp = get_level_up_exp(lv);
	noti_out.max_lv = owner->hero_mgr->calc_hero_max_lv();
	owner->send_to_self(cli_hero_exp_chg_noti_cmd, &noti_out, 0);
	
	if (lv > old_lv) {
		if (id == owner->role_id) {//主角升级
			owner->deal_role_levelup(old_lv, lv);
			const arena_info_t *p_info = arena_mgr->get_arena_ranking_info_by_userid(owner->user_id);
			if (p_info) {
				arena_mgr->update_ranking_info_lv(owner->user_id, lv);
			}
		}
		calc_all();
		send_hero_attr_change_noti();
	}

	//检查任务
	if (old_lv < lv && id != owner->role_id) {
		for (uint32_t i = old_lv; i <= lv; i++) {
			uint32_t lv_cnt = owner->hero_mgr->calc_over_cur_lv_hero_cnt(i);
			owner->task_mgr->check_task(em_task_type_hero_lv, i, lv_cnt);
			//检查成就
			owner->achievement_mgr->check_achievement(em_achievement_type_hero_lv, i, lv_cnt);
		}
	}

	T_KDEBUG_LOG(owner->user_id, "ADD HERO EXP\t[hero_id=%u, add_value=%u, lv=%u, exp=%u]", this->id, orig_add_value, this->lv, this->exp);

	return real_add_exp;
}

int
Hero::calc_hero_max_honor_lv()
{
	return 239;
}

int
Hero::get_level_up_honor(uint32_t lv)
{
	const hero_honor_exp_xml_info_t *p_info = hero_honor_exp_xml_mgr->get_hero_honor_exp_xml_info(lv);
	if (p_info) {
		return p_info->exp;
	}

	return -1;
}

/* @brief 添加战功
 */
int
Hero::add_honor(uint32_t add_value, bool item_flag)
{
	if (!add_value) {
		return 0;
	}

	uint32_t max_honor_lv = calc_hero_max_honor_lv();
	if (!max_honor_lv) {
		return 0;
	}

	if (honor_lv > max_honor_lv) {
		return 0;
	}

	//只有紫色以上武将才能加战功
   	if (base_info->star < 4) {
		return 0;
	}	

	bool cri_flag = false;
	if (item_flag) {//通过道具增加经验
		const hero_honor_exp_xml_info_t *p_info = hero_honor_exp_xml_mgr->get_hero_honor_exp_xml_info(lv);
		if (p_info) {
			uint32_t level_up_honor = get_level_up_honor(honor_lv);
			if (honor + add_value < level_up_honor) {//不够升级计算暴击概率
				double r = (rand() % 1000) / 1000;
				if (r < p_info->cri_prob) {
					cri_flag = true;
					add_value *= 2;
				}
			}
		}
	}	

	uint32_t old_honor_lv = honor_lv;	
	uint32_t old_max_grant_title = get_hero_max_grant_title();

	uint32_t old_honor = honor;
	uint32_t real_add_honor = 0;

	honor += add_value;

	if (honor_lv == max_honor_lv) {
		uint32_t need_honor = get_level_up_honor(max_honor_lv);
		if (honor >= need_honor) {
			honor = need_honor;
		}
		real_add_honor = honor - old_honor;
	} else {
		uint32_t level_up_honor = get_level_up_honor(honor_lv);
		while (honor >= level_up_honor) {//升级
			honor_lv++;
			if (honor_lv >= max_honor_lv) {
				uint32_t need_honor = get_level_up_honor(max_honor_lv);
				if (honor >= need_honor) {
					honor = need_honor;
				}
				real_add_honor += honor;
				break;
			}
			honor -= level_up_honor;
			level_up_honor = get_level_up_honor(honor_lv);
			real_add_honor += level_up_honor;
		}
	}

	if (cri_flag) {
		real_add_honor /= 2;
	}
	
	//更新db
	db_update_hero_honor_info_in db_in;
	db_in.hero_id = this->id;
	db_in.honor_lv = honor_lv;
	db_in.honor = honor;
	send_msg_to_dbroute(0, db_update_hero_honor_info_cmd, &db_in, owner->user_id);

	//战功变化通知
	cli_hero_honor_change_noti_out noti_out;
	noti_out.hero_id = id;
	noti_out.old_honor_lv = old_honor_lv;
	noti_out.honor_lv = honor_lv;
	noti_out.add_honor = add_value;
	noti_out.honor = honor;
	noti_out.levelup_honor = get_level_up_honor(honor_lv);
	noti_out.max_honor_lv = max_honor_lv;
	owner->send_to_self(cli_hero_honor_change_noti_cmd, &noti_out, 0);

	if (honor_lv > old_honor_lv) {
		send_hero_attr_change_noti();
		uint32_t max_grant_title = get_hero_max_grant_title();
		if (max_grant_title > old_max_grant_title) {//检查分封状态是否变化
			cli_hero_title_grant_state_change_noti_out noti_out;
			cli_hero_title_grant_info_t info;
			info.hero_id = this->id;
			info.max_grant_title = max_grant_title;
			noti_out.heros_state.push_back(info);
			owner->send_to_self(cli_hero_title_grant_state_change_noti_cmd, &noti_out, 0);
		}
	}
	
	//检查任务
	if (old_honor_lv < honor_lv) {
		for (uint32_t i = old_honor_lv; i <= honor_lv; i++) {
			uint32_t honor_lv_cnt = owner->hero_mgr->calc_over_cur_honor_lv_hero_cnt(i);
			owner->task_mgr->check_task(em_task_type_hero_honor_lv, i, honor_lv_cnt);
			owner->achievement_mgr->check_achievement(em_achievement_type_hero_honor_lv, i, honor_lv_cnt);
		}
	}


	T_KDEBUG_LOG(owner->user_id, "ADD HERO HONOR\t[hero_id=%u, add_honor=%u, honor_lv=%u, honor=%u, levelup_honor=%u]", 
			id, add_value, honor_lv, honor, noti_out.levelup_honor);

	return real_add_honor;
}

/* @brief 设置英雄状态
 */
int
Hero::set_hero_status(uint32_t status)
{
	if (this->state != status) {
		this->state = status;
	}
	if (status == 0) {//休息
		owner->instance_mgr->set_hero(this->id, 0);
	} else {//参战
		owner->instance_mgr->set_hero(this->id, 1);
	}

	//重新计算队伍战斗力
	owner->calc_btl_power();

	//设置DB
	db_set_hero_state_in db_in;
	db_in.hero_id = this->id;
	db_in.state = status;

	send_msg_to_dbroute(0, db_set_hero_state_cmd, &db_in, owner->user_id);

	T_KDEBUG_LOG(owner->user_id, "SET HERO status\t[hero_id=%u, state=%u]", id, status);

	return 0;
}

/* @brief 查找英雄装备
 */
Equipment*
Hero::get_hero_equipment(uint32_t get_tm)
{
	EquipmentMap::iterator it = equips.find(get_tm);
	if (it == equips.end()) {
		return 0;
	}

	return it->second;
}

/* @brief 查找同类型装备
 */
Equipment*
Hero::get_hero_same_type_equip(uint32_t equip_type)
{
	EquipmentMap::iterator it = equips.begin();
	for (; it != equips.end(); ++it) {
		if (it->second->base_info->equip_type == equip_type) {
			return it->second;
		}
	}

	return 0;
}

/* @brief 英雄穿上装备
 */
int
Hero::put_on_equipment(uint32_t get_tm, uint32_t &off_get_tm)
{
	off_get_tm = 0;
	Equipment *p_equip = owner->equip_mgr->get_equip(get_tm);
	if (!p_equip) {
		T_KWARN_LOG(owner->user_id, "put on equip not exist\t[get_tm=%u]", get_tm);
		return cli_equip_not_exist_err;
	}

	uint32_t equip_id = p_equip->id;
	const equip_xml_info_t *p_info = equip_xml_mgr->get_equip_xml_info(equip_id);
	if (!p_info) {
		T_KWARN_LOG(owner->user_id, "put on equip invalid\t[equip_id=%u]", equip_id);
		return cli_invalid_equip_err;
	}

	Equipment *p_hero_equip = get_hero_equipment(get_tm);
	if (p_hero_equip) {
		T_KWARN_LOG(owner->user_id, "equip already put on\t[get_tm=%u]", get_tm);
		return cli_equip_already_put_on_err;
	}

	if (p_equip->hero_id) {//如果已装备在其他英雄身上
		Hero *p_hero = owner->hero_mgr->get_hero(p_equip->hero_id);
	   	if (p_hero) {
			p_hero->put_off_equipment(get_tm);
		}
	}

	//查找是否有同类型装备
	Equipment *p_same_type_equip = get_hero_same_type_equip(p_equip->base_info->equip_type);
	if (p_same_type_equip) {//替换
		off_get_tm = p_same_type_equip->get_tm;
		p_same_type_equip->hero_id = 0;
		equips.erase(p_same_type_equip->get_tm);
		//更新DB
		db_hero_put_off_equipment_in db_in;
		db_in.get_tm = p_same_type_equip->get_tm;

		send_msg_to_dbroute(owner, db_hero_put_off_equipment_cmd, &db_in, owner->user_id);
	} 

	//更新缓存
	p_equip->hero_id = this->id;
	equips.insert(EquipmentMap::value_type(get_tm, p_equip));
	//更新DB
	db_hero_put_on_equipment_in db_in;
	db_in.hero_id = this->id;
	db_in.get_tm = get_tm;

	send_msg_to_dbroute(0, db_hero_put_on_equipment_cmd, &db_in, owner->user_id);

	KDEBUG_LOG(owner->user_id, "HERO PUT ON EQUIPMENT\t[hero_id=%u get_tm=%u off_equip_get_tm=%u]", id, get_tm, off_get_tm);

	return 0;
}

/* @brief 英雄穿上装备列表
 */
int
Hero::put_on_equipment_list(vector<uint32_t> &equips, cli_hero_put_on_equipment_out &cli_out)
{
	for (uint32_t i = 0; i < equips.size(); i++) {
		uint32_t get_tm = equips[i];
		Equipment *p_equip = owner->equip_mgr->get_equip(get_tm);
		if (!p_equip) {
			T_KWARN_LOG(owner->user_id, "put on equip not exist\t[get_tm=%u]", get_tm);
			return cli_equip_not_exist_err;
		}

		uint32_t equip_id = p_equip->id;
		const equip_xml_info_t *p_info = equip_xml_mgr->get_equip_xml_info(equip_id);
		if (!p_info) {
			T_KWARN_LOG(owner->user_id, "put on equip invalid\t[equip_id=%u]", equip_id);
			return cli_invalid_equip_err;
		}

		Equipment *p_hero_equip = get_hero_equipment(get_tm);
		if (p_hero_equip) {
			T_KWARN_LOG(owner->user_id, "equip already put on\t[get_tm=%u]", get_tm);
			return cli_equip_already_put_on_err;
		}
	}

	for (uint32_t i = 0; i < equips.size(); i++) {
		uint32_t get_tm = equips[i];
		uint32_t off_get_tm = 0;
		put_on_equipment(get_tm, off_get_tm);
		cli_out.on_equips.push_back(get_tm);
		if (off_get_tm) {
			cli_out.off_equips.push_back(off_get_tm);
		}
	}

	//更新属性
	send_hero_attr_change_noti();

	return 0;
}

/* @brief 英雄脱下装备
 */
int
Hero::put_off_equipment(uint32_t get_tm)
{
	Equipment *p_equip = owner->equip_mgr->get_equip(get_tm);
	if (!p_equip) {
		T_KWARN_LOG(owner->user_id, "put off equip not exist\t[get_tm=%u]", get_tm);
		return cli_equip_not_exist_err;
	}
	uint32_t equip_id = p_equip->id;
	const equip_xml_info_t *p_info = equip_xml_mgr->get_equip_xml_info(equip_id);
	if (!p_info) {
		T_KWARN_LOG(owner->user_id, "put on equip invalid\t[equip_id=%u]", equip_id);
		return cli_invalid_equip_err;
	}

	//更新缓存
	p_equip->hero_id = 0;

	equips.erase(get_tm);

	//更新属性
	send_hero_attr_change_noti();

	//更新DB
	db_hero_put_off_equipment_in db_in;
	db_in.get_tm = get_tm;

	send_msg_to_dbroute(0, db_hero_put_off_equipment_cmd, &db_in, owner->user_id);

	return 0;
}

/* @brief 英雄升阶
 */
int
Hero::rising_rank()
{
	if (rank >= MAX_HERO_RANK) {
		T_KWARN_LOG(owner->user_id, "rising rank over max rank\t[rank=%u]", rank);
		return cli_hero_reach_max_rank_err;
	}

	uint32_t army_id = this->base_info->army;

	const hero_rank_stuff_detail_xml_info_t *p_info = hero_rank_stuff_xml_mgr->get_hero_rank_stuff_xml_info(army_id, rank);
	if (!p_info) {
		T_KWARN_LOG(owner->user_id, "rising rank stuff not exist\t[army_id=%u, rank=%u]", army_id, rank);
		return cli_hero_cannot_rising_rank_err;
	}

	//检查本体卡数量
	uint32_t hero_card_id = 55 * 10000 + this->id;
	if (p_info->hero_card_cnt) {
		uint32_t hero_card_cnt = owner->items_mgr->get_item_cnt(hero_card_id);
		if (hero_card_cnt < p_info->hero_card_cnt) {
			T_KWARN_LOG(owner->user_id, "rising rank need hero card not enough\t[cur_cnt=%u, need_cnt=%u]", hero_card_cnt, p_info->hero_card_cnt);
			return cli_not_enough_item_err;
		}
	}

	//检查金币数量
	if (owner->golds < p_info->golds) {
		T_KWARN_LOG(owner->user_id, "rising rank need golds not enough\t[golds=%u, need_golds=%u]", owner->golds, p_info->golds);
		return cli_not_enough_golds_err;
	}

	for (int i = 0; i < 2; i++) {
		uint32_t item_cnt = owner->items_mgr->get_item_cnt(p_info->stuff[i].stuff_id);
		if (item_cnt < p_info->stuff[i].num) {
			T_KWARN_LOG(owner->user_id, "rising rank need stuff not enough\t[stuff_id=%u, cnt=%u, need_cnt=%u]", 
					p_info->stuff[i].stuff_id, item_cnt, p_info->stuff[i].num);
			return cli_not_enough_item_err;
		}
	}

	//扣除本体卡
	if (p_info->hero_card_cnt) {
		owner->items_mgr->del_item_without_callback(hero_card_id, p_info->hero_card_cnt);
	}

	//扣除金币
	if (p_info->golds) {
		owner->chg_golds(-p_info->golds);
	}

	//扣除进阶材料
	for (int i = 0; i < 2; i++) {
		if (p_info->stuff[i].stuff_id) {
			owner->items_mgr->del_item_without_callback(p_info->stuff[i].stuff_id, p_info->stuff[i].num);
		}
	}

	//进阶
	rank++;

	//更新属性
	send_hero_attr_change_noti();

	if (this->id == owner->role_id) {//主角 
		const arena_info_t *p_info = arena_mgr->get_arena_ranking_info_by_userid(owner->user_id);
		if (p_info) {
			arena_mgr->update_ranking_info_rank(owner->user_id, rank);
		}
	}

	//更新DB
	db_hero_rising_rank_in db_in;
	db_in.hero_id = this->id;
	send_msg_to_dbroute(0, db_hero_rising_rank_cmd, &db_in, owner->user_id);

	//检查任务
	uint32_t rank_cnt = owner->hero_mgr->calc_over_cur_rank_hero_cnt(rank);
	owner->task_mgr->check_task(em_task_type_hero_rank, rank, rank_cnt);
	owner->achievement_mgr->check_achievement(em_achievement_type_hero_rank, rank, rank_cnt);

	T_KDEBUG_LOG(owner->user_id, "HERO RISING RANK\t[hero_id=%u, rank=%u]", this->id, this->rank);

	return 0;
}

/* @brief 英雄吃经验道具
 */
int
Hero::eat_exp_items(uint32_t item_id, uint32_t item_cnt)
{
	const item_xml_info_t *p_info = items_xml_mgr->get_item_xml_info(item_id);
	if (!p_info || p_info->type != em_item_type_for_hero_exp) {
		T_KWARN_LOG(owner->user_id, "invalid hero exp item\t[item_id=%u]", item_id);
		return cli_invalid_item_err;
	}

	uint32_t cur_cnt = owner->items_mgr->get_item_cnt(item_id);
	if (cur_cnt < item_cnt) {
		T_KWARN_LOG(owner->user_id, "eat hero exp items not enough\t[cur_cnt=%u, eat_cnt=%u]", cur_cnt, item_cnt);
		return cli_not_enough_item_err;
	}

	//增加经验
	uint32_t total_exp = p_info->effect * item_cnt;
	uint32_t real_add_exp = add_exp(total_exp);

	uint32_t del_cnt = ceil((double)real_add_exp / p_info->effect);

	//扣除物品
	owner->items_mgr->del_item_without_callback(item_id, del_cnt);

	return 0;
}

/* @brief 英雄吃战功道具
 */
int
Hero::eat_honor_items(uint32_t item_id, uint32_t item_cnt)
{
	const item_xml_info_t *p_info = items_xml_mgr->get_item_xml_info(item_id);
	if (!p_info || p_info->type != em_item_type_for_hero_honor) {
		T_KWARN_LOG(owner->user_id, "invalid hero honor item\t[item_id=%u]", item_id);
		return cli_invalid_item_err;
	}

	uint32_t cur_cnt = owner->items_mgr->get_item_cnt(item_id);
	if (cur_cnt < item_cnt) {
		T_KWARN_LOG(owner->user_id, "eat hero honor items not enough\t[cur_cnt=%u, eat_cnt=%u]", cur_cnt, item_cnt);
		return cli_not_enough_item_err;
	}

	//扣除物品
	owner->items_mgr->del_item_without_callback(item_id, item_cnt);

	//增加战功
	uint32_t total_honor = p_info->effect * item_cnt;
	uint32_t real_add_honor = add_honor(total_honor, true);

	uint32_t del_cnt = ceil((double)real_add_honor / p_info->effect);

	//扣除物品
	owner->items_mgr->del_item_without_callback(item_id, del_cnt);

	return 0;
}

/* @brief 查找英雄战魂
 */
BtlSoul*
Hero::get_hero_btl_soul(uint32_t get_tm)
{
	BtlSoulMap::iterator it = btl_souls.find(get_tm);
	if (it != btl_souls.end()) {
		return it->second;
	}

	return 0;
}

/* @brief 查找同类型战魂
 */
BtlSoul *
Hero::get_same_type_btl_soul(uint32_t type)
{
	BtlSoulMap::iterator it = btl_souls.begin();
	for (; it != btl_souls.end(); ++it) {
		BtlSoul *btl_soul = it->second;
		if (btl_soul->base_info->type == type) {
			return btl_soul;
		}
	}

	return 0;
}

/* @brief 英雄装备战魂
 */
int
Hero::put_on_btl_soul(uint32_t get_tm, uint32_t &off_get_tm)
{	
	BtlSoul *btl_soul = owner->btl_soul_mgr->get_btl_soul(get_tm);
	if (!btl_soul) {
		T_KWARN_LOG(owner->user_id, "put on hero btl soul not exist\t[get_tm=%u]", get_tm);
		return cli_btl_soul_not_exist_err;
	}

	BtlSoul *hero_btl_soul = this->get_hero_btl_soul(get_tm);
	if (hero_btl_soul) {
		T_KWARN_LOG(owner->user_id, "already put on hero btl soul\t[get_tm=%u]", get_tm);
		return cli_btl_soul_already_put_on_err;
	}

	//查找是否有同类型战魂
	BtlSoul *p_same_btl_soul = this->get_same_type_btl_soul(btl_soul->base_info->type);
	if (p_same_btl_soul) {//替换
		off_get_tm = p_same_btl_soul->get_tm;
		p_same_btl_soul->hero_id = 0;
		btl_souls.erase(p_same_btl_soul->get_tm);
		//更新DB
		db_set_btl_soul_hero_info_in db_in;
		db_in.get_tm = p_same_btl_soul->get_tm;
		db_in.hero_id = 0;

		send_msg_to_dbroute(owner, db_set_btl_soul_hero_info_cmd, &db_in, owner->user_id);
	} 

	//更新缓存
	btl_soul->hero_id = this->id;
	btl_souls.insert(BtlSoulMap::value_type(get_tm, btl_soul));
	//更新DB
	db_set_btl_soul_hero_info_in db_in;
	db_in.hero_id = this->id;
	db_in.get_tm = get_tm;

	send_msg_to_dbroute(0, db_set_btl_soul_hero_info_cmd, &db_in, owner->user_id);

	return 0;
}

/* @brief 英雄卸下战魂
 */
int
Hero::put_off_btl_soul(uint32_t get_tm)
{
	BtlSoul *btl_soul = owner->btl_soul_mgr->get_btl_soul(get_tm);
	if (!btl_soul) {
		T_KWARN_LOG(owner->user_id, "put off hero btl soul not exist\t[get_tm=%u]", get_tm);
		return cli_btl_soul_not_exist_err;
	}

	BtlSoul *hero_btl_soul = this->get_hero_btl_soul(get_tm);
	if (!hero_btl_soul) {
		T_KWARN_LOG(owner->user_id, "already put off hero btl soul\t[get_tm=%u]", get_tm);
		return cli_btl_soul_already_put_off_err;
	}

	//更新缓存
	btl_souls.erase(get_tm);

	//更新DB
	db_set_btl_soul_hero_info_in db_in;
	db_in.get_tm = get_tm;
	db_in.hero_id = 0;

	send_msg_to_dbroute(owner, db_set_btl_soul_hero_info_cmd, &db_in, owner->user_id);

	return 0;
}

double
Hero::calc_hero_base_btl_power()
{
	uint32_t calc_star = star;
	if (star > 5) {
		calc_star = 5;
	}
	double star_power[] = {0, 0.4, 0.7, 0.95, 1, 1};

	const level_xml_info_t *p_info = level_xml_mgr->get_level_xml_info(lv);
	if (!p_info) {
		return 0;
	}
	//等级战斗力
	uint32_t base_btl_power = LEVEL_BASE_BTL_POWER * p_info->btl_power_scale * star_power[calc_star];

	return base_btl_power;
}

double
Hero::calc_hero_skill_btl_power()
{
	if (id != owner->role_id) {//非主公
		return skill_lv * 100 + unique_skill_lv * 100;
	}

	uint32_t role_skill_btl_power = 0;
	for (int i = 0; i < 3; i++) {
		uint32_t res_type = forever_main_hero_skill_1 + i;
		uint32_t skill_id = owner->res_mgr->get_res_value(res_type);
		if (skill_id) {
			const role_skill_xml_info_t *p_xml_info = role_skill_xml_mgr->get_role_skill_xml_info_by_skill(skill_id);
			if (p_xml_info) {
				uint32_t skill_level_type = forever_main_hero_skill_1_lv + p_xml_info->id - 1;
				uint32_t skill_level = owner->res_mgr->get_res_value(skill_level_type);
				role_skill_btl_power += skill_level * 100;
			}
		}
	}

	return role_skill_btl_power;
}

double
Hero::calc_hero_equip_btl_power()
{
	uint32_t rank_power[] = {0, 1, 2, 3, 4, 5};
	uint32_t refining_power[] = {0, 50, 100, 150, 200, 250, 300, 350, 400};
	double talent_power[] = {0, 3.2, 8, 16, 25.6, 36.8, 49.6, 64, 80}; 
	double equip_btl_power = 0;
	EquipmentMap::iterator it = equips.begin();
	for (; it != equips.end(); ++it) {
		Equipment *p_equip = it->second;
		const level_xml_info_t *p_info = level_xml_mgr->get_level_xml_info(p_equip->lv);
		if (!p_info) {
			continue;
		}
		//基础战斗力
		double base_power = EQUIP_BASE_BTL_POWER * p_info->btl_power_scale * rank_power[p_equip->base_info->rank];

		uint32_t calc_refining_lv = p_equip->refining_lv;
		if (calc_refining_lv > 8) {
			calc_refining_lv = 8;
		}
		//精炼本身加成
		double refining_base_power = refining_power[calc_refining_lv] * rank_power[p_equip->base_info->rank];
		//精炼天赋加成
		double refining_talent_power = talent_power[calc_refining_lv] * rank_power[p_equip->base_info->rank];

		equip_btl_power += (base_power + refining_base_power + refining_talent_power);
	}

	return equip_btl_power;
}

double
Hero::calc_hero_rank_btl_power()
{
	double star_power[] = {0, 0.4, 0.7, 0.95, 1, 1};
	uint32_t rank_power[] = {0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000};
	uint32_t talent_power[] = {0, 240, 600, 1200, 1920, 2760, 3720, 4800, 6000};

	uint32_t calc_rank = rank;
	if (calc_rank > 8) {
		calc_rank = 8;
	}

	double rank_base_power = rank_power[calc_rank] * star_power[this->star];
	double rank_talent_power = talent_power[calc_rank] * star_power[this->star];

	return rank_base_power + rank_talent_power;
}

double
Hero::calc_hero_btl_soul_btl_power()
{
	double btl_soul_btl_power = 0;
	BtlSoulMap::iterator it = btl_souls.begin();
	for (; it != btl_souls.end(); ++it) {
		BtlSoul *btl_soul = it->second;
		const level_xml_info_t *p_info = level_xml_mgr->get_level_xml_info(btl_soul->lv);
		if (!btl_soul || !p_info) {
			continue;
		}
		double power = BTL_SOUL_BASE_BTL_POWER * btl_soul->base_info->rank * p_info->btl_power_scale;

		btl_soul_btl_power += power;
	}

	return btl_soul_btl_power;
}

double
Hero::calc_hero_horse_btl_power()
{
	double horse_btl_power = 0;
	uint32_t factor = 1;
	if (owner->horse_lv <= 10) {
		factor = 1;
	} else if (owner->horse_lv <= 20) {
		factor = 2;
	} else if (owner->horse_lv <= 30) {
		factor = 3;
	} else if (owner->horse_lv <= 40) {
		factor = 4;
	} else {
		factor = 5;
	}
	horse_btl_power = 200 + 15 * factor;

	return horse_btl_power;
}

double 
Hero::calc_hero_honor_btl_power()
{
	const hero_honor_exp_xml_info_t *p_info = hero_honor_exp_xml_mgr->get_hero_honor_exp_xml_info(this->honor_lv);
	if (p_info) {
		return p_info->btl_power;
	}

	return 0;
}

/* @brief 计算英雄当前战斗力
 */
int
Hero::calc_hero_btl_power()
{
	uint32_t old_btl_power = btl_power;

	double base_btl_power = calc_hero_base_btl_power();
	double skill_btl_power = calc_hero_skill_btl_power();
	double equip_btl_power =  calc_hero_equip_btl_power();
	double rank_btl_power = calc_hero_rank_btl_power();
	double btl_soul_btl_power = calc_hero_btl_soul_btl_power();
	double horse_btl_power = calc_hero_horse_btl_power();
	double honor_btl_power = calc_hero_honor_btl_power();

	btl_power = base_btl_power + skill_btl_power + equip_btl_power + rank_btl_power + btl_soul_btl_power + horse_btl_power + honor_btl_power; 

	if (old_btl_power != btl_power) { 
		//通知前端更新英雄战斗力
		cli_hero_btl_power_change_noti_out noti_out;
		noti_out.hero_id = this->id;
		noti_out.old_btl_power = old_btl_power;
		noti_out.btl_power = btl_power;
		owner->send_to_self(cli_hero_btl_power_change_noti_cmd, &noti_out, 0);

		if (state == 1) {
			owner->calc_btl_power();
		}

		if (arena_mgr->is_defend_hero(owner->user_id, this->id)) {
			arena_mgr->update_ranking_info_btl_power(owner);
		}

		T_KTRACE_LOG(owner->user_id, "hero btl power changed\t[%u %u %u]", this->id, old_btl_power, btl_power);
	}

	return btl_power;
}

int
Hero::calc_hero_max_lv()
{
	if (!owner) {
		return 0;
	}
	if (this->id == owner->role_id) {//主角
		return MAX_HERO_LEVEL;	
	}
	
	Hero *p_hero = owner->hero_mgr->get_hero(owner->role_id);
	if (!p_hero) {
		return 0;
	}
	const level_xml_info_t *p_info = level_xml_mgr->get_level_xml_info(p_hero->lv);
	if (!p_info) {
		return 0;
	}

	return p_info->max_lv;
}

int
Hero::calc_hero_growth()
{
	if (!base_info || rank > MAX_HERO_RANK) {
		return 0;
	}

	/*
	str_growth = base_info->str_growth[rank - 1];
	agi_growth = base_info->agi_growth[rank - 1];
	int_growth = base_info->int_growth[rank - 1];
	*/

	return 0;
}

int
Hero::calc_cur_rank_attr(hero_attr_info_t &rank_attr)
{
	const hero_rank_detail_xml_info_t *p_info = hero_rank_xml_mgr->get_hero_rank_xml_info(this->id, this->rank);
	if (p_info) {
		memcpy(&rank_attr, &(p_info->attr), sizeof(rank_attr));
	}

	return 0;
}

int
Hero::calc_cur_talent_attr(hero_attr_info_t &talent_attr1, hero_attr_info_t &talent_attr2)
{
	for (uint32_t i = 1; i <= this->rank; i++) {
		const hero_rank_detail_xml_info_t *p_rank_info = hero_rank_xml_mgr->get_hero_rank_xml_info(this->id, i);
		if (p_rank_info && p_rank_info->talent_id) {
			const hero_talent_xml_info_t *p_xml_info = hero_talent_xml_mgr->get_hero_talent_xml_info(p_rank_info->talent_id);
			if (p_xml_info) {
				int flag = p_xml_info->passive_class == 1 ? 1 : -1;
				hero_attr_info_t &attr = (p_xml_info->passive_type > 0) ? talent_attr2 : talent_attr1; 
				switch (p_xml_info->type) 
				{
					case passive_buff_maxhp: 
						attr.max_hp += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_hp_regain:
						attr.hp_regain += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_atk_spd: 
						attr.atk_spd += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_ad: 
						attr.ad += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_armor: 
						attr.armor += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_resist: 
						attr.resist += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_all_def: 
						attr.armor += (flag * p_xml_info->passive_value);
						attr.resist += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_ad_cri: 
						attr.ad_cri += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_ad_ren: 
						attr.ad_ren += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_ad_chuan: 
						attr.ad_chuan += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_ap_chuan: 
						attr.ap_chuan += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_hit: 
						attr.hit += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_miss: 
						attr.miss += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_cri_damage: 
						attr.cri_damage += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_cri_avoid: 
						attr.cri_avoid += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_hp_steal: 
						attr.hp_steal += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_ad_avoid: 
						attr.ad_avoid += (flag * p_xml_info->passive_value);
						break;
					case passive_buff_ap_avoid: 
						attr.ap_avoid += (flag * p_xml_info->passive_value);
						break;
					default:
						break;
				}
			}
		}
	}

	return 0;
}

int
Hero::calc_cur_level_attr(hero_attr_info_t & level_attr)
{
	const hero_level_attr_xml_info_t *p_info = hero_level_attr_xml_mgr->get_hero_level_attr_xml_info(id);
	if (!p_info) {
		return 0;
	}
	level_attr.max_hp += lv * p_info->maxhp;
	level_attr.ad += lv * p_info->ad;
	level_attr.armor += lv * p_info->armor;
	level_attr.resist += lv * p_info->resist;
	level_attr.hp_regain += lv * p_info->hp_regain;

	return 0;
}

void 
Hero::calc_hero_base_attr()
{
	memset(&base_attr, 0x0, sizeof(base_attr));

	//等级加成
	hero_attr_info_t level_attr = {};
	calc_cur_level_attr(level_attr);

	//品阶加成
	hero_attr_info_t rank_attr = {};
	calc_cur_rank_attr(rank_attr);

	//三维待定
	//base_attr.strength = lv_factor * str_growth + rank_attr.strength + level_attr.strength;
	//base_attr.intelligence = lv_factor * str_growth + rank_attr.intelligence + level_attr.strength;
	//base_attr.agility = lv_factor * str_growth + rank_attr.agility + level_attr.agility;
	base_attr.speed = speed;

	base_attr.max_hp = base_info->attr.max_hp + rank_attr.max_hp + level_attr.max_hp;
	base_attr.ad = base_info->attr.ad + rank_attr.ad + level_attr.ad;
	base_attr.atk_spd = base_info->attr.atk_spd + rank_attr.atk_spd + level_attr.atk_spd;
	base_attr.armor = base_info->attr.armor + rank_attr.armor + level_attr.armor;
	base_attr.resist = base_info->attr.resist + rank_attr.resist + level_attr.resist;
	base_attr.ad_cri = base_info->attr.ad_cri + rank_attr.ad_cri + level_attr.ad_cri;
	base_attr.ad_ren = base_info->attr.ad_ren + rank_attr.ad_ren + level_attr.ad_ren;
	base_attr.ad_chuan = base_info->attr.ad_chuan + rank_attr.ad_chuan + level_attr.ad_chuan;
	base_attr.ap_chuan = base_info->attr.ap_chuan + rank_attr.ap_chuan + level_attr.ap_chuan;
	base_attr.hit = base_info->attr.hit + rank_attr.hit + level_attr.hit;
	base_attr.miss = base_info->attr.miss + rank_attr.miss + level_attr.miss;
	base_attr.hp_regain = base_info->attr.hp_regain + rank_attr.hp_regain + level_attr.hp_regain;
}

void
Hero::calc_hero_equip_attr()
{
	memset(&equip_attr, 0x0, sizeof(equip_attr));
	EquipmentMap::iterator it = equips.begin();
	for (; it != equips.end(); ++it) {
		Equipment *p_equip = it->second;

		//强化属性
		equip_attr.max_hp += (p_equip->base_info->attr.max_hp + p_equip->base_info->up_attr.max_hp * p_equip->lv);
		equip_attr.ad += (p_equip->base_info->attr.ad + p_equip->base_info->up_attr.ad * p_equip->lv);
		equip_attr.armor += (p_equip->base_info->attr.armor + p_equip->base_info->up_attr.armor * p_equip->lv);
		equip_attr.resist += (p_equip->base_info->attr.resist + p_equip->base_info->up_attr.resist * p_equip->lv);

		//精炼属性
		const equip_refining_sub_xml_info_t* p_refining_info = equip_refining_xml_mgr->get_equip_refining_xml_info(p_equip->id, p_equip->refining_lv);
		if (p_refining_info) {
			equip_attr.max_hp += (p_refining_info->maxhp + p_refining_info->extra_maxhp);
			equip_attr.ad += (p_refining_info->ad + p_refining_info->extra_ad);
			equip_attr.armor += (p_refining_info->armor + p_refining_info->extra_armor);
			equip_attr.resist += (p_refining_info->resist + p_refining_info->resist);
		}

		//宝石属性
		hero_attr_info_t info;
		p_equip->calc_equip_gem_attr(info);
		equip_attr.max_hp += info.max_hp;
		equip_attr.ad += info.ad;
		equip_attr.armor += info.armor;
		equip_attr.resist += info.resist;
	}
}


void 
Hero::calc_hero_talent_attr()
{
	memset(&talent_attr, 0x0, sizeof(talent_attr));
	//天赋加成
	hero_attr_info_t talent_attr1 = {};
	hero_attr_info_t talent_attr2 = {};
	calc_cur_talent_attr(talent_attr1, talent_attr2);

	//talent_attr.strength = talent_attr1.strength + (base_attr.strength + equip_attr.strength + talent_attr1.strength) * (talent_attr2.strength / 100.0);
	//talent_attr.intelligence = talent_attr1.intelligence + (base_attr.intelligence + equip_attr.intelligence + talent_attr1.intelligence) * (talent_attr2.intelligence / 100.0);
	//talent_attr.agility = talent_attr1.agility + (base_attr.agility + equip_attr.agility + talent_attr1.agility) * (talent_attr2.agility / 100.0);
	talent_attr.speed = talent_attr1.speed + (speed + talent_attr1.speed) * (talent_attr2.speed / 100.0);

	talent_attr.max_hp = talent_attr1.max_hp + (base_attr.max_hp + equip_attr.max_hp + talent_attr1.max_hp) * (talent_attr2.max_hp / 100.0);
	talent_attr.ad = talent_attr1.ad + (base_attr.ad + equip_attr.ad + talent_attr1.ad) * (talent_attr2.ad / 100.0);
	talent_attr.atk_spd = talent_attr1.atk_spd + (base_attr.atk_spd + equip_attr.atk_spd + talent_attr1.atk_spd) * (talent_attr2.atk_spd / 100.0);
	talent_attr.armor = talent_attr1.armor + (base_attr.armor + equip_attr.armor + talent_attr1.armor) * (talent_attr2.armor / 100.0);
	talent_attr.resist = talent_attr1.resist + (base_attr.resist + equip_attr.resist + talent_attr1.resist) * (talent_attr2.resist / 100.0);
	talent_attr.ad_cri = talent_attr1.ad_cri + (base_attr.ad_cri + equip_attr.ad_cri + talent_attr1.ad_cri) * (talent_attr2.ad_cri / 100.0);
	talent_attr.ad_ren = talent_attr1.ad_ren + (base_attr.ad_ren + equip_attr.ad_ren + talent_attr1.ad_ren) * (talent_attr2.ad_ren / 100.0);
	talent_attr.ad_chuan = talent_attr1.ad_chuan + (base_attr.ad_chuan + equip_attr.ad_chuan + talent_attr1.ad_chuan) * (talent_attr2.ad_chuan / 100.0);
	talent_attr.ap_chuan = talent_attr1.ap_chuan + (base_attr.ap_chuan + equip_attr.ap_chuan + talent_attr1.ap_chuan) * (talent_attr2.ap_chuan / 100.0);
	talent_attr.hit = talent_attr1.hit + (base_attr.hit + equip_attr.hit + talent_attr1.hit) * (talent_attr2.hit / 100.0);
	talent_attr.miss = talent_attr1.miss + (base_attr.miss + equip_attr.miss + talent_attr1.miss) * (talent_attr2.miss / 100.0);
	talent_attr.hp_regain = talent_attr1.hp_regain + (base_attr.hp_regain + equip_attr.hp_regain + talent_attr1.hp_regain) * (talent_attr2.hp_regain / 100.0);
}

void 
Hero::calc_hero_btl_soul_attr()
{
	memset(&btl_soul_attr, 0x0, sizeof(btl_soul_attr));
	BtlSoulMap::iterator it = btl_souls.begin();
	for (; it != btl_souls.end(); ++it) {
		BtlSoul *btl_soul = it->second;
		uint32_t effect = btl_soul->base_info->effect * btl_soul->lv;
		switch (btl_soul->base_info->type) {
		case em_btl_soul_effect_maxhp:
			btl_soul_attr.max_hp += effect;
			break;
		case em_btl_soul_effect_ad:
			btl_soul_attr.ad += effect;
			break;
		case em_btl_soul_effect_armor:
			btl_soul_attr.armor += effect;
			break;
		case em_btl_soul_effect_resist:
			btl_soul_attr.resist += effect;
			break;
		case em_btl_soul_effect_armor_chuan:
			btl_soul_attr.ad_chuan += effect;
			break;
		case em_btl_soul_effect_resist_chuan:
			btl_soul_attr.ap_chuan += effect;
			break;
		case em_btl_soul_effect_hit:
			btl_soul_attr.hit += effect;
			break;
		case em_btl_soul_effect_miss:
			btl_soul_attr.miss += effect;
			break;
		case em_btl_soul_effect_cri:
			btl_soul_attr.ad_cri += effect;
			break;
		case em_btl_soul_effect_ren:
			btl_soul_attr.ad_ren += effect;
			break;
		case em_btl_soul_effect_cri_damage:
		case em_btl_soul_effect_cri_avoid:
		case em_btl_soul_effect_final_damage:
		case em_btl_soul_effect_final_avoid:
			break;
		default:
			break;
		}
	}
}

void 
Hero::calc_hero_honor_attr()
{
	memset(&honor_attr, 0x0, sizeof(honor_attr));
	const hero_honor_level_xml_info_t *p_xml_info = hero_honor_xml_mgr->get_hero_honor_xml_info(base_info->army, honor_lv);
	if (p_xml_info) {
		honor_attr.max_hp = p_xml_info->max_hp;
		honor_attr.ad = p_xml_info->ad;
		honor_attr.armor = p_xml_info->armor;
		honor_attr.resist = p_xml_info->resist;
	}
}

void
Hero::calc_hero_horse_attr()
{
	memset(&horse_attr, 0x0, sizeof(horse_attr));
	horse_attr.max_hp = owner->horse->max_hp + owner->horse->out_max_hp;
	horse_attr.ad = owner->horse->ad + owner->horse->out_ad;
	horse_attr.armor = owner->horse->armor + owner->horse->out_armor;
	horse_attr.resist = owner->horse->resist + owner->horse->out_resist;
	horse_attr.hit = owner->horse->hit;
	horse_attr.miss = owner->horse->miss;
}

#define CALC_HERO_ATTR(field)\
	field = base_attr.field + equip_attr.field + talent_attr.field + btl_soul_attr.field + honor_attr.field + horse_attr.field;

void 
Hero::calc_all(bool flag)
{
	//基础属性
	calc_hero_base_attr();

	//装备属性
	calc_hero_equip_attr();

	//天赋属性
	calc_hero_talent_attr();

	//战魂属性
	calc_hero_btl_soul_attr();

	//战功属性
	calc_hero_honor_attr();

	//战马属性
	calc_hero_horse_attr();

	CALC_HERO_ATTR(strength)
	CALC_HERO_ATTR(intelligence)
	CALC_HERO_ATTR(agility)
	CALC_HERO_ATTR(speed)
	CALC_HERO_ATTR(max_hp)
	CALC_HERO_ATTR(ad)
	CALC_HERO_ATTR(atk_spd)
	CALC_HERO_ATTR(armor)
	CALC_HERO_ATTR(resist)
	CALC_HERO_ATTR(ad_cri)
	CALC_HERO_ATTR(ad_ren)
	CALC_HERO_ATTR(ad_chuan)
	CALC_HERO_ATTR(ap_chuan)
	CALC_HERO_ATTR(hit)
	CALC_HERO_ATTR(miss)
	CALC_HERO_ATTR(hp_regain)

	if (!flag) {
		calc_hero_btl_power();
	}
}

int
Hero::init_hero_base_info(uint32_t init_lv)
{
	if (!base_info) {
		return -1;
	}
	get_tm = get_now_tv()->tv_sec;
	rank = 0;
	star = base_info->star;
	lv = init_lv ? init_lv : 1;
	exp = 0;
	honor_lv = 0;
	honor = 0;
	btl_power = 0;
	title = 0;
	state = 0;

	str_growth = base_info->str_growth;
	int_growth = base_info->int_growth;
	agi_growth = base_info->agi_growth;

	strength = base_info->attr.strength;
	agility = base_info->attr.agility;
	intelligence = base_info->attr.intelligence;
	speed = base_info->attr.speed;

	hp = base_info->attr.max_hp;
	max_hp = base_info->attr.max_hp;
	hp_regain = base_info->attr.hp_regain;

	atk_spd = base_info->attr.atk_spd;
	ad = base_info->attr.ad;
	armor = base_info->attr.armor;
	ad_cri = base_info->attr.ad_cri;
	ad_ren = base_info->attr.ad_ren;
	resist = base_info->attr.resist;
	ad_chuan = base_info->attr.ad_chuan;
	ap_chuan = base_info->attr.ap_chuan;
	hit = base_info->attr.hit;
	miss = base_info->attr.miss;

	skill_lv = 1;
	unique_skill_lv = 1;
	state = 0;

	for (uint32_t i = 0; i < base_info->talents.size(); i++) {
		if (i >= rank) {
			break;
		}
		talents.push_back(base_info->talents[i]);
	}

	//calc_all();

	return 0;
}

int
Hero::init_hero_db_info(const db_hero_info_t *p_info)
{
	get_tm = p_info->get_tm;
	lv = p_info->lv;
	exp = p_info->exp;
	honor_lv = p_info->honor_lv;
	honor = p_info->honor;
	rank = p_info->rank;
	star = p_info->star;
	trip_id = p_info->trip_id;
	title = p_info->title;
	state = p_info->state;
	skill_lv = p_info->skill_lv;
	unique_skill_lv = p_info->unique_skill_lv;
	//装备信息在拉取装备后初始化

	T_KTRACE_LOG(owner->user_id, "init hero db info\t[%u %u %u %u %u %u %u %u %u %u %u]",
			id, get_tm, lv, exp, honor_lv, honor, rank, star, title, state, skill_lv);

	return 0;
}

int
Hero::send_hero_attr_change_noti()
{
	calc_all();
	
	cli_hero_attr_change_noti_out noti_out;
	pack_hero_client_info(noti_out.hero_info);

	return owner->send_to_self(cli_hero_attr_change_noti_cmd, &noti_out, 0);
}

int
Hero::get_hero_max_grant_title()
{
	uint32_t max_ranking = owner->res_mgr->get_res_value(forever_arena_history_ranking);
	uint32_t max_title_lv = 0;
	if (max_ranking <= 10 && honor_lv >= 160) {
		max_title_lv = 4;
	} else if (max_ranking <= 100 && honor_lv >= 120) {
		max_title_lv = 3;
	} else if (max_ranking <= 500 && honor_lv >= 80) {
		max_title_lv = 2;
	} else if (max_ranking <= 1000 && honor_lv >= 40) {
		max_title_lv = 1;
	}

	return 0;
}

int
Hero::get_hero_grant_state()
{
	uint32_t max_ranking = owner->res_mgr->get_res_value(forever_arena_history_ranking);
	uint32_t title_lv = 0;
	if (max_ranking <= 10) {
		title_lv = 4;
	} else if (max_ranking <= 100) {
		title_lv = 3;
	} else if (max_ranking <= 500) {
		title_lv = 2;
	} else if (max_ranking <= 1000) {
		title_lv = 1;
	}

	uint32_t cur_title_lv = title / 10;
	if (cur_title_lv >= title_lv) {
		return 1;
	}

	uint32_t need_honor_lv[] = {40, 80, 120, 160};
	if (honor_lv < need_honor_lv[cur_title_lv]) {
		return 2;
	}

	return 0;
}

bool
Hero::check_hero_is_can_grant(uint32_t title_id)
{
	if (!title_id) {
		return false;
	}

	uint32_t max_ranking = owner->res_mgr->get_res_value(forever_arena_history_ranking);
	uint32_t title_lv = 0;
	if (max_ranking <= 10) {
		title_lv = 4;
	} else if (max_ranking <= 100) {
		title_lv = 3;
	} else if (max_ranking <= 500) {
		title_lv = 2;
	} else if (max_ranking <= 1000) {
		title_lv = 1;
	}

	uint32_t grant_title_lv = title_id / 10;
	if (grant_title_lv >= title_lv) {
		return false;
	}

	uint32_t need_honor_lv[] = {40, 80, 120, 160};
	if (honor_lv < need_honor_lv[grant_title_lv]) {
		return false;
	}

	return true;
}

int
Hero::grant_hero_title(uint32_t title_id)
{
	if (title_id) {//分封
		/*if (!check_hero_is_can_grant(title_id)) {
			T_KWARN_LOG(owner->user_id, "hero cannot grant title\t[title_id=%u]", title_id);
			return cli_hero_cannot_grant_title_err;
		}*/
	} else {//卸下
		if (!title) {
			T_KWARN_LOG(owner->user_id, "hero grant already put off");
			return cli_hero_title_already_put_off_err;
		}
	}

	title = title_id;

	//更新DB
	db_update_hero_title_in db_in;
	db_in.hero_id = this->id;
	db_in.title = title;
	send_msg_to_dbroute(0, db_update_hero_title_cmd, &db_in, owner->user_id);

	//检查任务
	if (title_id) {
		owner->task_mgr->check_task(em_task_type_grant_title);
	}

	T_KDEBUG_LOG(owner->user_id, "GRANT HERO TITLE\t[title=%u]", title);

	return 0;
}

int 
Hero::normal_skill_level_up(uint32_t add_lv)
{
	if (!add_lv) {
		add_lv = 1;
	}
	//是否超过英雄等级
	if (skill_lv + add_lv > lv) {
		T_KWARN_LOG(owner->user_id, "hero skill lv over hero lv\t[hero_id=%u, skill_lv=%u, hero_lv=%u, add_lv=%u]", id, skill_lv, lv, add_lv);
		return cli_hero_skill_lv_reach_max_err;
	}

	//技能点是否足够
	if (owner->skill_point < add_lv) {
		T_KWARN_LOG(owner->user_id, "hero skill point not enough\t[skill_point=%u, need_point=%u]", owner->skill_point, add_lv);
		return cli_skill_point_not_enough_err;
	}

	//金币是否足够 
	uint32_t need_golds = 0;
	for (uint32_t i = skill_lv + 1; i <= skill_lv + add_lv; i++) {
		need_golds += skill_levelup_golds_xml_mgr->get_skill_levelup_golds(i);
	}
	if (owner->golds < need_golds) {
		T_KWARN_LOG(owner->user_id, "skill levelup need golds not enough\t[skill_lv=%u, need_golds=%u]", skill_lv, need_golds);
		return cli_not_enough_golds_err;
	}

	//扣除技能点
	owner->chg_skill_point(-add_lv);

	//扣除金币
	if (need_golds) {
		owner->chg_golds(-need_golds);
	}

	//技能升级
	skill_lv += add_lv;

	send_hero_attr_change_noti();

	//检查任务
	owner->task_mgr->check_task(em_task_type_skill_strength);

	//写入DB 
	db_set_hero_skill_lv_in db_in;
	db_in.hero_id = id;
	db_in.skill_lv = skill_lv;
	send_msg_to_dbroute(0, db_set_hero_skill_lv_cmd, &db_in, owner->user_id);

	return 0;
}

int
Hero::unique_skill_level_up(uint32_t add_lv)
{
	if (!add_lv) {
		add_lv = 1;
	}
	//是否超过英雄等级
	if (unique_skill_lv + add_lv > lv) {
		T_KWARN_LOG(owner->user_id, "hero unique skill lv over hero lv\t[hero_id=%u, unique_skill_lv=%u, hero_lv=%u, add_lv=%u]", id, unique_skill_lv, lv, add_lv);
		return cli_hero_skill_lv_reach_max_err;
	}

	//技能点是否足够
	if (owner->skill_point < add_lv) {
		T_KWARN_LOG(owner->user_id, "hero skill point not enough\t[skill_point=%u, need_point=%u]", owner->skill_point, add_lv);
		return cli_skill_point_not_enough_err;
	}

	//金币是否足够 
	uint32_t need_golds = 0;
	for (uint32_t i = unique_skill_lv + 1; i <= unique_skill_lv + add_lv; i++) {
		need_golds += skill_levelup_golds_xml_mgr->get_skill_levelup_golds(i);
	}
	if (owner->golds < need_golds) {
		T_KWARN_LOG(owner->user_id, "skill levelup need golds not enough\t[unique-skill_lv=%u, need_golds=%u]", unique_skill_lv, need_golds);
		return cli_not_enough_golds_err;
	}

	//扣除技能点
	owner->chg_skill_point(-add_lv);

	//扣除金币
	if (need_golds) {
		owner->chg_golds(-need_golds);
	}

	//技能升级
	unique_skill_lv += add_lv;

	send_hero_attr_change_noti();

	//检查任务
	owner->task_mgr->check_task(em_task_type_skill_strength);

	//写入DB 
	db_hero_unique_skill_level_up_in db_in;
	db_in.hero_id = id;
	send_msg_to_dbroute(0, db_hero_unique_skill_level_up_cmd, &db_in, owner->user_id);

	return 0;
}

int
Hero::role_skill_level_up(uint32_t skill_id, uint32_t add_lv)
{
	if (!add_lv) {
		add_lv = 1;
	}
	const role_skill_xml_info_t *p_xml_info = role_skill_xml_mgr->get_role_skill_xml_info_by_skill(skill_id);
	if (!p_xml_info || !p_xml_info->id || p_xml_info->id > 12) {
		T_KWARN_LOG(owner->user_id, "skill level up invalid skill id\t[skill_id=%u]", skill_id);
		return cli_invalid_skill_id_err;
	}

	uint32_t res_type = forever_main_hero_skill_1_lv + p_xml_info->id - 1;
	uint32_t res_value = owner->res_mgr->get_res_value(res_type);
	if (res_value + add_lv > lv) {
		T_KWARN_LOG(owner->user_id, "main hero skill lv over hero lv\t[hero_id=%u, skill_lv=%u, hero_lv=%u, add_lv=%u]", id, res_value, lv, add_lv);
		return cli_hero_skill_lv_reach_max_err;
	}

	//是否已解锁
	if (this->lv < p_xml_info->unlock_lv) {
		T_KWARN_LOG(owner->user_id, "role skill not unlock err\t[lv=%u, unlock_lv=%u]", this->lv, p_xml_info->unlock_lv);
		return cli_role_skill_not_unlock_err;
	}

	//技能点是否足够
	if (owner->skill_point < add_lv) {
		T_KWARN_LOG(owner->user_id, "hero skill point not enough\t[skill_point=%u, add_lv=%u]", owner->skill_point, add_lv);
		return cli_skill_point_not_enough_err;
	}

	//金币是否足够 
	uint32_t need_golds = 0;
	for (uint32_t i = res_value + 1; i <= res_value + add_lv; i++) {
		need_golds += skill_levelup_golds_xml_mgr->get_skill_levelup_golds(i);
	}
	if (owner->golds < need_golds) {
		T_KWARN_LOG(owner->user_id, "skill levelup need golds not enough\t[skill_lv=%u, need_golds=%u]", res_value, need_golds);
		return cli_not_enough_golds_err;
	}

	//扣除技能点
	owner->chg_skill_point(-add_lv);

	//扣除金币
	if (need_golds) {
		owner->chg_golds(-need_golds);
	}

	send_hero_attr_change_noti();

	//检查任务
	owner->task_mgr->check_task(em_task_type_skill_strength);

	owner->res_mgr->set_res_value(res_type, res_value + add_lv);

	return 0;
}

int
Hero::skill_level_up(uint32_t type, uint32_t add_lv) 
{
	if (this->id == owner->role_id) {//主公
		return role_skill_level_up(type, add_lv);
	}
	if (type == 1) {
		return normal_skill_level_up(add_lv);
	} else if (type == 2) {
		return unique_skill_level_up(add_lv);
	}

	return cli_invalid_input_arg_err;
}


void
Hero::pack_hero_simple_info(db_hero_simple_info_t &info)
{
	info.lv = lv;
	info.exp = exp;
}

void
Hero::pack_hero_db_info(db_hero_info_t &info)
{
	info.hero_id = id;
	info.get_tm = get_tm;
  	info.lv = lv;
	info.exp = exp;
	info.rank = rank;
	info.star = star;
	info.state = state;
	info.skill_lv = skill_lv;
}

void
Hero::pack_hero_client_info(cli_hero_info_t &info)
{
	info.hero_id = id;
	info.lv = lv;
	info.exp = exp;
	info.level_exp = get_level_up_exp(lv);
	info.honor_lv = honor_lv;
	info.honor = honor;
	info.level_honor = get_level_up_honor(honor_lv);
	info.rank = rank;
	info.star = star;
	info.max_grant_title = get_hero_max_grant_title();
	info.trip_id = trip_id;
	info.title = title;
	info.state = state;
	info.btl_power = btl_power;
	info.skill_lv = skill_lv;
	info.unique_skill_lv = unique_skill_lv;
	info.max_lv = calc_hero_max_lv();
	pack_hero_attr_info(em_hero_attr_type_base, info.base_attr_info);
	pack_hero_attr_info(em_hero_attr_type_equip, info.equip_attr_info);
	pack_hero_attr_info(em_hero_attr_type_talent, info.talent_attr_info);
	pack_hero_attr_info(em_hero_attr_type_btl_soul, info.btl_soul_attr_info);
	pack_hero_attr_info(em_hero_attr_type_honor, info.honor_attr_info);
	pack_hero_attr_info(em_hero_attr_type_horse, info.horse_attr_info);

	pack_hero_equips_info(info.equips);
	pack_hero_btl_soul_info(info.btl_souls);
}

void
Hero::pack_hero_attr_info(uint32_t type, common_hero_attr_info_t &info)
{
	hero_attr_info_t *p_info;
	if (type == em_hero_attr_type_base) {
		p_info = &base_attr;
	} else if (type == em_hero_attr_type_equip) {
		p_info = &equip_attr;
	} else if (type == em_hero_attr_type_talent) {
		p_info = &talent_attr;
	} else if (type == em_hero_attr_type_btl_soul) {
		p_info = &btl_soul_attr;
	} else if (type == em_hero_attr_type_honor) {
		p_info = &honor_attr;
	} else if (type == em_hero_attr_type_horse) {
		p_info = &horse_attr;
	} else {
		return;
	}

	info.strength = p_info->strength;
	info.intelligence = p_info->intelligence;
	info.agility = p_info->agility;
	info.speed = p_info->speed;

	info.max_hp = p_info->max_hp;
	info.atk_spd = p_info->atk_spd;
	info.ad = p_info->ad;
	info.armor = p_info->armor;
	info.ad_cri = p_info->ad_cri;
	info.ad_ren = p_info->ad_ren;
	info.resist = p_info->resist;
	info.ad_chuan = p_info->ad_chuan;
	info.ap_chuan = p_info->ap_chuan;
	info.hit = p_info->hit;
	info.miss = p_info->miss;
	info.hp_regain = p_info->hp_regain;

	T_KTRACE_LOG(owner->user_id, "PACK HERO ATTR INFO\t[%u]:[%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u]", type,
			info.strength, info.intelligence, info.agility, info.speed, info.max_hp, info.atk_spd, info.ad, info.armor, info.ad_cri, info.ad_ren, 
			info.resist, info.ad_chuan, info.ap_chuan, info.hit, info.miss, info.hp_regain);
}

void 
Hero::pack_hero_equips_info(vector<cli_equip_info_t> &equip_vec)
{
	EquipmentMap::iterator it = equips.begin();
	for (; it != equips.end(); ++it) {
		Equipment *p_equip = it->second;
		cli_equip_info_t info;
		info.equip_id = p_equip->id;
		info.get_tm = p_equip->get_tm;
		info.lv = p_equip->lv;
		info.exp = p_equip->exp;
		info.refining_lv = p_equip->refining_lv;
		for (int i = 0; i < 3; i++) {
			info.gem[i] = p_equip->gem[i];
		}
		info.gem_stat = p_equip->gem_stat;

		equip_vec.push_back(info);

		T_KTRACE_LOG(owner->user_id, "PACK HERO EQUIPMENT INFO\t[%u %u %u %u %u %u %u %u %u]", 
				info.equip_id, info.get_tm, info.lv, info.exp, info.refining_lv, info.gem[0], info.gem[1], info.gem[2], info.gem_stat);
	}
}

void 
Hero::pack_hero_btl_soul_info(vector<cli_btl_soul_info_t> &btl_soul_vec)
{
	BtlSoulMap::iterator it = btl_souls.begin();
	for (; it != btl_souls.end(); ++it) {
		BtlSoul *btl_soul = it->second;
		cli_btl_soul_info_t info;
		info.id = btl_soul->id;
		info.get_tm = btl_soul->get_tm;
		info.lv = btl_soul->lv;
		info.exp = btl_soul->exp;
		btl_soul_vec.push_back(info);

		T_KTRACE_LOG(owner->user_id, "PACK HERO BTL SOUL INFO\t[%u %u %u %u]", info.id, info.get_tm, info.lv, info.exp);
	}
}

/********************************************************************************/
/*							HeroManager Class									*/
/********************************************************************************/

HeroManager::HeroManager(Player *p) : owner(p) 
{

}

HeroManager::~HeroManager()
{
	HeroMap::iterator it = heros.begin();
	for (; it != heros.end(); it++) {
		Hero *p_hero = it->second;
		SAFE_DELETE(p_hero);
	}
}

int
HeroManager::calc_hero_max_lv()
{
	Hero *p_main_hero = get_hero(owner->role_id);
	if (p_main_hero) {
		const level_xml_info_t *p_info = level_xml_mgr->get_level_xml_info(p_main_hero->lv);
		if (p_info) {
			return p_info->max_lv;
		}
	}

	return MAX_HERO_LEVEL;	
}

Hero*
HeroManager::get_hero(uint32_t hero_id)
{
	map<uint32_t, Hero*>::iterator it = heros.find(hero_id);
	if (it == heros.end()) {
		return 0;
	}

	return it->second;
}

Hero* 
HeroManager::add_hero(uint32_t hero_id)
{
	const hero_xml_info_t *p_info = hero_xml_mgr->get_hero_xml_info(hero_id);
	if (!p_info) {
		KERROR_LOG(owner->user_id, "invalid hero id, hero_id=%u", hero_id);
		return 0;
	}

	//是否拥有该英雄
	if (get_hero(hero_id)) {
		KERROR_LOG(owner->user_id, "already owner the hero, hero_id=%u", hero_id);
		return 0;
	}

	Hero *hero = new Hero(owner, hero_id);
	if (!hero) {
		return 0;
	}

	//初始化英雄信息
	hero->init_hero_base_info();
	hero->calc_all();

	//插入map
	heros.insert(make_pair(hero_id, hero));

	//更新DB 
	db_add_hero_info_in db_in;
	hero->pack_hero_db_info(db_in.hero_info);
	send_msg_to_dbroute(0, db_add_hero_info_cmd, &db_in, owner->user_id);

	//通知前端
	cli_add_hero_noti_out noti_out;
	hero->pack_hero_client_info(noti_out.hero_info);
	owner->send_to_self(cli_add_hero_noti_cmd, &noti_out, 0);

	KDEBUG_LOG(owner->user_id, "ADD HERO\t[hero_id=%u]", hero_id);
	
	return hero;
}

int
HeroManager::set_trip_hero(uint32_t hero_id, uint32_t trip_id)
{
	if (trip_id > 8) {
		return cli_invalid_input_arg_err;
	}

	Hero *p_hero = this->get_hero(hero_id);
	if (!p_hero) {
		T_KWARN_LOG(owner->user_id, "set trip hero err, hero not exists\t[hero_id=%u]", hero_id);
		return  cli_invalid_hero_err;
	}
	
	if (owner->check_is_troop_hero(hero_id)) {
		T_KWARN_LOG(owner->user_id, "hero is troop hero\t[hero_id=%u]", hero_id);
		return cli_hero_is_troop_hero_err;
	}

	p_hero->trip_id = trip_id;

	//更新DB
	db_set_hero_trip_in db_in;
	db_in.hero_id = hero_id;
	db_in.trip_id = trip_id;
	send_msg_to_dbroute(0, db_set_hero_trip_cmd, &db_in, owner->user_id);
	
	return 0;
}

int
HeroManager::init_all_heros_info(db_get_player_heros_info_out *p_in)
{
	vector<db_hero_info_t>::iterator it = p_in->heros.begin();
	for (; it != p_in->heros.end(); it++) {
		db_hero_info_t *p_info = &(*it);
		const hero_xml_info_t *hero_info = hero_xml_mgr->get_hero_xml_info(p_info->hero_id);
		if (!hero_info) {
			KERROR_LOG(owner->user_id, "invalid hero id, hero_id=%u", p_info->hero_id);
			return -1;
		}

		Hero *hero = new Hero(owner, p_info->hero_id);
		if (!hero) {
			return -1;
		}
		hero->init_hero_base_info(p_info->lv);
		hero->init_hero_db_info(p_info);
		hero->set_hero_status(hero->state);
		hero->calc_all();

		heros.insert(HeroMap::value_type(hero->id, hero));
		T_KTRACE_LOG(owner->user_id, "init all heros info\t[%u %u %u]", hero->id, hero->lv, hero->state);
	}

	owner->calc_btl_power();

	return 0;
}

int
HeroManager::calc_btl_power()
{
	uint32_t btl_power = 0;
	HeroMap::iterator it = heros.begin();
	for (; it != heros.end(); ++it) {
		Hero *p_hero = it->second;
		if (p_hero->state == 1) {
			btl_power += p_hero->btl_power;
		}
	}

	return btl_power;
}

int
HeroManager::calc_all_heros_honor_lv()
{
	uint32_t honor_lv = 0;
	HeroMap::iterator it = heros.begin();
	for (; it != heros.end(); ++it) {
		Hero *p_hero = it->second;
		if (p_hero) {
			honor_lv += p_hero->honor_lv;
		}
	}

	return honor_lv;
}

int
HeroManager::calc_over_cur_lv_hero_cnt(uint32_t lv)
{
	int cnt = 0;
	HeroMap::iterator it = heros.begin();
	for (; it != heros.end(); ++it) {
		Hero *p_hero = it->second;
		if (!p_hero || p_hero->id == owner->role_id) {
			continue;
		}
		if (p_hero->lv >= lv) {
			cnt++;
		}
	}

	return cnt;
}

int
HeroManager::calc_over_cur_rank_hero_cnt(uint32_t rank)
{
	int cnt = 0;
	HeroMap::iterator it = heros.begin();
	for (; it != heros.end(); ++it) {
		Hero *p_hero = it->second;
		if (!p_hero || p_hero->id == owner->role_id) {
			continue;
		}
		if (p_hero->rank >= rank) {
			cnt++;
		}
	}

	return cnt;
}

int
HeroManager::calc_over_cur_star_hero_cnt(uint32_t star)
{
	int cnt = 0;
	HeroMap::iterator it = heros.begin();
	for (; it != heros.end(); ++it) {
		Hero *p_hero = it->second;
		if (!p_hero || p_hero->id == owner->role_id) {
			continue;
		}
		if (p_hero->star >= star) {
			cnt++;
		}
	}

	return cnt;
}

int
HeroManager::calc_over_cur_honor_lv_hero_cnt(uint32_t honor_lv)
{
	int cnt = 0;
	HeroMap::iterator it = heros.begin();
	for (; it != heros.end(); ++it) {
		Hero *p_hero = it->second;
		if (!p_hero || p_hero->id == owner->role_id) {
			continue;
		}
		if (p_hero->honor_lv >= honor_lv) {
			cnt++;
		}
	}

	return cnt;
}

int
HeroManager::deal_hero_title_grant_state()
{
	cli_hero_title_grant_state_change_noti_out noti_out;
	HeroMap::iterator it = heros.begin();
	for (; it != heros.end(); it++) {
		Hero *p_hero = it->second;
		if (p_hero) {
			cli_hero_title_grant_info_t info;
			info.hero_id = p_hero->id;
			info.max_grant_title = p_hero->get_hero_max_grant_title();
			noti_out.heros_state.push_back(info);
		}
	}

	if (noti_out.heros_state.size() > 0) {
		return owner->send_to_self(cli_hero_title_grant_state_change_noti_cmd, &noti_out, 0);
	}
	
	return 0;
}

int
HeroManager::pack_all_heros_info(cli_get_heros_info_out &out)
{
	HeroMap::iterator it = heros.begin();
	for (; it != heros.end(); it++) {
		Hero *hero = it->second;
		cli_hero_info_t info;
		hero->pack_hero_client_info(info);

		out.heros.push_back(info);
	}

	return 0;
}


/********************************************************************************/
/*						HeroXmlManager Class									*/
/********************************************************************************/
HeroXmlManager::HeroXmlManager()
{

}

HeroXmlManager::~HeroXmlManager()
{

}

int 
HeroXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_hero_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);
	return ret; 
}

int
HeroXmlManager::load_hero_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("hero"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");

			HeroXmlMap::iterator it = hero_xml_map.find(id);
			if (it != hero_xml_map.end()) {
				KERROR_LOG(0, "load hero xml info err, hero id exists\t[id=%u]", id);
				return -1;
			}

			hero_xml_info_t info = {};
			info.id = id;
			get_xml_prop(info.camp, cur, "camp");
			get_xml_prop(info.sex, cur, "sex");
			get_xml_prop(info.star, cur, "star");
			get_xml_prop(info.str_growth, cur, "li");
			get_xml_prop(info.int_growth, cur, "zhi");
			get_xml_prop(info.agi_growth, cur, "min");
			get_xml_prop(info.range, cur, "range");
			get_xml_prop(info.army, cur, "army");
			get_xml_prop(info.atk_type, cur, "atk_type");
			get_xml_prop(info.def_type, cur, "def_type");
			get_xml_prop(info.attr.speed, cur, "move_spd");
			get_xml_prop(info.attr.max_hp, cur, "maxhp");
			get_xml_prop(info.attr.hp_regain, cur, "hp_regain");
			double atk_spd;
			get_xml_prop(atk_spd, cur, "atk_spd");
			info.attr.atk_spd = atk_spd * 100;
			get_xml_prop(info.attr.ad, cur, "ad");
			get_xml_prop(info.attr.armor, cur, "armor");
			get_xml_prop(info.attr.ad_cri, cur, "ad_cri");
			get_xml_prop(info.attr.ad_ren, cur, "ad_ren");
			get_xml_prop(info.attr.resist, cur, "resist");
			get_xml_prop(info.attr.hit, cur, "hit");
			get_xml_prop(info.attr.miss, cur, "miss");

			for (int i = 0; i < 6; i++) {
				char skill[16] = {0};
				sprintf(skill, "skill_%d", i + 1);
				int skill_id = 0;
				get_xml_prop(skill_id, cur, skill);
				if (skill_id != -1) {
					info.skills.push_back(skill_id);
				}
			}

			for (int i = 0; i < 9; i++) {
				char talent[16] = {0};
				sprintf(talent, "talent_%d", i + 1);
				int talent_id = 0;
				get_xml_prop(talent_id, cur, talent);
				if (talent_id != -1) {
					info.talents.push_back(talent_id);
				}
			}


			KTRACE_LOG(0, "load hero xml info\t[%u %u %u %u %u %u %u %u %u %u %u %u %f %u %u %f %f %f %u %u]", 
					info.id, info.camp, info.sex, info.star, info.str_growth, info.int_growth, info.agi_growth,
					info.range, info.army, info.atk_type, info.def_type, 
					info.attr.speed, info.attr.max_hp, info.attr.hp_regain, info.attr.atk_spd,
					info.attr.ad, info.attr.armor, info.attr.resist, info.attr.hit, info.attr.miss); 

			hero_xml_map.insert(HeroXmlMap::value_type(id, info));
		}

		cur = cur->next;
	}

	return 0;
}

const hero_xml_info_t*
HeroXmlManager::get_hero_xml_info(uint32_t hero_id)
{
	HeroXmlMap::iterator it = hero_xml_map.find(hero_id);
	if (it == hero_xml_map.end()) {
		return 0;
	}

	return &(it->second);
}


/********************************************************************************/
/*						HeroRankXmlManager Class								*/
/********************************************************************************/
HeroRankXmlManager::HeroRankXmlManager()
{

}

HeroRankXmlManager::~HeroRankXmlManager()
{

}

const hero_rank_detail_xml_info_t*
HeroRankXmlManager::get_hero_rank_xml_info(uint32_t hero_id, uint32_t rank)
{
	HeroRankXmlMap::const_iterator it = hero_map.find(hero_id);
	if (it == hero_map.end()) {
		return 0;
	}
	HeroRankDetailXmlMap::const_iterator it2 = it->second.rank_map.find(rank);
	if (it2 == it->second.rank_map.end()) {
		return 0;
	}

	return &(it2->second);
}

int
HeroRankXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_hero_rank_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
HeroRankXmlManager::load_hero_rank_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("hero"))) {
			uint32_t hero_id = 0;
			get_xml_prop(hero_id, cur, "id");
			HeroRankXmlMap::iterator it = hero_map.find(hero_id);
			if (it != hero_map.end()) {
				ERROR_LOG("load hero rank err, id exists, hero_id=%u", hero_id);
				return -1;
			}

			hero_rank_xml_info_t info;
			info.hero_id = hero_id;
			if (load_hero_rank_detail_xml_info(cur, info.rank_map) == -1) {
				ERROR_LOG("load hero rank detail xml info err");
				return -1;
			}

			TRACE_LOG("load hero rank growth xml info\t[hero_id=%u]", hero_id);
			
			hero_map.insert(HeroRankXmlMap::value_type(hero_id, info));
		}
		cur = cur->next;
	}

	return 0;
}

int
HeroRankXmlManager::load_hero_rank_detail_xml_info(xmlNodePtr cur, HeroRankDetailXmlMap &rank_map)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("growth"))) {
			uint32_t rank = 0;
			get_xml_prop(rank, cur, "rank");
			HeroRankDetailXmlMap::iterator it = rank_map.find(rank);
			if (it != rank_map.end()) {
				ERROR_LOG("load hero rank detail xml info err, id exists, rank=%u", rank);
				return -1;
			}

			hero_rank_detail_xml_info_t info = {};
			info.rank = rank;
			get_xml_prop(info.attr.max_hp, cur, "maxhp");
			get_xml_prop(info.attr.ad, cur, "ad");
			get_xml_prop(info.attr.armor, cur, "armor");
			get_xml_prop(info.attr.resist, cur, "resist");
			get_xml_prop_def(info.talent_id, cur, "talent_id", 0);

			TRACE_LOG("load hero rank detail xml info\t[%u %f %f %f %f %u]", 
					info.rank, info.attr.max_hp, info.attr.ad, info.attr.armor, info.attr.resist, info.talent_id);

			rank_map.insert(HeroRankDetailXmlMap::value_type(rank, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							HeroRankStuffXmlManager Class						*/
/********************************************************************************/
HeroRankStuffXmlManager::HeroRankStuffXmlManager()
{

}

HeroRankStuffXmlManager::~HeroRankStuffXmlManager()
{

}

const hero_rank_stuff_detail_xml_info_t*
HeroRankStuffXmlManager::get_hero_rank_stuff_xml_info(uint32_t hero_id, uint32_t rank)
{
	HeroRankStuffXmlMap::iterator it = stuff_map.find(hero_id);
	if (it == stuff_map.end()) {
		return 0;
	}

	HeroRankStuffDetailXmlMap::iterator it2 = it->second.rank_map.find(rank);
	if (it2 == it->second.rank_map.end()) {
		return 0;
	}

	return &(it2->second);
}

int
HeroRankStuffXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_hero_rank_stuff_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
HeroRankStuffXmlManager::load_hero_rank_stuff_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("stuff"))) {
			uint32_t army_id = 0;
			get_xml_prop(army_id, cur, "army_id");

			hero_rank_stuff_detail_xml_info_t info = {};
			get_xml_prop(info.rank, cur, "rank");
			get_xml_prop_def(info.hero_card_cnt, cur, "hero_card_cnt", 0);
			get_xml_prop_def(info.golds, cur, "golds", 0);
			get_xml_prop(info.stuff[0].stuff_id, cur, "stuff_1");
			get_xml_prop(info.stuff[0].num, cur, "num_1");
			get_xml_prop(info.stuff[1].stuff_id, cur, "stuff_2");
			get_xml_prop(info.stuff[1].num, cur, "num_2");

			HeroRankStuffXmlMap::iterator it = stuff_map.find(army_id);
			if (it != stuff_map.end()) {
				HeroRankStuffDetailXmlMap::iterator it2 = it->second.rank_map.find(info.rank);
				if (it2 != it->second.rank_map.end()) {
					ERROR_LOG("load hero rank stuff xml info err, rank exists, army_id=%u, rank=%u", army_id, info.rank);
					return -1;
				}
				it->second.rank_map.insert(HeroRankStuffDetailXmlMap::value_type(info.rank, info));
			} else {
				hero_rank_stuff_xml_info_t hero_info;
				hero_info.army_id = army_id;
				hero_info.rank_map.insert(HeroRankStuffDetailXmlMap::value_type(info.rank, info));

				stuff_map.insert(HeroRankStuffXmlMap::value_type(army_id, hero_info));
			}
			KTRACE_LOG(0, "load hero rank stuff detail xml info\t[%u %u %u %u %u %u %u %u]", army_id, info.rank, info.hero_card_cnt, info.golds,
					info.stuff[0].stuff_id, info.stuff[0].num, info.stuff[1].stuff_id, info.stuff[1].num);		
		}
		cur = cur->next;
	}

	return 0;
}


/********************************************************************************/
/*							HeroLevelAttrXmlManager Class							*/
/********************************************************************************/
HeroLevelAttrXmlManager::HeroLevelAttrXmlManager()
{

}

HeroLevelAttrXmlManager::~HeroLevelAttrXmlManager()
{

}

const hero_level_attr_xml_info_t *
HeroLevelAttrXmlManager::get_hero_level_attr_xml_info(uint32_t hero_id)
{
	HeroLevelAttrXmlMap::const_iterator it = hero_map.find(hero_id);
	if (it != hero_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
HeroLevelAttrXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_hero_level_attr_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
HeroLevelAttrXmlManager::load_hero_level_attr_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("hero"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			HeroLevelAttrXmlMap::iterator it = hero_map.find(id);
			if (it != hero_map.end()) {
				ERROR_LOG("load hero attr level xml info err, id exists, id=%u", id);
				return -1;
			}

			hero_level_attr_xml_info_t info = {};
			info.hero_id = id;
			get_xml_prop(info.maxhp, cur, "maxhp");
			get_xml_prop(info.ad, cur, "ad");
			get_xml_prop(info.armor, cur, "armor");
			get_xml_prop(info.resist, cur, "resist");
			get_xml_prop(info.hp_regain, cur, "hp_regain");

			TRACE_LOG("load hero level attr xml info\t[%u %f %f %f %f %f]", info.hero_id, info.maxhp, info.ad, info.armor, info.resist, info.hp_regain);

			hero_map.insert(HeroLevelAttrXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							LevelXmlManager Class							*/
/********************************************************************************/
LevelXmlManager::LevelXmlManager()
{

}

LevelXmlManager::~LevelXmlManager()
{

}

const level_xml_info_t *
LevelXmlManager::get_level_xml_info(uint32_t level)
{
	LevelXmlMap::iterator it = level_map.find(level);
	if (it != level_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
LevelXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_level_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
LevelXmlManager::load_level_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("level"))) {
			uint32_t level = 0;
			get_xml_prop(level, cur, "level");
			LevelXmlMap::iterator it = level_map.find(level);
			if (it != level_map.end()) {
				ERROR_LOG("load hero level xml info err, level exists, level=%u", level);
				return -1;
			}

			level_xml_info_t info;
			info.level = level;
			get_xml_prop(info.master_exp, cur, "master_exp");
			get_xml_prop(info.hero_exp, cur, "hero_exp");
			get_xml_prop(info.soldier_exp, cur, "soldier_exp");
			get_xml_prop(info.levelup_endurance, cur, "levelup_endurance");
			get_xml_prop(info.max_lv, cur, "max_lv");
			get_xml_prop(info.adventure, cur, "adventure");
			get_xml_prop(info.equip_max_lv, cur, "equip_maxlv");
			get_xml_prop(info.btl_power_scale, cur, "btl_power_scale");
			get_xml_prop_def(info.item_id, cur, "item_id", 0);
			get_xml_prop_def(info.item_cnt, cur, "item_cnt", 0);
			get_xml_prop_def(info.unlock_soldier, cur, "unlock_soldier", 0);

			TRACE_LOG("load hero level xml info\t[%u %u %u %u %u %u %u %u %f %u %u %u]", 
					info.level, info.master_exp, info.hero_exp, info.soldier_exp, info.levelup_endurance, 
					info.max_lv, info.adventure, info.equip_max_lv, info.btl_power_scale, info.item_id, info.item_cnt, info.unlock_soldier);

			level_map.insert(LevelXmlMap::value_type(level, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							HeroHonorXmlManager Class							*/
/********************************************************************************/
HeroHonorXmlManager::HeroHonorXmlManager()
{

}

HeroHonorXmlManager::~HeroHonorXmlManager()
{

}

const hero_honor_level_xml_info_t *
HeroHonorXmlManager::get_hero_honor_xml_info(uint32_t army, uint32_t lv)
{
	HeroHonorXmlMap::iterator it = honor_map.find(army);
	if (it == honor_map.end()) {
		return 0;
	}
	HeroHonorLevelXmlMap::iterator it2 = it->second.level_map.find(lv);
	if (it2 == it->second.level_map.end()) {
		return 0;
	}

	return &(it2->second);
}

int
HeroHonorXmlManager::read_from_xml(const char *filename) 
{
	XML_PARSE_FILE(filename);
	int ret = load_hero_honor_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
HeroHonorXmlManager::load_hero_honor_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("honor"))) {
			uint32_t army = 0;
			get_xml_prop(army, cur, "army");
			hero_honor_level_xml_info_t sub_info;
			get_xml_prop(sub_info.lv, cur, "lv");
			get_xml_prop_def(sub_info.max_hp, cur, "total_maxhp", 0.0);
			get_xml_prop_def(sub_info.ad, cur, "total_ad", 0.0);
			get_xml_prop_def(sub_info.armor, cur, "total_armor", 0.0);
			get_xml_prop_def(sub_info.resist, cur, "total_resist", 0.0);

			HeroHonorXmlMap::iterator it = honor_map.find(army);
			if (it != honor_map.end()) {
				HeroHonorLevelXmlMap::iterator it2 = it->second.level_map.find(sub_info.lv);
				if (it2 != it->second.level_map.end()) {
					ERROR_LOG("load hero honor xml info err, lv exist, army=%u, lv=%u", army, sub_info.lv);
					return -1;
				}
				it->second.level_map.insert(HeroHonorLevelXmlMap::value_type(sub_info.lv, sub_info));
			} else {
				hero_honor_xml_info_t info;
				info.army = army;
				info.level_map.insert(HeroHonorLevelXmlMap::value_type(sub_info.lv, sub_info));
				honor_map.insert(HeroHonorXmlMap::value_type(army, info));
			}
			TRACE_LOG("load hero honor xml info\t[%u %u %f %f %f %f]", army, sub_info.lv, sub_info.max_hp, sub_info.ad, sub_info.armor, sub_info.resist);
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							HeroHonorExpXmlManager Class						*/
/********************************************************************************/
HeroHonorExpXmlManager::HeroHonorExpXmlManager()
{

}

HeroHonorExpXmlManager::~HeroHonorExpXmlManager()
{

}

const hero_honor_exp_xml_info_t*
HeroHonorExpXmlManager::get_hero_honor_exp_xml_info(uint32_t lv)
{
	HeroHonorExpXmlMap::iterator it = honor_exp_map.find(lv);
	if (it != honor_exp_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
HeroHonorExpXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_hero_honor_exp_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
HeroHonorExpXmlManager::load_hero_honor_exp_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("honor"))) {
			uint32_t lv = 0;
			get_xml_prop(lv, cur, "lv");
			HeroHonorExpXmlMap::iterator it = honor_exp_map.find(lv);
			if (it != honor_exp_map.end()) {
				ERROR_LOG("load hero honor exp xml info err, lv exist, lv=%u", lv);
				return -1;
			}
			hero_honor_exp_xml_info_t info;
			info.lv = lv;
			get_xml_prop(info.exp, cur, "exp");
			get_xml_prop(info.cri_prob, cur, "cri_prob");
			get_xml_prop(info.btl_power, cur, "add_btl_power");

			TRACE_LOG("load hero honor exp xml info\t[%u %u %f %u]", info.lv, info.exp, info.cri_prob, info.btl_power);

			honor_exp_map.insert(HeroHonorExpXmlMap::value_type(lv, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							HeroTitleXmlManager Class							*/
/********************************************************************************/
HeroTitleXmlManager::HeroTitleXmlManager()
{
	hero_title_xml_map.clear();
}

HeroTitleXmlManager::~HeroTitleXmlManager()
{

}

uint32_t 
HeroTitleXmlManager::random_one_title(uint32_t title_lv)
{
	HeroTitleXmlMap::iterator it = hero_title_xml_map.find(title_lv);
	if (it == hero_title_xml_map.end()) {
		return 0;
	}

	uint32_t sz = it->second.titles.size();
	if (!sz) {
		return 0;
	}

	uint32_t r = rand() % sz;

	return it->second.titles[r];
}

int
HeroTitleXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_hero_title_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
HeroTitleXmlManager::load_hero_title_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("title"))) {
			uint32_t title = 0;
			get_xml_prop(title, cur, "title_id");
			uint32_t title_lv = title / 10;
			HeroTitleXmlMap::iterator it = hero_title_xml_map.find(title_lv);
			if (it != hero_title_xml_map.end()) {
				it->second.titles.push_back(title);
			} else {
				hero_title_xml_info_t info;
				info.title_lv = title_lv;
				info.titles.push_back(title);
				hero_title_xml_map.insert(HeroTitleXmlMap::value_type(title_lv, info));
			}

			TRACE_LOG("load hero title xml info\t[%u]", title);
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								client request									*/
/********************************************************************************/
/* @brief 拉取英雄信息
 */
int cli_get_heros_info(Player *p, Cmessage *c_in)
{
	return send_msg_to_dbroute(p, db_get_player_heros_info_cmd, 0, p->user_id);
}

/* @brief 英雄穿戴装备
 */
int cli_hero_put_on_equipment(Player *p, Cmessage *c_in)
{
	cli_hero_put_on_equipment_in *p_in = P_IN;
	
	Hero *hero = p->hero_mgr->get_hero(p_in->hero_id);
   	if (!hero) {
		T_KWARN_LOG(p->user_id, "hero not exist\t[hero_id=%u]", p_in->hero_id);
		return p->send_to_self_error(p->wait_cmd, cli_hero_not_exist_err, 1);
	}

	cli_hero_put_on_equipment_out cli_out;
	cli_out.hero_id = hero->id;
	int ret = hero->put_on_equipment_list(p_in->equips, cli_out);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	KDEBUG_LOG(p->user_id, "HERO PUT ON EQUIPMENT LIST\t[hero_id=%u]", p_in->hero_id);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 英雄脱下装备
 */
int cli_hero_put_off_equipment(Player *p, Cmessage *c_in)
{
	cli_hero_put_off_equipment_in *p_in = P_IN;
	Hero *hero = p->hero_mgr->get_hero(p_in->hero_id);
   	if (!hero) {
		T_KWARN_LOG(p->user_id, "hero not exist\t[hero_id=%u]", p_in->hero_id);
		return p->send_to_self_error(p->wait_cmd, cli_hero_not_exist_err, 1);
	}

	int ret = hero->put_off_equipment(p_in->get_tm);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "HERO PUT OFF EQUIPMENT\t[hero_id=%u get_tm=%u]", p_in->hero_id, p_in->get_tm);

	cli_hero_put_off_equipment_out cli_out;
	cli_out.hero_id = hero->id;
	cli_out.get_tm = p_in->get_tm;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 英雄升阶
 */
int cli_hero_rising_rank(Player *p, Cmessage *c_in)
{
	cli_hero_rising_rank_in *p_in = P_IN;
	Hero *hero = p->hero_mgr->get_hero(p_in->hero_id);
   	if (!hero) {
		T_KWARN_LOG(p->user_id, "hero not exist\t[hero_id=%u]", p_in->hero_id);
		return p->send_to_self_error(p->wait_cmd, cli_hero_not_exist_err, 1);
	}

	int ret = hero->rising_rank();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_hero_rising_rank_out cli_out;
	cli_out.hero_id = hero->id;
	cli_out.rank = hero->rank;

	T_KDEBUG_LOG(p->user_id, "HERO RISING RANK\t[hero_id=%u, rank=%u]", hero->id, hero->rank);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 英雄技能升级
 */
int cli_hero_skill_level_up(Player *p, Cmessage *c_in)
{
	cli_hero_skill_level_up_in *p_in = P_IN;
	Hero *hero = p->hero_mgr->get_hero(p_in->hero_id);
   	if (!hero) {
		T_KWARN_LOG(p->user_id, "hero not exist\t[hero_id=%u]", p_in->hero_id);
		return p->send_to_self_error(p->wait_cmd, cli_hero_not_exist_err, 1);
	}

	int ret = hero->skill_level_up(p_in->skill_type, p_in->cost_point);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_hero_skill_level_up_out cli_out;
	cli_out.hero_id = hero->id;
	cli_out.skill_type = p_in->skill_type;
	cli_out.cost_point = p_in->cost_point;
	if (p_in->skill_type == 1) {
		cli_out.skill_lv = hero->skill_lv;
	} else if (p_in->skill_type == 2) {
		cli_out.skill_lv = hero->unique_skill_lv;
	}
	if (hero->id == p->role_id) {
		const role_skill_xml_info_t *p_xml_info = role_skill_xml_mgr->get_role_skill_xml_info_by_skill(p_in->skill_type);
		if (p_xml_info && p_xml_info->id && p_xml_info->id < 12) {
			uint32_t res_type = forever_main_hero_skill_1_lv + p_xml_info->id - 1;
			uint32_t res_value = p->res_mgr->get_res_value(res_type);
			cli_out.skill_lv = res_value;
		}
	}
	cli_out.skill_point = p->skill_point;

	T_KDEBUG_LOG(p->user_id, "HERO SKILL LEVEL UP!\t[hero_id=%u, type=%u, add_lv=%u, skill_lv=%u, skill_point=%u]", 
			p_in->hero_id, p_in->skill_type, p_in->cost_point, cli_out.skill_lv, p->skill_point);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 英雄使用道具
 */
int cli_use_items_for_hero(Player *p, Cmessage *c_in)
{
	cli_use_items_for_hero_in *p_in = P_IN;
	Hero *hero = p->hero_mgr->get_hero(p_in->hero_id);
	if (!hero) {
		T_KWARN_LOG(p->user_id, "hero not exist\t[hero_id=%u]", p_in->hero_id);
		return p->send_to_self_error(p->wait_cmd, cli_hero_not_exist_err, 1);
	}

	const item_xml_info_t *p_info = items_xml_mgr->get_item_xml_info(p_in->item_id);
	if (!p_info) {
		T_KWARN_LOG(p->user_id, "invalid item id\t[item_id=%u]", p_in->item_id);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_item_err, 1);
	}

	int ret = 0;
	if (p_info->type == em_item_type_for_hero_exp) {//经验道具
		ret = hero->eat_exp_items(p_in->item_id, p_in->item_cnt);
	} else if (p_info->type == em_item_type_for_hero_honor) {//战功道具
		ret = hero->eat_honor_items(p_in->item_id, p_in->item_cnt);
	}
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_use_items_for_hero_out cli_out;
	cli_out.hero_id = hero->id;
	cli_out.item_id = p_in->item_id;
	cli_out.item_cnt = p_in->item_cnt;

	T_KDEBUG_LOG(p->user_id, "HERO USE ITEMS\t[hero_id=%u item_id=%u item_cnt=%u]", p_in->hero_id, p_in->item_id, p_in->item_cnt);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 英雄装备战魂
 */
int cli_hero_put_on_btl_soul(Player *p, Cmessage *c_in)
{
	cli_hero_put_on_btl_soul_in *p_in = P_IN;
	Hero *p_hero = p->hero_mgr->get_hero(p_in->hero_id);
	if (!p_hero) {
		T_KWARN_LOG(p->user_id, "hero not exist\t[hero_id=%u]", p_in->hero_id);
		return p->send_to_self_error(p->wait_cmd, cli_hero_not_exist_err, 1);
	}

	BtlSoul *btl_soul = p->btl_soul_mgr->get_btl_soul(p_in->get_tm);
	if (!btl_soul) {
		T_KWARN_LOG(p->user_id, "btl soul not exist\t[get_tm=%u]", p_in->get_tm);
		return p->send_to_self_error(p->wait_cmd, cli_btl_soul_not_exist_err, 1);
	}

	uint32_t off_get_tm = 0;
	int ret = p_hero->put_on_btl_soul(p_in->get_tm, off_get_tm);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_hero_put_on_btl_soul_out cli_out;
	cli_out.hero_id = p_in->hero_id;
	cli_out.get_tm = p_in->get_tm;
	cli_out.off_get_tm = off_get_tm;

	T_KDEBUG_LOG(p->user_id, "HERO PUT ON BTL SOUL\t[hero_id=%u, get_tm=%u, off_get_tm=%u]", p_in->hero_id, p_in->get_tm, off_get_tm);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 英雄卸下战魂
 */
int cli_hero_put_off_btl_soul(Player *p, Cmessage *c_in)
{
	cli_hero_put_off_btl_soul_in *p_in = P_IN;
	Hero *p_hero = p->hero_mgr->get_hero(p_in->hero_id);
	if (!p_hero) {
		T_KWARN_LOG(p->user_id, "hero not exist\t[hero_id=%u]", p_in->hero_id);
		return p->send_to_self_error(p->wait_cmd, cli_hero_not_exist_err, 1);
	}

	BtlSoul *btl_soul = p->btl_soul_mgr->get_btl_soul(p_in->get_tm);
	if (!btl_soul) {
		T_KWARN_LOG(p->user_id, "btl soul not exist\t[get_id=%u]", p_in->get_tm);
		return p->send_to_self_error(p->wait_cmd, cli_btl_soul_not_exist_err, 1);
	}

	int ret = p_hero->put_off_btl_soul(p_in->get_tm);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_hero_put_off_btl_soul_out cli_out;
	cli_out.hero_id = p_in->hero_id;
	cli_out.get_tm = p_in->get_tm;

	T_KDEBUG_LOG(p->user_id, "HERO PUT OFF BTL SOUL\t[hero_id=%u, get_tm=%u]", p_in->hero_id, p_in->get_tm);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);

}

/* @brief 设置英雄状态
 */
int cli_set_hero_status(Player *p, Cmessage *c_in)
{
	cli_set_hero_status_in *p_in = P_IN;
	Hero *p_hero = p->hero_mgr->get_hero(p_in->hero_id);
	if (!p_hero) {
		T_KWARN_LOG(p->user_id, "hero not exist\t[hero_id=%u]", p_in->hero_id);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_hero_err, 1);
	}

	p_hero->set_hero_status(p_in->status);

	T_KDEBUG_LOG(p->user_id, "SET HERO STATUS\t[hero_id=%u, status=%u]", p_in->hero_id, p_in->status);

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 分封英雄官衔
 */
int cli_grant_hero_title(Player *p, Cmessage *c_in)
{
	cli_grant_hero_title_in *p_in = P_IN;
	Hero *p_hero = p->hero_mgr->get_hero(p_in->hero_id);
	if (!p_hero) {
		T_KWARN_LOG(p->user_id, "hero not exist\t[hero_id=%u]", p_in->hero_id);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_hero_err, 1);
	}

	int ret = p_hero->grant_hero_title(p_in->title);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_grant_hero_title_out cli_out;
	cli_out.hero_id = p_in->hero_id;
	cli_out.grant_flag = p_hero->get_hero_grant_state();
	cli_out.title = p_in->title;

	T_KDEBUG_LOG(p->user_id, "GRANT HERO TITLE\t[hero_id=%u, title=%u]", p_in->hero_id, p_hero->title);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 设置羁绊英雄
 */
int cli_set_trip_hero(Player *p, Cmessage *c_in)
{
	cli_set_trip_hero_in *p_in = P_IN;

	int ret = p->hero_mgr->set_trip_hero(p_in->hero_id, p_in->trip_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "SET TRIP HERO\t[hero_id=%u, trip_id=%u]", p_in->hero_id, p_in->trip_id);

	cli_set_trip_hero_out cli_out;
	cli_out.hero_id = p_in->hero_id;
	cli_out.trip_id = p_in->trip_id;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);

}

/********************************************************************************/
/*									dbsvr return 								*/
/********************************************************************************/
/* @brief 拉取英雄信息返回
 */
int db_get_player_heros_info(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);

	db_get_player_heros_info_out *p_in = P_IN;

	p->hero_mgr->init_all_heros_info(p_in);
	p->equip_mgr->init_heros_equip_info();
	p->btl_soul_mgr->init_hero_btl_soul_list();

	if (p->wait_cmd == cli_proto_login_cmd) {
		p->login_step++;

		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u HEROS INFO", p->login_step);

		//英雄信息
		cli_get_heros_info_out cli_out;
		p->hero_mgr->pack_all_heros_info(cli_out);
		p->send_to_self(cli_get_heros_info_cmd, &cli_out, 0); 

		//拉取小兵信息
		return send_msg_to_dbroute(p, db_get_player_soldiers_info_cmd, 0, p->user_id);
	} else if (p->wait_cmd == cli_get_heros_info_cmd) {
		cli_get_heros_info_out cli_out;
		p->hero_mgr->pack_all_heros_info(cli_out);

		return p->send_to_self(p->wait_cmd, &cli_out, 1);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}

