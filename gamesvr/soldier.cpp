/*
 * =====================================================================================
 *
 *  @file  soldier.cpp 
 *
 *  @brief  小兵系统
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
#include "soldier.hpp"
#include "player.hpp"
#include "dbroute.hpp"
#include "talent.hpp"
#include "arena.hpp"
*/

#include "stdafx.hpp"
using namespace project;
using namespace std;

//SoldierXmlManager soldier_xml_mgr;
//SoldierRankXmlManager soldier_rank_xml_mgr;
//SoldierStarXmlManager soldier_star_xml_mgr;
//SoldierTrainCostXmlManager soldier_train_cost_xml_mgr;
//SoldierLevelAttrXmlManager soldier_level_attr_xml_mgr;

/********************************************************************************/
/*							Soldier Class										*/
/********************************************************************************/
Soldier::Soldier(Player *p, uint32_t soldier_id) : id(soldier_id), owner(p)
{
	get_tm = 0;
	lv = 0;
	exp = 0;
	rank = 0;
	rank_exp = 0;
	star = 0;
	btl_power = 0;
	state = 0;

	hp = 0;
	max_hp = 0;
	ad = 0;
	armor = 0;
	resist = 0;

	memset(&train_lv, 0x0, sizeof(train_lv));

	base_info = soldier_xml_mgr->get_soldier_xml_info(soldier_id);
}

Soldier::~Soldier()
{

}

int
Soldier::get_level_up_exp()
{
	const level_xml_info_t * p_info = level_xml_mgr->get_level_xml_info(lv);
	if (!p_info) {
		return -1;
	}

	return p_info->soldier_exp;
}

int
Soldier::calc_soldier_max_lv()
{
	return owner->lv;
}

int
Soldier::add_exp(uint32_t add_value)
{
	if (!add_value) {
		return 0;
	}
	
	uint32_t max_lv = calc_soldier_max_lv();
	if (!max_lv || lv > max_lv) {
		return 0;
	}

	uint32_t old_lv = lv;
	exp += add_value;

	if (lv == max_lv) {
		uint32_t level_up_exp = get_level_up_exp();
		if (exp >= level_up_exp) {
			exp = level_up_exp;
		}
	} else {
		uint32_t level_up_exp = get_level_up_exp();
		while (exp >= level_up_exp) {//升级
			exp -= level_up_exp;
			lv++;
			level_up_exp = get_level_up_exp();
			if (lv >= max_lv) {
				if (exp >= level_up_exp) {
					exp = level_up_exp;
				}
				break;
			}
		}
	}

	if (lv > old_lv) {//升级
		calc_all();
		send_soldier_attr_change_noti();
	}

	//更新DB
	db_update_soldier_exp_info_in db_in;
	db_in.soldier_id = this->id;
	db_in.lv = this->lv;
	db_in.exp = this->exp;
	send_msg_to_dbroute(0, db_update_soldier_exp_info_cmd, &db_in, owner->user_id);

	//通知前端
	cli_soldier_exp_change_noti_out noti_out;
	noti_out.soldier_id = this->id;
	noti_out.old_lv = old_lv;
	noti_out.lv = lv;
	noti_out.add_exp = add_value;
	noti_out.exp = exp;
	noti_out.levelup_exp = get_level_up_exp();
	noti_out.max_lv = calc_soldier_max_lv();
	owner->send_to_self(cli_soldier_exp_change_noti_cmd, &noti_out, 0);

	T_KDEBUG_LOG(owner->user_id, "ADD SOLDIER EXP\t[add_exp=%u, lv=%u, exp=%u]", add_value, lv, exp);

	return 0;
}

int
Soldier::eat_exp_items(uint32_t item_id, uint32_t item_cnt)
{
	const item_xml_info_t *p_item_info = items_xml_mgr->get_item_xml_info(item_id);
	if (!p_item_info || p_item_info->type != em_item_type_for_soldier_exp) {
		T_KWARN_LOG(owner->user_id, "invalid hero exp item\t[item_id=%u]", item_id);
		return cli_invalid_item_err;
	}    

	uint32_t cur_cnt = owner->items_mgr->get_item_cnt(item_id);
	if (cur_cnt < item_cnt) {
		T_KWARN_LOG(owner->user_id, "eat soldier exp items not enough\t[cur_cnt=%u, eat_cnt=%u]", cur_cnt, item_cnt);
		return cli_not_enough_item_err;
	}    

	//扣除物品
	owner->items_mgr->del_item_without_callback(item_id, item_cnt);
	
	//增加经验
	uint32_t total_exp = p_item_info->effect * item_cnt;
	add_exp(total_exp);
	
	return 0;
}

int
Soldier::get_rank_up_exp()
{
	uint32_t query_rank = rank + 1;
	if (rank >= MAX_SOLDIER_RANK) {
		query_rank = MAX_SOLDIER_RANK;
	}
	const soldier_rank_detail_xml_info_t *p_info = soldier_rank_xml_mgr->get_soldier_rank_xml_info(this->id, query_rank);
	if (!p_info) {
		return -1;
	}
	
	return p_info->exp;
}

int
Soldier::add_rank_exp(uint32_t add_value)
{
	if (!add_value) {
		return 0;
	}
	if (rank >= MAX_SOLDIER_RANK) {
		return 0;
	}

	uint32_t old_rank = rank;

	rank_exp += add_value;
	uint32_t rank_up_exp = this->get_rank_up_exp();
	while (rank_exp >= rank_up_exp) {
		rank++;
		rank_exp -= rank_up_exp;
		rank_up_exp = this->get_rank_up_exp();
	}

	//更新DB
	db_update_soldier_rank_info_in db_in;
	db_in.soldier_id = this->id;
	db_in.rank = this->rank;
	db_in.rank_exp = this->rank_exp;
	send_msg_to_dbroute(0, db_update_soldier_rank_info_cmd, &db_in, owner->user_id);

	if (rank > old_rank) {
		calc_all();
		
		//检查任务
		for (uint32_t r = old_rank; r <= rank; r++) {
			uint32_t rank_cnt = owner->soldier_mgr->calc_over_cur_rank_soldier_cnt(r);
			owner->task_mgr->check_task(em_task_type_soldier_rank, r, rank_cnt);
			owner->achievement_mgr->check_achievement(em_achievement_type_soldier_rank, r, rank_cnt);
		}
	}

	return 0;
}

/* @brief 强化小兵
 */
int
Soldier::strength_soldier(vector<cli_item_info_t> &cards)
{
	if (rank >= MAX_SOLDIER_RANK) {
		return cli_soldier_reach_max_rank_err;
	}
	uint32_t add_exp = 0;
	for (uint32_t i = 0; i < cards.size(); i++) {
		uint32_t item_id = cards[i].item_id;
		uint32_t item_cnt = cards[i].item_cnt;
		const item_xml_info_t* p_info = items_xml_mgr->get_item_xml_info(item_id);
		if (!p_info || p_info->type != em_item_type_for_hero_card) {
			T_KWARN_LOG(owner->user_id, "invalid soldier strength item type\t[item_id=%u]", item_id);
			return cli_invalid_soldier_strength_item_err;
		}
		uint32_t cur_cnt = owner->items_mgr->get_item_cnt(item_id);
		if (cur_cnt < item_cnt) {
			T_KWARN_LOG(owner->user_id, "strength soldier need item not enough\t[cur_cnt=%u, need_cnt=%u]", cur_cnt, item_cnt);
			return cli_not_enough_item_err;
		}
		add_exp += p_info->effect * item_cnt;
	}

	//检查金币是否足够
	if (add_exp) {
		uint32_t need_golds = add_exp;
		if (owner->golds < need_golds) {
			T_KWARN_LOG(owner->user_id, "soldier strength soldier need golds not enough\t[golds=%u, need_golds=%u]", owner->golds, need_golds);
			return cli_not_enough_golds_err;
		}

		//扣除金币
		owner->chg_golds(-need_golds);
	}

	//扣除物品
	for (uint32_t i = 0; i < cards.size(); i++) {
		uint32_t item_id = cards[i].item_id;
		uint32_t item_cnt = cards[i].item_cnt;
		owner->items_mgr->del_item_without_callback(item_id, item_cnt);
	}

	//增加经验
	this->add_rank_exp(add_exp);

	return 0;
}

/* @brief 小兵升星
 */
int
Soldier::rising_star()
{
	if (star > MAX_SOLDIER_STAR) {
		return cli_soldier_reach_max_star_err;
	}


	const soldier_star_detail_xml_info_t *p_info = soldier_star_xml_mgr->get_soldier_star_xml_info(this->id, this->star + 1);
	if (!p_info) {
		return cli_soldier_cannot_rising_star_err;
	}
	//检查金币是否足够
	if (owner->golds < p_info->golds) {
		T_KWARN_LOG(owner->user_id, "soldier rising star need golds not enough\t[golds=%u need_golds=%u]", owner->golds, p_info->golds);
		return cli_not_enough_golds_err;
	}

	//碎片是否足够
	uint32_t soul_cnt = owner->items_mgr->get_item_cnt(p_info->soul_id);
	if (soul_cnt < p_info->soul_cnt) {
		T_KWARN_LOG(owner->user_id, "rising soldier star need item not enough\t[cur_cnt=%u, need_cnt=%u]", soul_cnt, p_info->soul_cnt);
		return cli_not_enough_item_err;
	}	

	//扣除金币
	if (p_info->golds) {
		owner->chg_golds(0 - p_info->golds);
	}

	//扣除物品
	owner->items_mgr->del_item_without_callback(p_info->soul_id, p_info->soul_cnt);

	if (p_info->evolution_id) {//进化
		uint32_t old_soldier_id = this->id;
		this->id = p_info->evolution_id;
		this->star = 1;
		base_info = soldier_xml_mgr->get_soldier_xml_info(this->id);

		//更新map
		owner->soldier_mgr->soldier_evolution(old_soldier_id, this->id);

		//更新DB
		db_soldier_evolution_in db_in;
		db_in.soldier_id = old_soldier_id;
		db_in.evolution_soldier_id = this->id;
		send_msg_to_dbroute(0, db_soldier_evolution_cmd, &db_in, owner->user_id);
	} else {
		//升星
		star++;

		//更新DB
		db_soldier_rising_star_in db_in;
		db_in.soldier_id = this->id;
		send_msg_to_dbroute(0, db_soldier_rising_star_cmd, &db_in, owner->user_id);

		//检查任务
		uint32_t star_cnt = owner->soldier_mgr->calc_over_cur_star_soldier_cnt(star);
		owner->task_mgr->check_task(em_task_type_soldier_star, star, star_cnt);
		owner->achievement_mgr->check_achievement(em_achievement_type_soldier_star, star, star_cnt);
	}

	//重新计算属性
	calc_all();

	return 0;
}

/* @brief 小兵训练
 */
int
Soldier::soldier_training(uint32_t type, uint32_t add_lv)
{
	if (!add_lv) {
		add_lv = 1;
	}
	if (!type || type > 4) {
		return cli_soldier_training_type_err;
	}

	//是否超过小兵等级
	if (this->train_lv[type-1] + add_lv > lv) {
		return cli_training_lv_reach_max_err;
	}

	//检查训练点是否足够
	if (owner->soldier_train_point < add_lv) {
		T_KWARN_LOG(owner->user_id, "hero soldier train point not enough\t[soldier_train_point=%u, need_point=%u]", owner->soldier_train_point, add_lv);
		return cli_soldier_train_point_not_enough_err;
	}

	//判断金币是否足够 
	uint32_t need_golds = 0;
	for (uint32_t i = this->train_lv[type-1] + 1; i <= this->train_lv[type-1] + add_lv; i++) {
		const soldier_train_cost_xml_info_t *p_info = soldier_train_cost_xml_mgr->get_soldier_train_cost_xml_info(this->train_lv[type-1] + 1);
		if (!p_info) {
			return cli_soldier_cannot_train_err;
		}
		need_golds += p_info->cost[type-1];
	}

	if (owner->golds < need_golds) {
		return cli_not_enough_golds_err;
	}


	//扣除训练点
	owner->chg_soldier_train_point(-add_lv);

	//扣除金币
	owner->chg_golds(-need_golds);

	//升级
	this->train_lv[type-1] += add_lv;

	//更新DB
	db_update_soldier_training_info_in db_in;
	db_in.soldier_id = this->id;
	db_in.type = type;
	db_in.lv = this->train_lv[type-1];
	send_msg_to_dbroute(0, db_update_soldier_training_info_cmd, &db_in, owner->user_id);

	//重新计算属性
	calc_all();

	//检查任务
	owner->task_mgr->check_task(em_task_type_soldier_train);
	owner->achievement_mgr->check_achievement(em_achievement_type_soldier_train);

	return 0;
}


/* @brief 设置小兵状态
 */
int
Soldier::set_soldier_status(uint32_t status)
{
	if (this->state != status) {
		this->state = status;
	}

	if (status == 0) {//休息
		owner->instance_mgr->set_soldier(this->id, 0);
	} else {//参战
		owner->instance_mgr->set_soldier(this->id, 1);
	}

	owner->calc_btl_power();

	//设置DB
	db_set_soldier_state_in db_in;
	db_in.soldier_id = this->id;
	db_in.state = status;

	send_msg_to_dbroute(0, db_set_soldier_state_cmd, &db_in, owner->user_id);

	return 0;
}

int
Soldier::calc_soldier_base_btl_power()
{
	//训练等级战斗力
	double star_factor[] = {0, 1, 1.25, 1.5, 1.75, 2};
	double factor = star_factor[this->star];
	double base_btl_power = 0;
	for (int i = 0; i < 4; i++) {
		uint32_t lv = train_lv[i];
		const level_xml_info_t *p_info = level_xml_mgr->get_level_xml_info(lv);
		if (p_info) {
			base_btl_power += SOLDIER_TRAIN_BASE_BTL_POWER * p_info->btl_power_scale * factor;
		}
	}

	return base_btl_power;
}

int
Soldier::calc_soldier_level_btl_power()
{
	return SOLDIER_LEVEL_BASE_BTL_POWER * lv;
}

int
Soldier::calc_soldier_rank_btl_power()
{
	//兵种晋升战斗力
	uint32_t rank_power[] = {0, 1250, 2500, 3750, 5000, 6250, 7500, 8750, 10000};

	return rank_power[this->rank];
}

/* @brief 计算小兵当前战斗力
 */
int
Soldier::calc_soldier_btl_power()
{
	uint32_t old_btl_power = btl_power;

	double base_btl_power = calc_soldier_base_btl_power();
	double level_btl_power = calc_soldier_level_btl_power();
	double rank_btl_power = calc_soldier_rank_btl_power();

	btl_power = base_btl_power + level_btl_power + rank_btl_power;
	
	//通知前端
	if (old_btl_power != btl_power && owner) {
		cli_soldier_btl_power_change_noti_out noti_out;
		noti_out.soldier_id = this->id;
		noti_out.old_btl_power = old_btl_power;
		noti_out.btl_power = btl_power;

		owner->send_to_self(cli_soldier_btl_power_change_noti_cmd, &noti_out, 0);

		if (this->state == 1) {
			owner->calc_btl_power();
		}
		
		/*
		if (arena_mgr.is_defend_soldier(owner->user_id, this->id)) {
			arena_mgr.update_ranking_info_btl_power(owner);
		}*/

		T_KTRACE_LOG(owner->user_id, "soldier btl power changed\t[%u %u %u]", this->id, old_btl_power, btl_power);
	}
	
	return btl_power;
}

/* @brief 计算天赋增益后的属性
 */
void
Soldier::calc_talent_attr(soldier_attr_info_t &talent_attr1, soldier_attr_info_t &talent_attr2)
{
	for (uint32_t i = 1; i <= this->rank; i++) {
		const soldier_rank_detail_xml_info_t *p_rank_info = soldier_rank_xml_mgr->get_soldier_rank_xml_info(this->id, i);
		if (p_rank_info && p_rank_info->talent_id) {
			const soldier_talent_xml_info_t *p_xml_info = soldier_talent_xml_mgr->get_soldier_talent_xml_info(p_rank_info->talent_id);
			if (p_xml_info) {
				int flag = p_xml_info->passive_class == 1 ? 1 : -1;
				soldier_attr_info_t &attr = (p_xml_info->passive_type == 1) ? talent_attr1 : talent_attr2;
				switch (p_xml_info->type) {
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
}

void
Soldier::calc_talent_gain()
{
	soldier_attr_info_t attr1 = {};
	soldier_attr_info_t attr2 = {};

	calc_talent_attr(attr1, attr2);

	max_hp = (max_hp + attr2.max_hp) * (1 + attr1.max_hp / 100.0);
	ad = (ad + attr2.ad) * (1 + attr1.ad / 100.0);
	armor = (armor + attr2.armor) * (1 + attr1.armor / 100.0);
	resist = (resist + attr2.resist) * (1 + attr1.resist / 100.0);
	hp_regain = (hp_regain + attr2.hp_regain) * (1 + attr1.hp_regain / 100.0);
	atk_spd = (atk_spd + attr2.atk_spd) * (1 + attr1.atk_spd / 100.0);
	ad_cri = (ad_cri + attr2.ad_cri) * (1 + attr1.ad_cri / 100.0);
	ad_ren = (ad_ren + attr2.ad_ren) * (1 + attr1.ad_ren / 100.0);
	ad_chuan = (ad_chuan + attr2.ad_chuan) * (1 + attr1.ad_chuan / 100.0);
	ap_chuan = (ap_chuan + attr2.ap_chuan) * (1 + attr1.ap_chuan / 100.0);
	hit = (hit + attr2.hit) * (1 + attr1.hit / 100.0);
	miss = (miss + attr2.miss) * (1 + attr1.miss / 100.0);
	cri_damage = (cri_damage + attr2.cri_damage) * (1 + attr1.cri_damage / 100.0);
	cri_avoid = (cri_avoid + attr2.cri_avoid) * (1 + attr1.cri_avoid / 100.0);
	hp_steal = (hp_steal + attr2.hp_steal) * (1 + attr1.hp_steal / 100.0);
	ad_avoid = (ad_avoid + attr2.ad_avoid) * (1 + attr1.ad_avoid / 100.0);
	ap_avoid = (ap_avoid + attr2.ap_avoid) * (1 + attr1.ap_avoid / 100.0);
}

void 
Soldier::calc_all(bool flag)
{
	if (!base_info) {
		return;
	}
	//基础属性
	max_hp = base_info->max_hp;
	ad = base_info->ad;
	armor = base_info->armor;
	resist = base_info->resist;

	//等级属性
	const soldier_level_attr_xml_info_t *p_level_attr_xml_info = soldier_level_attr_xml_mgr->get_soldier_level_attr_xml_info(this->id);
	if (p_level_attr_xml_info) {
		max_hp += lv * p_level_attr_xml_info->max_hp;
		ad += lv * p_level_attr_xml_info->ad;
		armor += lv * p_level_attr_xml_info->armor;
		resist += lv * p_level_attr_xml_info->resist;
	}

	//品阶属性
	const soldier_rank_detail_xml_info_t *p_rank_info = soldier_rank_xml_mgr->get_soldier_rank_xml_info(this->id, this->rank);
	if (p_rank_info) {
		max_hp += p_rank_info->max_hp;
		ad += p_rank_info->ad;
		armor += p_rank_info->armor;
		resist += p_rank_info->resist;
	}

	//训练属性
	max_hp += base_info->hp_up * this->train_lv[0];
	ad += base_info->ad_up * this->train_lv[1];
	armor += base_info->armor_up * this->train_lv[2];
	resist += base_info->resist_up * this->train_lv[3];

	//战马属性
	if (owner && owner->horse) {
		max_hp += owner->horse->soldier_max_hp;
		ad += owner->horse->soldier_ad;
	}

	//计算天赋加成
	calc_talent_gain();

	//星级属性
	double star_factor[] = {0, 1, 1.25, 1.5, 1.75, 2};
	double factor = star_factor[this->star];
	max_hp *= factor;
	ad *= factor;
	armor *= factor;
	resist *= factor;

	if (!flag) {
		calc_soldier_btl_power();
	}
}

int
Soldier::init_soldier_base_info()
{
	if (!base_info) {
		return -1;
	}
	get_tm = get_now_tv()->tv_sec;
	lv = 1;
	exp = 0;
	rank = 0;
	rank_exp = 0;
	star = 1;
	btl_power = 0;
	state = 0;

	hp = base_info->max_hp;
	max_hp = base_info->max_hp;
	ad = base_info->ad;
	armor = base_info->armor;
	resist = base_info->resist;

	memset(train_lv, 0, sizeof(train_lv));

	return 0;
}

int
Soldier::init_soldier_info(const db_soldier_info_t *p_info)
{
	get_tm = p_info->get_tm;
	lv = p_info->lv;
	exp = p_info->exp;
	rank = p_info->rank;
	rank_exp = p_info->rank_exp;
	star = p_info->star;
	state = p_info->state;
	for (int i = 0; i < 4; i++) {
		train_lv[i] = p_info->train_lv[i];
	}

	return 0;
}

int
Soldier::send_soldier_attr_change_noti()
{
	cli_soldier_attr_change_noti_out noti_out;
	noti_out.soldier_id = id;
	noti_out.max_hp = max_hp;
	noti_out.ad = ad;
	noti_out.armor = armor;
	noti_out.resist = resist;

	return owner->send_to_self(cli_soldier_attr_change_noti_cmd, &noti_out, 0);
}


void
Soldier::pack_soldier_db_info(db_soldier_info_t &info)
{
	info.soldier_id = id;
	info.get_tm = get_tm;
	info.lv = lv;
	info.exp = exp;
	info.rank = rank;
	info.rank_exp = rank_exp;
	info.star = star;
	info.state = state;
	for (int i = 0; i < 4; i++) {
		info.train_lv[i] = train_lv[i];
	}
}

void 
Soldier::pack_soldier_client_info(cli_soldier_info_t &info)
{
	info.soldier_id = id;
	info.get_tm = get_tm;
	info.lv = lv;
	info.exp = exp;
	info.level_up_exp = get_level_up_exp();
	info.rank = rank;
	info.rank_exp = rank_exp;
	info.rank_up_exp = get_rank_up_exp();
	info.star = star;
	info.state = state;
	info.btl_power = btl_power;
	info.attr_info.max_hp = max_hp;
	info.attr_info.ad = ad;
	info.attr_info.armor = armor;
	info.attr_info.resist = resist;
	info.attr_info.hp_regain = hp_regain;
	info.attr_info.atk_spd = atk_spd;
	info.attr_info.ad_cri = ad_cri;
	info.attr_info.ad_ren = ad_ren;
	info.attr_info.ad_chuan = ad_chuan;
	info.attr_info.ap_chuan = ap_chuan;
	info.attr_info.hit = hit;
	info.attr_info.miss = miss;
	info.attr_info.cri_damage = cri_damage;
	info.attr_info.cri_avoid = cri_avoid;
	info.attr_info.hp_steal = hp_steal;
	info.attr_info.ad_avoid = ad_avoid;
	info.attr_info.ap_avoid = ap_avoid;
	for (int i = 0; i < 4; i++) {
		info.train_lv[i] = train_lv[i];
	}
}


/********************************************************************************/
/*							SoldierManager Class									*/
/********************************************************************************/

SoldierManager::SoldierManager(Player *p) : owner(p) 
{

}

SoldierManager::~SoldierManager()
{
	SoldierMap::iterator it = soldiers.begin();
	for (; it != soldiers.end(); it++) {
		Soldier *p_soldier = it->second;
		SAFE_DELETE(p_soldier);
	}
}

Soldier*
SoldierManager::get_soldier(uint32_t soldier_id)
{
	map<uint32_t, Soldier*>::iterator it = soldiers.find(soldier_id);
	if (it == soldiers.end()) {
		return 0;
	}

	return it->second;
}

int
SoldierManager::soldier_evolution(uint32_t old_soldier_id, uint32_t soldier_id)
{
	map<uint32_t, Soldier*>::iterator it = soldiers.find(old_soldier_id);
	if (it == soldiers.end()) {
		return 0;
	}
	Soldier *p_soldier = it->second;
	soldiers.erase(old_soldier_id);
	soldiers.insert(make_pair(soldier_id, p_soldier));

	return 0;
}

Soldier* 
SoldierManager::add_soldier(uint32_t soldier_id)
{
	const soldier_xml_info_t *p_info = soldier_xml_mgr->get_soldier_xml_info(soldier_id);
	if (!p_info) {
		KERROR_LOG(owner->user_id, "invalid soldier id, soldier_id=%u", soldier_id);
		return 0;
	}

	//是否拥有该小兵
	if (get_soldier(soldier_id)) {
		KERROR_LOG(owner->user_id, "already owner the soldier, soldier_id=%u", soldier_id);
		return 0;
	}

	Soldier *soldier = new Soldier(owner, soldier_id);
	if (!soldier) {
		return 0;
	}

	//初始化小兵信息
	soldier->init_soldier_base_info();
	soldier->calc_all();

	//插入map
	soldiers.insert(make_pair(soldier_id, soldier));

	//更新DB 
	db_add_one_soldier_in db_in;
	soldier->pack_soldier_db_info(db_in.soldier_info);
	send_msg_to_dbroute(0, db_add_one_soldier_cmd, &db_in, owner->user_id);

	//通知前端
	cli_add_soldier_noti_out noti_out;
	soldier->pack_soldier_client_info(noti_out.soldier_info);
	owner->send_to_self(cli_add_soldier_noti_cmd, &noti_out, 0);
	
	KDEBUG_LOG(owner->user_id, "ADD SOLDIER\t[soldier_id=%u]", soldier_id);
	
	return soldier;
}

int
SoldierManager::init_all_soldiers_info(db_get_player_soldiers_info_out *p_in)
{
	vector<db_soldier_info_t>::iterator it = p_in->soldiers.begin();
	for (; it != p_in->soldiers.end(); it++) {
		db_soldier_info_t *p_info = &(*it);
		const soldier_xml_info_t *soldier_info = soldier_xml_mgr->get_soldier_xml_info(p_info->soldier_id);
		if (!soldier_info) {
			KERROR_LOG(owner->user_id, "invalid soldier id, soldier_id=%u", p_info->soldier_id);
			return -1;
		}

		Soldier *soldier = new Soldier(owner, p_info->soldier_id);
		if (!soldier) {
			return -1;
		}
		soldier->init_soldier_base_info();
		soldier->init_soldier_info(p_info);
		soldier->calc_all();

		soldiers.insert(SoldierMap::value_type(soldier->id, soldier));
		T_KTRACE_LOG(owner->user_id, "init all soldiers info\t[%u %u %u %u %u %u]", 
				soldier->id, soldier->lv, soldier->exp, soldier->star, soldier->rank, soldier->state);
	}

	owner->calc_btl_power();

	return 0;
}

int
SoldierManager::calc_btl_power()
{
	uint32_t btl_power = 0;
	SoldierMap::iterator it = soldiers.begin();
	for (; it != soldiers.end(); ++it) {
		Soldier *p_soldier = it->second;
		if (p_soldier->state == 1) {
			btl_power += p_soldier->btl_power;
		}
	}

	return btl_power;
}

int
SoldierManager::calc_over_cur_star_soldier_cnt(uint32_t star)
{
	int cnt = 0;
	SoldierMap::iterator it = soldiers.begin();
	for (; it != soldiers.end(); ++it) {
		Soldier *p_soldier = it->second;
		if (p_soldier && p_soldier->star >= star) {
			cnt++;
		}
	}

	return cnt;
}

int
SoldierManager::calc_over_cur_rank_soldier_cnt(uint32_t rank)
{
	int cnt = 0;
	SoldierMap::iterator it = soldiers.begin();
	for (; it != soldiers.end(); ++it) {
		Soldier *p_soldier = it->second;
		if (p_soldier && p_soldier->rank >= rank) {
			cnt++;
		}
	}

	return cnt;
}


int
SoldierManager::pack_all_soldiers_info(cli_get_soldiers_info_out &out)
{
	SoldierMap::iterator it = soldiers.begin();
	for (; it != soldiers.end(); it++) {
		Soldier *soldier = it->second;
		cli_soldier_info_t info;
		soldier->pack_soldier_client_info(info);

		out.soldiers.push_back(info);

		T_KTRACE_LOG(owner->user_id, "PACK ALL SOLDIERS INFO\t[%u %u %u %u %u %u %u %u]", 
				info.soldier_id, info.lv, info.exp, info.rank, info.rank_exp, info.star, info.state, info.btl_power);
	}


	return 0;
}


/********************************************************************************/
/*						SoldierXmlManager Class									*/
/********************************************************************************/
SoldierXmlManager::SoldierXmlManager()
{

}

SoldierXmlManager::~SoldierXmlManager()
{

}

int 
SoldierXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_soldier_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);
	return ret; 
}

int
SoldierXmlManager::load_soldier_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("soldier"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");

			SoldierXmlMap::iterator it = soldier_xml_map.find(id);
			if (it != soldier_xml_map.end()) {
				KERROR_LOG(0, "load soldier xml info err, soldier id exists\t[id=%u]", id);
				return -1;
			}

			soldier_xml_info_t info;
			info.id = id;
			get_xml_prop(info.camp, cur, "camp");
			get_xml_prop(info.sex, cur, "sex");
			get_xml_prop(info.rank, cur, "rank");
			get_xml_prop(info.range, cur, "range");
			get_xml_prop(info.army, cur, "army");
			get_xml_prop(info.atk_type, cur, "atk_type");
			get_xml_prop(info.def_type, cur, "def_type");
			get_xml_prop(info.max_hp, cur, "maxhp");
			get_xml_prop(info.ad, cur, "ad");
			get_xml_prop(info.armor, cur, "armor");
			get_xml_prop(info.resist, cur, "resist");
			get_xml_prop(info.hp_up, cur, "hp_up");
			get_xml_prop(info.ad_up, cur, "ad_up");
			get_xml_prop(info.armor_up, cur, "armor_up");
			get_xml_prop(info.resist_up, cur, "resist_up");

			KTRACE_LOG(0, "load soldier xml info\t[%u %u %u %u %u %u %u %u %f %f %f %f %f %f %f %f]", 
					info.id, info.camp, info.sex, info.rank, 
					info.range, info.army, info.atk_type, info.def_type, 
					info.max_hp, info.ad, info.armor, info.resist, 
					info.hp_up, info.ad_up, info.armor_up, info.resist_up 
					);

			soldier_xml_map.insert(SoldierXmlMap::value_type(id, info));
		}

		cur = cur->next;
	}

	return 0;
}

const soldier_xml_info_t*
SoldierXmlManager::get_soldier_xml_info(uint32_t soldier_id)
{
	SoldierXmlMap::iterator it = soldier_xml_map.find(soldier_id);
	if (it == soldier_xml_map.end()) {
		return 0;
	}

	return &(it->second);
}

/********************************************************************************/
/*						SoldierRankXmlManager Class								*/
/********************************************************************************/
SoldierRankXmlManager::SoldierRankXmlManager()
{

}

SoldierRankXmlManager::~SoldierRankXmlManager()
{

}

const soldier_rank_detail_xml_info_t*
SoldierRankXmlManager::get_soldier_rank_xml_info(uint32_t soldier_id, uint32_t rank)
{
	SoldierRankXmlMap::const_iterator it = soldier_map.find(soldier_id);
	if (it == soldier_map.end()) {
		return 0;
	}
	SoldierRankDetailXmlMap::const_iterator it2 = it->second.rank_map.find(rank);
	if (it2 == it->second.rank_map.end()) {
		return 0;
	}

	return &(it2->second);
}

int
SoldierRankXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_soldier_rank_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
SoldierRankXmlManager::load_soldier_rank_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("soldier"))) {
			uint32_t soldier_id = 0;
			get_xml_prop(soldier_id, cur, "id");
			soldier_rank_detail_xml_info_t sub_info;
			get_xml_prop(sub_info.rank, cur, "rank");
			get_xml_prop(sub_info.max_hp, cur, "maxhp");
			get_xml_prop(sub_info.ad, cur, "ad");
			get_xml_prop(sub_info.armor, cur, "armor");
			get_xml_prop(sub_info.resist, cur, "resist");
			get_xml_prop(sub_info.exp, cur, "exp");
			get_xml_prop_def(sub_info.talent_id, cur, "talent_id", 0);

			SoldierRankXmlMap::iterator it = soldier_map.find(soldier_id);
			if (it != soldier_map.end()) {
				SoldierRankDetailXmlMap *p_map = &(it->second.rank_map);
				SoldierRankDetailXmlMap::iterator it = p_map->find(sub_info.rank);
				if (it != p_map->end()) {
					ERROR_LOG("load soldier rank xml info err, rank exists, id=%u, rank=%u", soldier_id, sub_info.rank);
					return -1;
				}
				p_map->insert(SoldierRankDetailXmlMap::value_type(sub_info.rank, sub_info));
			} else {
				soldier_rank_xml_info_t info;
				info.soldier_id = soldier_id;
				info.rank_map.insert(SoldierRankDetailXmlMap::value_type(sub_info.rank, sub_info));

				soldier_map.insert(SoldierRankXmlMap::value_type(soldier_id, info));
			}

			TRACE_LOG("load soldier rank growth xml info\t[%u %u %u %u %u %u %u %u]", 
					soldier_id, sub_info.rank, sub_info.max_hp, sub_info.ad, sub_info.armor, sub_info.resist, sub_info.exp, sub_info.talent_id);
			
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							SoldierStarXmlManager Class							*/
/********************************************************************************/
SoldierStarXmlManager::SoldierStarXmlManager()
{

}

SoldierStarXmlManager::~SoldierStarXmlManager()
{

}

const soldier_star_detail_xml_info_t *
SoldierStarXmlManager::get_soldier_star_xml_info(uint32_t soldier_id, uint32_t star)
{
	SoldierStarXmlMap::const_iterator it = soldier_map.find(soldier_id);
	if (it == soldier_map.end()) {
		return 0;
	}
	SoldierStarDetailXmlMap::const_iterator it2 = it->second.star_map.find(star);
	if (it2 == it->second.star_map.end()) {
		return 0;
	}

	return &(it2->second);
}

int
SoldierStarXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_soldier_star_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
SoldierStarXmlManager::load_soldier_star_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("soldier"))) {
			uint32_t soldier_id = 0;
			get_xml_prop(soldier_id, cur, "id");
			soldier_star_detail_xml_info_t sub_info;
			get_xml_prop(sub_info.star, cur, "star");
			get_xml_prop(sub_info.soul_id, cur, "soul_id");
			get_xml_prop(sub_info.soul_cnt, cur, "soul");
			get_xml_prop_def(sub_info.golds, cur, "golds", 0);
			get_xml_prop_def(sub_info.evolution_id, cur, "evolution_id", 0);

			SoldierStarXmlMap::iterator it = soldier_map.find(soldier_id);
			if (it != soldier_map.end()) {
				SoldierStarDetailXmlMap *p_map = &(it->second.star_map);
				SoldierStarDetailXmlMap::iterator it = p_map->find(sub_info.star);
				if (it != p_map->end()) {
					ERROR_LOG("load soldier star xml info err, star exists, id=%u, star=%u", soldier_id, sub_info.star);
					return -1;
				}
				p_map->insert(SoldierStarDetailXmlMap::value_type(sub_info.star, sub_info));
			} else {
				soldier_star_xml_info_t info;
				info.soldier_id = soldier_id;
				info.star_map.insert(SoldierStarDetailXmlMap::value_type(sub_info.star, sub_info));

				soldier_map.insert(SoldierStarXmlMap::value_type(soldier_id, info));
			}

			TRACE_LOG("load soldier star xml info\t[%u %u %u %u %u %u]", 
					soldier_id, sub_info.star, sub_info.soul_id, sub_info.soul_cnt, sub_info.golds, sub_info.evolution_id);

		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							SoldierTrainCostXmlManager Class					*/
/********************************************************************************/
SoldierTrainCostXmlManager::SoldierTrainCostXmlManager()
{

}

SoldierTrainCostXmlManager::~SoldierTrainCostXmlManager()
{

}

const soldier_train_cost_xml_info_t *
SoldierTrainCostXmlManager::get_soldier_train_cost_xml_info(uint32_t lv)
{
	SoldierTrainCostXmlMap::iterator it = train_cost_map.find(lv);
	if (it != train_cost_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
SoldierTrainCostXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_soldier_train_cost_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
SoldierTrainCostXmlManager::load_soldier_train_cost_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("cost"))) {
			uint32_t lv = 0;
			get_xml_prop(lv, cur, "lv");
			SoldierTrainCostXmlMap::iterator it = train_cost_map.find(lv);
			if (it != train_cost_map.end()) {
				ERROR_LOG("load soldier train_cost xml info err, lv exists, lv=%u", lv);
				return -1;
			}

			soldier_train_cost_xml_info_t info;
			info.level = lv;
			get_xml_prop(info.cost[0], cur, "hp_golds");
			get_xml_prop(info.cost[1], cur, "ad_golds");
			get_xml_prop(info.cost[2], cur, "armor_golds");
			get_xml_prop(info.cost[3], cur, "resist_golds");

			TRACE_LOG("load soldier train cost xml info\t[%u %u %u %u %u]", 
					info.level, info.cost[0], info.cost[1], info.cost[2], info.cost[3]);

			train_cost_map.insert(SoldierTrainCostXmlMap::value_type(lv, info));
		}
		cur = cur->next;
	}

	return 0;
}
/********************************************************************************/
/*							SoldierLevelAttrXmlManager Class					*/
/********************************************************************************/
SoldierLevelAttrXmlManager::SoldierLevelAttrXmlManager()
{

}

SoldierLevelAttrXmlManager::~SoldierLevelAttrXmlManager()
{

}

const soldier_level_attr_xml_info_t*
SoldierLevelAttrXmlManager::get_soldier_level_attr_xml_info(uint32_t soldier_id)
{
	SoldierLevelAttrXmlMap::const_iterator it = soldier_level_attr_xml_map.find(soldier_id);
	if (it != soldier_level_attr_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
SoldierLevelAttrXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_soldier_level_attr_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

int
SoldierLevelAttrXmlManager::load_soldier_level_attr_xml_info(xmlNodePtr cur)
{
	 cur = cur->xmlChildrenNode;
	 while (cur) {
	 	if (!xmlStrcmp(cur->name, XMLCHAR_CAST("soldier"))) {
			uint32_t soldier_id = 0;
			get_xml_prop(soldier_id, cur, "id");
			SoldierLevelAttrXmlMap::iterator it = soldier_level_attr_xml_map.find(soldier_id);
			if (it != soldier_level_attr_xml_map.end()) {
				ERROR_LOG("load soldier level attr xml info err, soldier_id=%u", soldier_id);
				return -1;
			}

			soldier_level_attr_xml_info_t info = {};
			info.soldier_id = soldier_id;
			get_xml_prop(info.max_hp, cur, "maxhp");
			get_xml_prop(info.ad, cur, "ad");
			get_xml_prop(info.armor, cur, "armor");
			get_xml_prop(info.resist, cur, "resist");

			TRACE_LOG("load soldier level attr xml info\t[%u %f %f %f %f]", info.soldier_id, info.max_hp, info.ad, info.armor, info.resist);

			soldier_level_attr_xml_map.insert(SoldierLevelAttrXmlMap::value_type(soldier_id, info));
		}
		cur = cur->next;
	 }

	 return 0;
}

/********************************************************************************/
/*								client request									*/
/********************************************************************************/
/* @brief 拉取小兵信息
 */
int cli_get_soldiers_info(Player *p, Cmessage *c_in)
{
	return send_msg_to_dbroute(p, db_get_player_soldiers_info_cmd, 0, p->user_id);
}

/* @brief 小兵强化
 */
int cli_strengthen_soldier(Player *p, Cmessage *c_in)
{
	cli_strengthen_soldier_in *p_in = P_IN;
	Soldier *p_soldier = p->soldier_mgr->get_soldier(p_in->soldier_id);
	if (!p_soldier) {
		T_KWARN_LOG(p->user_id, "soldier not exists\t[soldier_id=%u]", p_in->soldier_id);
		return p->send_to_self_error(p->wait_cmd, cli_soldier_not_exist_err, 1);
	}

	uint32_t old_rank = p_soldier->rank;
	uint32_t old_rank_exp = p_soldier->rank_exp;

	int ret = p_soldier->strength_soldier(p_in->cards);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_strengthen_soldier_out cli_out;
	cli_out.soldier_id = p_soldier->id;
	cli_out.old_rank = old_rank;
	cli_out.old_rank_exp = old_rank_exp;
	cli_out.rank = p_soldier->rank;
	cli_out.rank_exp = p_soldier->rank_exp;
	cli_out.rank_up_exp = p_soldier->get_rank_up_exp();
	cli_out.max_hp = p_soldier->max_hp;
	cli_out.ad = p_soldier->ad;
	cli_out.armor = p_soldier->armor;
	cli_out.resist = p_soldier->resist;

	T_KDEBUG_LOG(p->user_id, "STRENGTH SOLDIER\t[soldier_id=%u, old_rank=%u, old_rank_exp=%u, rank=%u, rank_exp=%u, rank_up_exp=%u]",
			cli_out.soldier_id, cli_out.old_rank, cli_out.old_rank_exp, cli_out.rank, cli_out.rank_exp, cli_out.rank_up_exp);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 小兵升星
 */
int cli_soldier_rising_star(Player *p, Cmessage *c_in)
{
	cli_soldier_rising_star_in *p_in = P_IN;
	Soldier *p_soldier = p->soldier_mgr->get_soldier(p_in->soldier_id);
	if (!p_soldier) {
		T_KWARN_LOG(p->user_id, "soldier not exists\t[soldier_id=%u]", p_in->soldier_id);
		return p->send_to_self_error(p->wait_cmd, cli_soldier_not_exist_err, 1);
	}

	int ret = p_soldier->rising_star();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_soldier_rising_star_out cli_out;
	cli_out.old_soldier_id = p_in->soldier_id;
	cli_out.soldier_id = p_soldier->id;
	cli_out.star = p_soldier->star;
	cli_out.max_hp = p_soldier->max_hp;
	cli_out.ad = p_soldier->ad;
	cli_out.armor = p_soldier->armor;
	cli_out.resist = p_soldier->resist;

	T_KDEBUG_LOG(p->user_id, "SOLDIER RISING STAR\t[soldier_id=%u, star=%u]", cli_out.soldier_id, cli_out.star);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 小兵训练
 */
int cli_soldier_training(Player *p, Cmessage *c_in)
{
	cli_soldier_training_in *p_in = P_IN;
	if (!p_in->type || p_in->type > 4) {
		return p->send_to_self_error(p->wait_cmd, cli_soldier_training_type_err, 1);
	}

	Soldier *p_soldier = p->soldier_mgr->get_soldier(p_in->soldier_id);
	if (!p_soldier) {
		T_KWARN_LOG(p->user_id, "soldier not exists\t[soldier_id=%u]", p_in->soldier_id);
		return p->send_to_self_error(p->wait_cmd, cli_soldier_not_exist_err, 1);
	}

	int ret = p_soldier->soldier_training(p_in->type, p_in->cost_point);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_soldier_training_out cli_out;
	cli_out.soldier_id = p_soldier->id;
	cli_out.type = p_in->type;
	cli_out.cost_point = p_in->cost_point;
	cli_out.cur_lv = p_soldier->train_lv[p_in->type - 1];
	cli_out.max_hp = p_soldier->max_hp;
	cli_out.ad = p_soldier->ad;
	cli_out.armor = p_soldier->armor;
	cli_out.resist = p_soldier->resist;
	cli_out.left_soldier_train_point = p->soldier_train_point;

	T_KDEBUG_LOG(p->user_id, "SOLDIER TRAINING\t[soldier_id=%u, type=%u, cost_point=%u, cur_lv=%u]", 
			cli_out.soldier_id, cli_out.type, cli_out.cost_point, cli_out.cur_lv);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);

}

/* @brief 设置小兵状态
 */
int cli_set_soldier_status(Player *p, Cmessage *c_in)
{
	cli_set_soldier_status_in *p_in = P_IN;
	Soldier *p_soldier = p->soldier_mgr->get_soldier(p_in->soldier_id);
	if (!p_soldier) {
		T_KWARN_LOG(p->user_id, "soldier not exists\t[soldier_id=%u]", p_in->soldier_id);
		return p->send_to_self_error(p->wait_cmd, cli_soldier_not_exist_err, 1);
	}

	p_soldier->set_soldier_status(p_in->status);

	T_KDEBUG_LOG(p->user_id, "SET SOLDIER STATUS\t[soldier_id=%u, status=%u]", p_in->soldier_id, p_in->status);

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 小兵使用道具
 */
int cli_use_items_for_soldier(Player *p, Cmessage *c_in)
{
	cli_use_items_for_soldier_in *p_in = P_IN;
	Soldier *p_soldier = p->soldier_mgr->get_soldier(p_in->soldier_id);
	if (!p_soldier) {
		T_KWARN_LOG(p->user_id, "soldier not exists\t[soldier_id=%u]", p_in->soldier_id);
		return p->send_to_self_error(p->wait_cmd, cli_soldier_not_exist_err, 1);
	}

	const item_xml_info_t *p_item_info = items_xml_mgr->get_item_xml_info(p_in->item_id);
	if (!p_item_info) {
		T_KWARN_LOG(p->user_id, "invalid item id\t[item_id=%u]", p_in->item_id);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_item_err, 1);
	}

	int ret = 0;
	if (p_item_info->type == em_item_type_for_soldier_exp) {
		ret = p_soldier->eat_exp_items(p_in->item_id, p_in->item_cnt);
	}

	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_use_items_for_soldier_out cli_out;
	cli_out.soldier_id = p_in->soldier_id;
	cli_out.item_id = p_in->item_id;
	cli_out.item_cnt = p_in->item_cnt;

	T_KDEBUG_LOG(p->user_id, "USE ITEMS FOR SOLDIER\t[soldier_id=%u, item_id=%u, item_cnt=%u]", p_in->soldier_id, p_in->item_id, p_in->item_cnt);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}


/********************************************************************************/
/*									dbsvr return 								*/
/********************************************************************************/
/* @brief 拉取小兵信息返回
 */
int db_get_player_soldiers_info(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);

	db_get_player_soldiers_info_out *p_in = P_IN;

	p->soldier_mgr->init_all_soldiers_info(p_in);

	if (p->wait_cmd == cli_proto_login_cmd) {
		p->login_step++;

		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u SOLDIERS INFO", p->login_step);

		//小兵信息
		cli_get_soldiers_info_out cli_out;
		p->soldier_mgr->pack_all_soldiers_info(cli_out);
		p->send_to_self(cli_get_soldiers_info_cmd, &cli_out, 0); 

		//拉取阵容信息
		return send_msg_to_dbroute(p, db_get_troop_list_cmd, 0, p->user_id);
	} else if (p->wait_cmd == cli_get_soldiers_info_cmd) {
		cli_get_soldiers_info_out cli_out;
		p->soldier_mgr->pack_all_soldiers_info(cli_out);

		return p->send_to_self(p->wait_cmd, &cli_out, 1);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}
