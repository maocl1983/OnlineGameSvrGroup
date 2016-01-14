/*
 * =====================================================================================
 *
 *  @file  player.cpp 
 *
 *  @brief  处理跟玩家有关的函数
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  kings, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"
#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"

#include "global_data.hpp"
#include "player.hpp"
#include "timer.hpp"
#include "cli_dispatch.hpp"
#include "hash.hpp"
#include "dbroute.hpp"
#include "wheel_timer.h"
#include "arena.hpp"
#include "redis.hpp"
#include "utils.hpp"
#include "astrology.hpp"
#include "trial_tower.hpp"
#include "lua_script_manage.hpp"

using namespace std;
using namespace project;

//PlayerManager g_player_mgr;
//RoleSkillXmlManager role_skill_xml_mgr;

/********************************************************************************/
/*								Player Class									*/
/********************************************************************************/
Player::Player(uint32_t uid, fdsession_t *fds) : user_id(uid), fdsess(fds)
{
	account_id = 0;
	memset(nick, 0x00, NICK_LEN);
	reg_tm = 0;
	sex = 0;
	role_id = 0;
	guild_id = 0;
	lv = 0;
	exp = 0;
	vip_lv = 0;

	golds = 0;
	diamond = 0;
	hero_soul = 0;
	energy = 0;
	used_energy = 0;
	endurance = 0;
	adventure = 0;
	last_logout_tm = 0;
	login_step = 0;
	login_completed = 0;

	skill_point = 0;
	soldier_train_point = 0;
	btl_soul_exp = 0;

	hero_mgr = new HeroManager(this);
	soldier_mgr = new SoldierManager(this);
	items_mgr = new ItemsManager(this);
	equip_mgr = new EquipmentManager(this);
	res_mgr = new Restriction(this);
	instance_mgr = new InstanceManager(this);
	btl_soul_mgr = new BtlSoulManager(this);
	ten_even_draw_mgr = new TenEvenDrawManager(this);
	horse = new Horse(this);
	horse_equip_mgr = new HorseEquipManager(this);
	treasure_mgr = new TreasureManager(this);
	task_mgr = new TaskManager(this);
	shop_mgr = new ShopManager(this);
	adventure_mgr = new AdventureManager(this);
	achievement_mgr = new AchievementManager(this);
	common_fight = new CommonFight(this);
	internal_affairs = new InternalAffairs(this);
	vip_mgr = new VipManager(this);
	astrology_mgr = new AstrologyManager(this);
	trial_tower_mgr = new TrialTowerManager(this);

	session_key = 0;
	last_conn_tm = time(0);
	wait_cmd = 0;
	seqno = 0;

	keep_alive_tm = 0;

	//keep_alive_tm = add_timer_event(0, keep_alive_noti, this, 0, 10000);
	//skill_point_tm = add_timer_event(0, add_player_skill_point, this, 0, SKILL_POINT_PER_SEC * 1000);
	skill_point_tm = add_timer_event(0, tm_add_player_skill_point_index, this, 0, 5 * 1000);
	soldier_train_point_tm = add_timer_event(0, tm_add_player_soldier_train_point_index, this, 0, SOLDIER_TRAIN_POINT_PER_SEC * 1000);
	energy_tm = add_timer_event(0, tm_add_player_energy_index, this, 0, ENERGY_PER_SEC * 1000);
	endurance_tm = add_timer_event(0, tm_add_player_endurance_index, this, 0, ENDURANCE_PER_SEC * 1000);
	adventure_tm = add_timer_event(0, tm_add_player_adventure_index, this, 0, ADVENTURE_PER_SEC * 1000);
	
	KDEBUG_LOG(user_id, "player create");
}

Player::~Player()
{
	SAFE_DELETE(hero_mgr);
	SAFE_DELETE(soldier_mgr);
	SAFE_DELETE(items_mgr);
	SAFE_DELETE(equip_mgr);
	SAFE_DELETE(res_mgr);
	SAFE_DELETE(instance_mgr);
	SAFE_DELETE(btl_soul_mgr);
	SAFE_DELETE(ten_even_draw_mgr);
	SAFE_DELETE(horse);
	SAFE_DELETE(horse_equip_mgr);
	SAFE_DELETE(treasure_mgr);
	SAFE_DELETE(task_mgr);
	SAFE_DELETE(shop_mgr);
	SAFE_DELETE(adventure_mgr);
	SAFE_DELETE(achievement_mgr);
	SAFE_DELETE(common_fight);
	SAFE_DELETE(internal_affairs);
	SAFE_DELETE(vip_mgr);
	SAFE_DELETE(astrology_mgr);
	SAFE_DELETE(trial_tower_mgr);

	/*
	if (keep_alive_tm) {
		do_remove_timer(keep_alive_tm, 1);
	}*/
	if (skill_point_tm) {
		do_remove_timer(skill_point_tm, 1);
	}
	if (soldier_train_point_tm) {
		do_remove_timer(soldier_train_point_tm, 1);
	}
	if (energy_tm) {
		do_remove_timer(energy_tm, 1);
	}
	if (endurance_tm) {
		do_remove_timer(endurance_tm, 1);
	}
	if (adventure_tm) {
		do_remove_timer(adventure_tm, 1);
	}
}

int
Player::set_player_level(uint32_t lv)
{
	//更新缓存
	this->lv = lv;

	//更新redis
	redis_mgr->set_user_level(this);

	//更新DB
	db_set_player_level_in db_in;
	db_in.lv = lv;
	return send_msg_to_dbroute(0, db_set_player_level_cmd, &db_in, this->user_id);
}

int
Player::add_role_exp(uint32_t add_value)
{
	Hero *p_hero = this->hero_mgr->get_hero(this->role_id);
	if (p_hero) {
		p_hero->add_exp(add_value);
	}

	return 0;
}

int 
Player::chg_golds(int32_t chg_value)
{
	if (!chg_value || (chg_value && static_cast<uint32_t>(abs(chg_value)) >= MAX_GOLDS)) {
		return 0;
	}

	if (chg_value < 0) {
		if (golds < static_cast<uint32_t>(abs(chg_value))) {
			chg_value = -golds;
		}
	} else {
		if (golds + chg_value > MAX_GOLDS) {
			chg_value = golds < MAX_GOLDS ? MAX_GOLDS - golds : 0;
		}	
	}

	//更新缓存
	golds += chg_value;

	//更新DB
	db_change_golds_in db_in;
	db_in.flag = chg_value > 0 ? 1 : 0;
	db_in.chg_value = static_cast<uint32_t>(abs(chg_value));
	send_msg_to_dbroute(0, db_change_golds_cmd, &db_in, this->user_id);

	//通知前端
	cli_golds_change_noti_out noti_out;
	noti_out.golds = golds;
	this->send_to_self(cli_golds_change_noti_cmd, &noti_out, 0);

	T_KDEBUG_LOG(this->user_id, "CHANGE GOLDS\t[golds=%u chg_value=%d]", golds, chg_value);

	return 0;
}

int 
Player::chg_diamond(int32_t chg_value)
{
	if (!chg_value || (chg_value && static_cast<uint32_t>(abs(chg_value)) >= MAX_DIAMOND)) {
		return 0;
	}

	if (chg_value < 0) {
		if (diamond < static_cast<uint32_t>(abs(chg_value))) {
			chg_value = -diamond;
		}
		//记录消费的钻石数
		uint32_t cost_diamond = this->res_mgr->get_res_value(forever_total_cost_diamond);
		cost_diamond += static_cast<uint32_t>(abs(chg_value));
		this->res_mgr->set_res_value(forever_total_cost_diamond, cost_diamond);
	} else {
		if (diamond + chg_value > MAX_DIAMOND) {
			chg_value = diamond < MAX_DIAMOND ? MAX_DIAMOND - diamond : 0;
		}	
	}

	//更新缓存
	diamond += chg_value;

	//更新DB
	db_change_diamond_in db_in;
	db_in.flag = chg_value > 0 ? 1 : 0;
	db_in.chg_value = static_cast<uint32_t>(abs(chg_value));
	send_msg_to_dbroute(0, db_change_diamond_cmd, &db_in, this->user_id);

	//通知前端
	cli_diamond_change_noti_out noti_out;
	noti_out.diamond = diamond;
	this->send_to_self(cli_diamond_change_noti_cmd, &noti_out, 0);

	T_KDEBUG_LOG(this->user_id, "CHANGE DIAMOND\t[diamond=%u chg_value=%d]", diamond, chg_value);

	return 0;
}

int 
Player::chg_hero_soul(int32_t chg_value)
{
	if (!chg_value || (chg_value && static_cast<uint32_t>(abs(chg_value)) >= MAX_HERO_SOUL)) {
		return 0;
	}

	if (chg_value < 0) {
		if (hero_soul < static_cast<uint32_t>(abs(chg_value))) {
			chg_value = -hero_soul;
		}
	} else {
		if (hero_soul + chg_value > MAX_HERO_SOUL) {
			chg_value = hero_soul < MAX_HERO_SOUL ? MAX_HERO_SOUL - hero_soul : 0;
		}	
	}

	//更新缓存
	hero_soul += chg_value;

	//更新DB
	db_change_hero_soul_in db_in;
	db_in.flag = chg_value > 0 ? 1 : 0;
	db_in.chg_value = static_cast<uint32_t>(abs(chg_value));
	send_msg_to_dbroute(0, db_change_hero_soul_cmd, &db_in, this->user_id);

	//通知前端
	cli_hero_soul_change_noti_out noti_out;
	noti_out.hero_soul = hero_soul;
	this->send_to_self(cli_hero_soul_change_noti_cmd, &noti_out, 0);

	T_KDEBUG_LOG(this->user_id, "CHANGE HERO SOUL\t[hero_soul=%u chg_value=%d]", hero_soul, chg_value);

	return 0;
}

int
Player::calc_btl_power()
{
	uint32_t old_btl_power = btl_power;
	btl_power = this->hero_mgr->calc_btl_power();
	btl_power += this->soldier_mgr->calc_btl_power();

	if (btl_power > max_btl_power) {
		max_btl_power = btl_power;

		//更新DB
		db_update_role_max_btl_power_in db_in;
		db_in.btl_power = max_btl_power;
		send_msg_to_dbroute(0, db_update_role_max_btl_power_cmd, &db_in, this->user_id);

		//检查任务
		this->task_mgr->check_task(em_task_type_btl_power, btl_power);
	}

	//通知前端
	if (btl_power != old_btl_power) {
		cli_send_role_btl_power_change_noti_out noti_out;
		noti_out.btl_power = btl_power;
		noti_out.max_btl_power = max_btl_power;
		this->send_to_self(cli_send_role_btl_power_change_noti_cmd, &noti_out, 0);

		T_KTRACE_LOG(this->user_id, "BTL POWER CHANGED\t[old_btl_power=%u, btl_power=%u]", old_btl_power, btl_power);
	}

	return btl_power;
}

int
Player::calc_max_energy()
{
	/*
	const level_xml_info_t *p_info = level_xml_mgr->get_level_xml_info(lv);
	if (!p_info) {
		return 0;
	}

	uint32_t total_honor_lv = this->hero_mgr->calc_all_heros_honor_lv();
	uint32_t need_honor_lv[] = {3150, 3300, 3450, 3600, 3750, 3900, 4050, 4200, 4350, 4500};
	uint32_t add_max_energy = 0;
	for (uint32_t i = 0; i < sizeof(need_honor_lv) / sizeof(need_honor_lv[0]); i++) {
		if (total_honor_lv >= need_honor_lv[i]) {
			add_max_energy += 5;
		}
	}

	uint32_t max_energy = p_info->max_energy + add_max_energy;
	*/

	return 150;
}

int
Player::init_skill_point()
{
	uint32_t cur_tm = get_now_tv()->tv_sec;
	uint32_t diff_tm = cur_tm - last_logout_tm;

	uint32_t diff_point = diff_tm / SKILL_POINT_PER_SEC;

	this->chg_skill_point(diff_point);


	return skill_point;
}

int 
Player::add_skill_point_tm()
{
	uint32_t max_skill_point = vip_lv ? 20 : 10;
	if (skill_point >= max_skill_point) {
		return 0;
	}

	chg_skill_point(1);

	return 0;
}

int
Player::init_soldier_train_point()
{
	uint32_t cur_tm = get_now_tv()->tv_sec;
	uint32_t diff_tm = cur_tm - last_logout_tm;

	uint32_t diff_point = diff_tm / SOLDIER_TRAIN_POINT_PER_SEC;

	this->chg_soldier_train_point(diff_point);

	return 0;
}

int 
Player::add_soldier_train_point_tm()
{
	uint32_t max_soldier_train_point = vip_lv ? 20 : 10;
	if (soldier_train_point >= max_soldier_train_point) {
		return 0;
	}

	chg_soldier_train_point(1);

	return 0;
}

int
Player::init_energy()
{
	uint32_t cur_tm = get_now_tv()->tv_sec;
	uint32_t diff_tm = cur_tm - last_logout_tm;

	uint32_t diff_point = diff_tm / ENERGY_PER_SEC;

	this->chg_energy(diff_point, true);

	return 0;
}

int 
Player::add_energy_tm()
{
	chg_energy(1, true);
	return 0;
}

int
Player::init_endurance()
{
	uint32_t cur_tm = get_now_tv()->tv_sec;
	uint32_t diff_tm = cur_tm - last_logout_tm;

	uint32_t diff_point = diff_tm / ENDURANCE_PER_SEC;

	this->chg_endurance(diff_point, true);

	return 0;
}

int 
Player::add_endurance_tm()
{
	chg_endurance(1, true);
	return 0;
}

int
Player::init_adventure()
{
	uint32_t cur_tm = get_now_tv()->tv_sec;
	uint32_t diff_tm = cur_tm - last_logout_tm;

	uint32_t diff_point = diff_tm / ADVENTURE_PER_SEC;

	this->chg_adventure(diff_point, true);

	return 0;
}

int
Player::add_adventure_tm()
{
	chg_adventure(1, true);
	return 0;
}

int
Player::chg_energy(int32_t chg_value, bool recovery_flag, bool role_exp_flag)
{
	uint32_t max_limit = MAX_ENERGY;
	if (!recovery_flag) {
		max_limit = 9999;
	}
	if (!chg_value) {
		return 0;
	}

	if (chg_value < 0) {
		if (energy < static_cast<uint32_t>(abs(chg_value))) {
			chg_value = -energy;
		}
		used_energy += static_cast<uint32_t>(abs(chg_value));
	} else {
		if (energy + chg_value > max_limit) {
			chg_value = energy < max_limit ? max_limit - energy : 0;
		}
	}

	if (role_exp_flag && chg_value < 0) {//增加主公经验
		Hero *p_hero = this->hero_mgr->get_hero(this->role_id);
		if (p_hero) {
			uint32_t abs_value = static_cast<uint32_t>(abs(chg_value));
			uint32_t add_exp = abs_value * p_hero->lv * 2;
			p_hero->add_exp(add_exp);
		}
	}

	//更新缓存
	energy += chg_value;

	//更新DB
	db_change_energy_in db_in;
	db_in.flag = chg_value > 0 ? 1 : 0;
	db_in.chg_value = static_cast<uint32_t>(abs(chg_value));
	send_msg_to_dbroute(0, db_change_energy_cmd, &db_in, this->user_id);

	//行动力变化通知
	if (g_player_mgr->get_player_by_uid(this->user_id)) {//玩家在线，则通知
		send_energy_change_noti();
	}

	T_KDEBUG_LOG(this->user_id, "CHANGE ENERGY\t[energy=%u chg_value=%d]", energy, chg_value);

	return 0;
}

int
Player::calc_max_endurance()
{
	/*
	uint32_t total_honor_lv = this->hero_mgr->calc_all_heros_honor_lv();
	uint32_t need_honor_lv[] = {3, 75, 150, 300, 450, 600, 750, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
	uint32_t add_max_endurance = 0;
	for (uint32_t i = 0; i < sizeof(need_honor_lv) / sizeof(need_honor_lv[0]); i++) {
		if (total_honor_lv >= need_honor_lv[i]) {
			add_max_endurance += 2;
		}
	}*/
	return MAX_ENDURANCE;
}

int
Player::chg_endurance(int32_t chg_value, bool recovery_flag, bool role_exp_flag)
{
	uint32_t max_endurance = calc_max_endurance();
	uint32_t max_limit = max_endurance;
	if (!recovery_flag) {
		max_limit = 9999;
	}
	if (!chg_value) {
		return 0;
	}

	if (chg_value < 0) {
		if (endurance < static_cast<uint32_t>(abs(chg_value))) {
			chg_value = -endurance;
		}
	} else {
		if (endurance + chg_value > max_limit) {
			chg_value = endurance < max_limit ? max_limit - endurance : 0;
		}
	}

	//更新缓存
	endurance += chg_value;

	if (role_exp_flag && chg_value < 0) {//增加主公经验
		Hero *p_hero = this->hero_mgr->get_hero(this->role_id);
		if (p_hero) {
			uint32_t abs_value = static_cast<uint32_t>(abs(chg_value));
			uint32_t add_exp = abs_value * p_hero->lv * 1;
			p_hero->add_exp(add_exp);
		}
	}
	
	//更新DB
	db_update_endurance_in db_in;
	db_in.endurance = endurance;
	send_msg_to_dbroute(0, db_update_endurance_cmd, &db_in, this->user_id);

	//通知前端
	if (g_player_mgr->get_player_by_uid(this->user_id)) {//玩家在线，则通知
		uint32_t now_sec = get_now_tv()->tv_sec;
		cli_endurance_change_noti_out noti_out;
		noti_out.endurance = endurance;
		noti_out.max_endurance = max_endurance;
		noti_out.endurance_recovery_tm = endurance_tm->tv.tv_sec > now_sec ? endurance_tm->tv.tv_sec - now_sec : 0;
		noti_out.total_endurance_recovery_tm = (endurance+1)<MAX_ENDURANCE ? ((MAX_ENDURANCE - endurance - 1) * ENERGY_PER_SEC + noti_out.endurance_recovery_tm) : 0;

		this->send_to_self(cli_endurance_change_noti_cmd, &noti_out, 0);
	}

	T_KDEBUG_LOG(this->user_id, "CHANGE ENDURANCE\t[endurance=%u chg_value=%d]", endurance, chg_value);

	return 0;
}

int
Player::calc_max_adventure()
{
	return 10;
}

int
Player::chg_adventure(int32_t chg_value, bool recovery_flag)
{
	uint32_t max_adventure = calc_max_adventure();
	uint32_t max_limit = max_adventure;
	if (!recovery_flag) {
		max_limit = 9999;
	}
	if (!chg_value) {
		return 0;
	}

	if (chg_value < 0) {
		if (endurance < static_cast<uint32_t>(abs(chg_value))) {
			chg_value = -adventure;
		}
	} else {
		if (adventure + chg_value > max_limit) {
			chg_value = adventure < max_limit ? max_limit - adventure : 0;
		}
	}

	//更新缓存
	adventure += chg_value;

	//更新DB
	db_update_adventure_in db_in;
	db_in.adventure = adventure;
	send_msg_to_dbroute(0, db_update_adventure_cmd, &db_in, this->user_id);

	//通知前端
	if (g_player_mgr->get_player_by_uid(this->user_id)) {//玩家在线，则通知
		cli_adventure_change_noti_out noti_out;
		noti_out.adventure = adventure;
		noti_out.max_adventure = max_adventure;
		this->send_to_self(cli_adventure_change_noti_cmd, &noti_out, 0);
	}

	T_KDEBUG_LOG(this->user_id, "CHANGE ENDURANCE\t[adventure=%u chg_value=%d]", adventure, chg_value);

	return 0;
}

int
Player::chg_btl_soul_exp(int32_t chg_value)
{
	if (!chg_value || (chg_value && static_cast<uint32_t>(abs(chg_value)) >= MAX_BTL_SOUL_EXP)) {
		return 0;
	}

	if (chg_value < 0) {
		if (btl_soul_exp < static_cast<uint32_t>(abs(chg_value))) {
			chg_value = -btl_soul_exp;
		}
	} else {
		if (btl_soul_exp + chg_value > MAX_BTL_SOUL_EXP) {
			chg_value = btl_soul_exp < MAX_BTL_SOUL_EXP ? MAX_BTL_SOUL_EXP - btl_soul_exp : 0;
		}
	}

	//更新缓存
	btl_soul_exp += chg_value;

	//更新DB
	db_update_role_btl_soul_exp_in db_in;
	db_in.btl_soul_exp = btl_soul_exp;
	send_msg_to_dbroute(0, db_update_role_btl_soul_exp_cmd, &db_in, this->user_id);

	//通知前端
	cli_role_btl_soul_exp_noti_out noti_out;
	noti_out.btl_soul_exp = btl_soul_exp;
	this->send_to_self(cli_role_btl_soul_exp_noti_cmd, &noti_out, 0);

	T_KDEBUG_LOG(this->user_id, "CHANGE BTL SOUL EXP\t[btl_soul_exp=%u chg_value=%u]", btl_soul_exp, chg_value);

	return 0;
}

int
Player::send_energy_change_noti()
{
	uint32_t now_sec = get_now_tv()->tv_sec;
	cli_send_energy_change_noti_out noti_out;
	noti_out.energy = energy;
	noti_out.max_energy = max_energy;
	noti_out.energy_recovery_tm = energy_tm->tv.tv_sec > now_sec ? energy_tm->tv.tv_sec - now_sec : 0;
	noti_out.total_energy_recovery_tm = (energy + 1) < MAX_ENERGY ? ((MAX_ENERGY - energy - 1) * ENERGY_PER_SEC + noti_out.energy_recovery_tm) : 0;

	return this->send_to_self(cli_send_energy_change_noti_cmd, &noti_out, 0);
}

int 
Player::calc_arena_btl_power(const arena_info_t *p_info)
{
	uint32_t total_power = 0;
	Hero *p_main_hero = hero_mgr->get_hero(role_id);
	if (p_main_hero) {
		total_power += p_main_hero->btl_power;
	}
	for (int i = 0; i < 3; i++) {
		Hero *p_hero = hero_mgr->get_hero(p_info->hero[i]);
		if (p_hero) {
			total_power += p_hero->btl_power;
		}
	}
	for (int i = 0; i < 3; i++) {
		Soldier *p_soldier = soldier_mgr->get_soldier(p_info->soldier[i]);
		if (p_soldier) {
			total_power += p_soldier->btl_power;
		}
	}

	return total_power;
}

int
Player::deal_role_levelup(uint32_t old_lv, uint32_t lv)
{
	if (old_lv >= lv) {
		return 0;
	}
	//设置角色等级
	set_player_level(lv);

	for (uint32_t i = old_lv + 1; i <= lv; i++) {
		const level_xml_info_t *p_info = level_xml_mgr->get_level_xml_info(i);
		if (p_info) {
			//提高当前耐力
			this->chg_endurance(p_info->levelup_endurance);

			//提高当前奇遇点
			this->chg_adventure(p_info->adventure);

			//增加奇遇令
			/*
			uint32_t add_cnt = p_info->adventure;
			this->items_mgr->add_item_without_callback(120006, add_cnt);
			*/

			//增加物品
			if (p_info->item_id) {
				this->items_mgr->add_reward(p_info->item_id, p_info->item_cnt);
			}

			//解锁小兵
			if (p_info->unlock_soldier) {
				Soldier *p_soldier = this->soldier_mgr->get_soldier(p_info->unlock_soldier);
				if (!p_soldier) {
					this->soldier_mgr->add_soldier(p_info->unlock_soldier);
				}
			}
		}
	}

	//加入竞技场 TODO
	if (old_lv < 10 && lv >= 10) {
		arena_mgr->first_add_to_arena(this);
	}

	//检查任务 
	this->task_mgr->check_task(em_task_type_role_lv, lv);

	return 0;
}

int
Player::init_troop_list(db_get_troop_list_out *p_in)
{
	for (uint32_t i = 0; i < p_in->troop_list.size(); i++) {
		db_troop_info_t *p_info = &(p_in->troop_list[i]);
		troop_info_t info;
		info.type = p_info->type;
		for (int i = 0; i < 3; i++) {
			if (p_info->heros[i]) {
				info.heros.push_back(p_info->heros[i]);
			}
			if (p_info->soldiers[i]) {
				info.soldiers.push_back(p_info->soldiers[i]);
			}
		}

		troop_map.insert(TroopMap::value_type(p_info->type, info));

		T_KTRACE_LOG(this->user_id, "init troop list[%u %u %u %u %u %u %u]", 
				p_info->type, p_info->heros[0], p_info->heros[1], p_info->heros[2], p_info->soldiers[0], p_info->soldiers[1], p_info->soldiers[2]);
	}

	return 0;
}

int
Player::set_troop(cli_troop_info_t &troop_info)
{
	for (uint32_t i = 0; i < troop_info.heros.size(); i++) {
		uint32_t hero_id = troop_info.heros[i];
		Hero *p_hero = this->hero_mgr->get_hero(hero_id);
		if (!p_hero) {
			return cli_invalid_hero_err;
		}
		if (p_hero->trip_id) {
			return cli_hero_is_trip_hero_err;
		}
	}

	TroopMap::iterator it = troop_map.find(troop_info.type);
	if (it == troop_map.end()) {
		troop_info_t info;
		info.type = troop_info.type;
		for (uint32_t i = 0; i < troop_info.heros.size(); i++) {
			info.heros.push_back(troop_info.heros[i]);
		}
		for (uint32_t i = 0; i < troop_info.soldiers.size(); i++) {
			info.soldiers.push_back(troop_info.soldiers[i]);
		}
		troop_map.insert(TroopMap::value_type(info.type, info));
	} else {
		it->second.heros.clear();
		it->second.soldiers.clear();
		for (uint32_t i = 0; i < troop_info.heros.size(); i++) {
			it->second.heros.push_back(troop_info.heros[i]);
		}
		for (uint32_t i = 0; i < troop_info.soldiers.size(); i++) {
			it->second.soldiers.push_back(troop_info.soldiers[i]);
		}
	}

	//更新DB
	db_add_troop_info_in db_in;
	db_in.troop_info.type = troop_info.type;
	for (uint32_t i = 0; i < 3; i++) {
		if (i < troop_info.heros.size()) {
			db_in.troop_info.heros[i] = troop_info.heros[i];
		} else {
			db_in.troop_info.heros[i] = 0;
		}
		if (i < troop_info.soldiers.size()) {
			db_in.troop_info.soldiers[i] = troop_info.soldiers[i];
		} else {
			db_in.troop_info.soldiers[i] = 0;
		}
	}

	send_msg_to_dbroute(0, db_add_troop_info_cmd, &db_in, this->user_id);

	T_KDEBUG_LOG(this->user_id, "SET TROOP\t[%u %u %u %u %u %u %u]", 
			db_in.troop_info.type, db_in.troop_info.heros[0], db_in.troop_info.heros[1], db_in.troop_info.heros[2], db_in.troop_info.soldiers[0], db_in.troop_info.soldiers[1], db_in.troop_info.soldiers[2]);

	return 0;
}

const troop_info_t*
Player::get_troop(uint32_t type)
{
	TroopMap::iterator it = troop_map.find(type);
	if (it != troop_map.end()) {
		return &(it->second);
	}

	return 0;
}

bool
Player::check_is_troop_hero(uint32_t hero_id)
{
	TroopMap::iterator it = troop_map.begin();
	for (; it != troop_map.end(); ++it) {
		const troop_info_t *p_info = &(it->second);
		for (uint32_t i = 0; i < p_info->heros.size(); i++) {
			if (hero_id == p_info->heros[i]) {
				return true;
			}
		}
		
	}

	return false;
}

int
Player::init_player_login_info(db_get_player_login_info_out *p_in)
{
	reg_tm = p_in->regtime;
	sex = p_in->sex;
	role_id = p_in->role_id;
	guild_id = p_in->guild_id;
	memcpy(nick, p_in->nick, NICK_LEN);
	lv = p_in->lv;
	exp = p_in->exp;
	golds = p_in->golds;
	diamond = p_in->diamond;
	hero_soul = p_in->hero_soul;
	max_energy = calc_max_energy();
	energy = p_in->energy;
	used_energy = p_in->used_energy;
	endurance = p_in->endurance;
	adventure = p_in->adventure;
	max_btl_power = p_in->max_btl_power;
	last_login_tm = p_in->last_login_tm;
	last_logout_tm = p_in->last_logout_tm;
	today_online_tm = p_in->today_online_tm;
	cur_login_tm = get_now_tv()->tv_sec;
	skill_point = p_in->skill_point;
	soldier_train_point = p_in->soldier_train_point;
	btl_soul_exp = p_in->btl_soul_exp;
	this->horse->init_horse_info_from_db(p_in->horse_lv, p_in->horse_exp);
	this->internal_affairs->init_internal_affairs_exp_info_from_db(p_in->affairs_lv, p_in->affairs_exp);

	T_KTRACE_LOG(this->user_id, "login info\t[%u %u %u %s %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u]", 
			reg_tm, sex, role_id, nick, lv, exp, golds, diamond, hero_soul, energy, max_energy, endurance, adventure, max_btl_power, 
			last_login_tm, last_logout_tm, today_online_tm, cur_login_tm, skill_point, soldier_train_point, btl_soul_exp,
			p_in->horse_lv, p_in->horse_exp, p_in->affairs_lv, p_in->affairs_exp);

	return 0;
}


int 
Player::deal_something_when_login()
{
	handle_timer_login_init();

	handle_role_login_init();
	
	handle_redis_login_init();

	set_login_day();

	//lua_script_mgr.DoTest(this);
	return 0;
}

void
Player::handle_timer_login_init()
{
	init_energy();
	init_endurance();
	init_adventure();
	init_skill_point();
	init_soldier_train_point();
}

void
Player::handle_role_login_init()
{
	uint32_t limit = this->res_mgr->get_res_value(forever_role_login_init_flag);
	if (!limit) {//首次登陆添加英雄
		this->res_mgr->set_res_value(forever_role_login_init_flag, 1);

		//添加主公
		db_add_heros_in db_in;
		db_hero_info_t info;
		info.hero_id = 100;
		info.lv = 1; 
		info.rank = 0;
		info.star = 3;
		info.exp = 0;
		info.honor_lv = 0;
		info.honor = 0;
		info.skill_lv = 1;
		db_in.heros.push_back(info);
		send_msg_to_dbroute(0, db_add_heros_cmd, &db_in, this->user_id);

		//设置主公ID
		role_id = 100;
		db_set_role_id_in db_in2;
		db_in2.role_id = 100;
		send_msg_to_dbroute(0, db_set_role_id_cmd, &db_in2, this->user_id);
		//设置主公技能ID
		this->res_mgr->set_res_value(forever_main_hero_skill_1, 10021);
		//设置主公技能等级
		for (int i = 0; i < 12; i++) {
			uint32_t res_type = forever_main_hero_skill_1_lv + i;
			this->res_mgr->set_res_value(res_type, 1);
		}

		this->chg_skill_point(10);
		this->chg_soldier_train_point(10);
		this->chg_adventure(10);

		//设置主公等级
		set_player_level(1);

		//增加几个小兵
		uint32_t soldiers[1] = {2003};
		db_add_soldiers_in db_in3;
		for (int i = 0; i < 1; i++) {
			db_soldier_info_t info;
			info.soldier_id = soldiers[i];
			info.get_tm = time(0) + i;
			info.lv = 1;
			info.exp = 0;
			info.rank = 0;
			info.rank_exp = 0;
			info.star = 1;
			
			db_in3.soldiers.push_back(info);
		}
		send_msg_to_dbroute(0, db_add_soldiers_cmd, &db_in3, this->user_id);

		//加些物品
		uint32_t items[] = {501101, 502101, 502102};

		for (int i = 0; i < (int)(sizeof(items) / sizeof(items[0])); i++) {
			db_change_items_in db_in;
			db_in.flag = 1;
			db_in.item_id = items[i]; 
			db_in.chg_cnt = 1;

			send_msg_to_dbroute(0, db_change_items_cmd, &db_in, this->user_id);
		}

	}

	//初始化每日占星
	this->astrology_mgr->init_astrology_info();
}

void
Player::handle_redis_login_init()
{
	uint32_t limit = res_mgr->get_res_value(forever_redis_login_init_flag);
	if (!limit) {
		res_mgr->set_res_value(forever_redis_login_init_flag, 1);
		redis_mgr->set_user_nick(this);
		redis_mgr->set_user_level(this);
	}

	//redis_mgr->set_treasure_piece_user(this, 821101);
	//redis_mgr->set_treasure_piece_user(this, 821102);
	//redis_mgr->get_treasure_piece_user_list(this, 821101);
}

void
Player::set_login_day()
{
	uint32_t now_sec = time(0);
	uint32_t now_date = utils_mgr->get_date(now_sec);
	uint32_t last_login_date = utils_mgr->get_date(last_login_tm);

	if (now_date != last_login_date) {
		uint32_t login_day = this->res_mgr->get_res_value(forever_player_login_day);
		login_day++;
		this->res_mgr->set_res_value(forever_player_login_day, login_day);
	}
}

int 
Player::complete_new_player_lead_task(uint32_t lead_id)
{
	this->res_mgr->set_res_value(forever_new_player_lead_id, lead_id);

	return 0;
}

int
Player::get_today_online_tm()
{
	uint32_t cur_tm = get_now_tv()->tv_sec;
	uint32_t online_tm = today_online_tm + (cur_tm - cur_login_tm);

	return online_tm;
}

int 
Player::set_player_last_login_tm()
{
	if (cur_login_tm) {
		db_set_player_last_login_tm_in db_in;
		db_in.last_login_tm = cur_login_tm;
		send_msg_to_dbroute(0, db_set_player_last_login_tm_cmd, &db_in, user_id);
	}

	return 0;
}

int 
Player::set_player_last_logout_tm()
{
	if (cur_login_tm) {
		last_logout_tm = get_now_tv()->tv_sec;
		db_set_player_last_logout_tm_in db_in;
		db_in.last_logout_tm = get_now_tv()->tv_sec;
		db_in.today_online_tm = get_today_online_tm();
		send_msg_to_dbroute(0, db_set_player_last_logout_tm_cmd, &db_in, user_id);
	}

	return 0;
}

int 
Player::chg_skill_point(int32_t chg_value)
{
	DEBUG_LOG("CHG SKILL PT=%d", chg_value);
	if (!chg_value) {
		return 0;
	}
	uint32_t max_skill_point = vip_lv ? 20 : 10;

	if (chg_value < 0) {
		if (skill_point < static_cast<uint32_t>(abs(chg_value))) {
			chg_value = -skill_point;
		}
	} else {
		if (skill_point + chg_value > max_skill_point) {
			chg_value = skill_point < max_skill_point ? max_skill_point - skill_point : 0;
		}
	}

	//更新缓存
	skill_point += chg_value;

	//更新DB
	db_set_player_skill_point_in db_in;
	db_in.skill_point = skill_point;
	send_msg_to_dbroute(0, db_set_player_skill_point_cmd, &db_in, user_id);
	
	//通知前端
	if (g_player_mgr->get_player_by_uid(this->user_id)) {//玩家在线，则通知
		cli_skill_point_change_noti_out noti_out;
		noti_out.left_skill_point = skill_point;

		this->send_to_self(cli_skill_point_change_noti_cmd, &noti_out, 0);
	}

	return 0;
}

int 
Player::chg_soldier_train_point(int32_t chg_value)
{
	if (!chg_value) {
		return 0;
	}
	uint32_t max_soldier_train_point = vip_lv ? 20 : 10;

	if (chg_value < 0) {
		if (soldier_train_point < static_cast<uint32_t>(abs(chg_value))) {
			chg_value = -soldier_train_point;
		}
	} else {
		if (soldier_train_point + chg_value > max_soldier_train_point) {
			chg_value = soldier_train_point < max_soldier_train_point ? max_soldier_train_point - soldier_train_point : 0;
		}
	}

	//更新缓存
	soldier_train_point += chg_value;

	//更新DB
	db_set_player_soldier_train_point_in db_in;
	db_in.soldier_train_point = soldier_train_point;
	send_msg_to_dbroute(0, db_set_player_soldier_train_point_cmd, &db_in, user_id);

	if (g_player_mgr->get_player_by_uid(this->user_id)) {//玩家在线，则通知
		cli_soldier_train_point_change_noti_out noti_out;
		noti_out.left_soldier_train_point = soldier_train_point;

		this->send_to_self(cli_soldier_train_point_change_noti_cmd, &noti_out, 0);
	}

	return 0;
}

int
Player::calc_energy_buy_max_tms()
{
	return 2;
}

int
Player::calc_endurance_buy_max_tms()
{
	return 5;
}

int
Player::get_buy_item_left_tms(uint32_t type)
{
	uint32_t left_tms = 0;
	if (type == 3) {
		uint32_t daily_tms = this->res_mgr->get_res_value(daily_skill_point_buy_tms);
		left_tms = daily_tms ? 0 : 1;
	} else if (type == 4) {
		uint32_t daily_tms = this->res_mgr->get_res_value(daily_soldier_train_point_buy_tms);
		left_tms = daily_tms ? 0 : 1;
	} else {
		left_tms = this->shop_mgr->get_item_shop_left_buy_tms(type);
	}

	return left_tms;
}

int
Player::buy_item(uint32_t type)
{
	uint32_t daily_res = 0;
	uint32_t daily_max = 0;
	uint32_t item_id = 0;
	if (type == 3) {//技能点
		daily_res = daily_skill_point_buy_tms;
		daily_max = 1;
	} else if (type == 4) {//小兵训练点
		daily_res = daily_soldier_train_point_buy_tms;
		daily_max = 1;
	} else {
		const item_shop_xml_info_t *p_xml_info = item_shop_xml_mgr->get_item_shop_xml_info(type);
		if (p_xml_info) {
			daily_res = p_xml_info->res_type;
			daily_max = this->shop_mgr->get_item_shop_buy_limit(type);
			item_id = p_xml_info->item_id;
		} else {
			return cli_invalid_buy_item_type_err;
		}
	}

	uint32_t daily_tms = this->res_mgr->get_res_value(daily_res);
	if (daily_tms >= daily_max) {
		T_KWARN_LOG(this->user_id, "buy item daily tms not enough\t[type=%u, daily_tms=%u]", type, daily_tms);
		return cli_buy_item_tms_not_enough_err;
	}

	uint32_t cost_diamond = 0;
	if (type == 3) {
		cost_diamond = 50;
	} else if (type == 4) {
		cost_diamond = 50;
	} else {
		cost_diamond = this->shop_mgr->get_item_shop_price(item_id, daily_tms);
	}
	if (this->diamond < cost_diamond) {
		T_KWARN_LOG(this->user_id, "but item need diamond not enough\t[diamond=%u, cost_diamond=%u]", diamond, cost_diamond);
		return cli_not_enough_diamond_err;
	}

	//扣除钻石
	this->chg_diamond(-cost_diamond);

	//增加次数
	daily_tms++;
	this->res_mgr->set_res_value(daily_res, daily_tms);

	//添加物品
	if (type == 3) {//技能点
		this->chg_skill_point(10);
	} else if (type == 4) {//小兵训练点
		this->chg_soldier_train_point(10);
	} else {
		this->items_mgr->add_item_without_callback(item_id, 1);
	}

	return 0;
}

int
Player::eat_energy_items(uint32_t item_id, uint32_t item_cnt)
{
	const item_xml_info_t *p_info = items_xml_mgr->get_item_xml_info(item_id);
	if (!p_info || p_info->type != em_item_type_for_energy) {
		T_KWARN_LOG(this->user_id, "invalid energy item\t[item_id=%u]", item_id);
		return cli_invalid_item_err;
	}

	uint32_t cur_cnt = this->items_mgr->get_item_cnt(item_id);
	if (cur_cnt < item_cnt) {
		T_KWARN_LOG(this->user_id, "eat energy items not enough\t[cur_cnt=%u, eat_cnt=%u]", cur_cnt, item_cnt);
		return cli_not_enough_item_err;
	}

	//扣除物品
	this->items_mgr->del_item_without_callback(item_id, item_cnt);

	//增加体力
	uint32_t total_energy = p_info->effect * item_cnt;
	this->chg_energy(total_energy);

	return 0;
}

int
Player::eat_endurance_items(uint32_t item_id, uint32_t item_cnt)
{
	const item_xml_info_t *p_info = items_xml_mgr->get_item_xml_info(item_id);
	if (!p_info || p_info->type != em_item_type_for_endurance) {
		T_KWARN_LOG(this->user_id, "invalid endurance item\t[item_id=%u]", item_id);
		return cli_invalid_item_err;
	}

	uint32_t cur_cnt = this->items_mgr->get_item_cnt(item_id);
	if (cur_cnt < item_cnt) {
		T_KWARN_LOG(this->user_id, "eat endurance items not enough\t[cur_cnt=%u, eat_cnt=%u]", cur_cnt, item_cnt);
		return cli_not_enough_item_err;
	}

	//扣除物品
	this->items_mgr->del_item_without_callback(item_id, item_cnt);

	//增加体力
	uint32_t total_endurance = p_info->effect * item_cnt;
	this->chg_endurance(total_endurance);

	return 0;
}

int
Player::eat_adventure_items(uint32_t item_id, uint32_t item_cnt)
{
	const item_xml_info_t *p_info = items_xml_mgr->get_item_xml_info(item_id);
	if (!p_info || p_info->type != em_item_type_for_adventure) {
		T_KWARN_LOG(this->user_id, "invalid adventure item\t[item_id=%u]", item_id);
		return cli_invalid_item_err;
	}

	uint32_t cur_cnt = this->items_mgr->get_item_cnt(item_id);
	if (cur_cnt < item_cnt) {
		T_KWARN_LOG(this->user_id, "eat adventure items not enough\t[cur_cnt=%u, eat_cnt=%u]", cur_cnt, item_cnt);
		return cli_not_enough_item_err;
	}

	//扣除物品
	this->items_mgr->del_item_without_callback(item_id, item_cnt);

	//增加体力
	uint32_t total_adventure = p_info->effect * item_cnt;
	this->chg_adventure(total_adventure);

	return 0;
}

int
Player::use_battle_items(uint32_t item_id, uint32_t item_cnt)
{
	const item_xml_info_t *p_info = items_xml_mgr->get_item_xml_info(item_id);
	if (!p_info || p_info->type != em_item_type_for_battle) {
		T_KWARN_LOG(this->user_id, "invalid battle item\t[item_id=%u]", item_id);
		return cli_invalid_item_err;
	}

	uint32_t cur_cnt = this->items_mgr->get_item_cnt(item_id);
	if (cur_cnt < item_cnt) {
		T_KWARN_LOG(this->user_id, "use battle items not enough\t[cur_cnt=%u, eat_cnt=%u]", cur_cnt, item_cnt);
		return cli_not_enough_item_err;
	}

	//扣除物品
	this->items_mgr->del_item_without_callback(item_id, item_cnt);

	return 0;
}

int
Player::set_role_skills(std::vector<uint32_t> &skills)
{
	//先清空原先的
	for (int i = 0; i < 3; i++) {
		uint32_t res_type = forever_main_hero_skill_1 + i;
		uint32_t res_value = this->res_mgr->get_res_value(res_type);
		if (res_value) {
			this->res_mgr->set_res_value(res_type, 0);
		}
	}
	for (uint32_t i = 0; i < skills.size(); i++) {
		uint32_t skill_id = skills[i];
		const role_skill_xml_info_t *p_xml_info = role_skill_xml_mgr->get_role_skill_xml_info_by_skill(skill_id);
		if (!p_xml_info) {
			return cli_invalid_skill_id_err;
		}
		if (this->lv < p_xml_info->unlock_lv) {
			return cli_role_skill_not_unlock_err;
		}
		if (i < 3) {
			uint32_t res_type = forever_main_hero_skill_1 + i;
			this->res_mgr->set_res_value(res_type, skill_id);
		}
	}

	return 0;
}

bool
Player::get_role_skill_state(uint32_t skill_id)
{
	for (int i = 0; i < 3; i++) {
		uint32_t res_type = forever_main_hero_skill_1 + i;
		uint32_t res_value = this->res_mgr->get_res_value(res_type);
		if (res_value == skill_id) {
			return true;
		}
	}

	return false;
}

int 
Player::pack_player_login_info(cli_proto_login_out &cli_out)
{
	cli_out.user_id = this->user_id;
	cli_out.time = get_now_tv()->tv_sec;
	cli_out.session_key = session_key;
	pack_player_base_info(cli_out.base_info);

	return 0;
}

int
Player::pack_player_base_info(cli_player_base_info_t &info)
{
	uint32_t now_sec = get_now_tv()->tv_sec;
	info.regtime = reg_tm;
	info.sex = sex;
	memcpy(info.nick, nick, NICK_LEN);
	info.role_id = role_id;
	info.guild_id = guild_id;
	info.lv = lv;
	info.exp = exp;
	info.vip_lv = vip_lv;
	info.golds = golds;
	info.diamond = diamond;
	info.hero_soul = hero_soul;
	info.max_energy = max_energy;
	info.energy = energy;
	info.used_energy = used_energy;
	info.energy_recovery_tm = ENERGY_PER_SEC;
	info.total_energy_recovery_tm = energy < MAX_ENERGY ? (MAX_ENERGY - energy) * ENERGY_PER_SEC : 0;
	info.endurance = endurance;
	info.max_endurance = calc_max_endurance();
	info.endurance_recovery_tm = ENDURANCE_PER_SEC;
	info.total_endurance_recovery_tm = endurance < MAX_ENDURANCE ? (MAX_ENDURANCE - endurance) : 0;
	info.adventure = adventure;
	info.max_adventure = calc_max_adventure();
	info.btl_power = btl_power;
	info.max_btl_power = max_btl_power;
	info.last_login_tm = last_login_tm;
	info.last_logout_tm = last_logout_tm;
	info.left_skill_point = skill_point;
	if (skill_point < 10 && skill_point_tm) {
		info.left_skill_point_tm = skill_point_tm->tv.tv_sec > now_sec ? skill_point_tm->tv.tv_sec - now_sec : 0;
	}
	info.left_soldier_train_point = soldier_train_point;
	if (soldier_train_point < 10 && soldier_train_point_tm) {
		info.left_soldier_train_point_tm = soldier_train_point_tm->tv.tv_sec > now_sec ? soldier_train_point_tm->tv.tv_sec - now_sec : 0;
	}
	info.btl_soul_exp = btl_soul_exp;
	info.divine_id = this->res_mgr->get_res_value(forever_divine_id);
	info.lead_id = this->res_mgr->get_res_value(forever_new_player_lead_id);
	if (!info.divine_id) {
		info.divine_id = 1;
		this->res_mgr->set_res_value(forever_divine_id, 1);
	}
	info.energy_gift_stat = this->res_mgr->get_res_value(daily_energy_gift_stat);
	info.vip_daily_gift_stat = this->res_mgr->get_res_value(daily_vip_gift_stat);
	pack_items_buy_list(info.items_buy_info);
	role_skill_xml_mgr->pack_unlock_role_skill(this, info.role_skills);

	return 0;
}

void
Player::pack_items_buy_list(vector<cli_item_buy_info_t> &vec)
{
	cli_item_buy_info_t info[9];
	info[0].item_id = 100001;
	info[0].buy_tms = this->res_mgr->get_res_value(daily_shop_3_item_100001_buy_tms);
	info[0].buy_limit = 5;

	info[1].item_id = 102001;
	info[1].buy_tms = this->res_mgr->get_res_value(daily_shop_3_item_102001_buy_tms);
	info[1].buy_limit = 5;

	info[2].item_id = 120006;
	info[2].buy_tms = this->res_mgr->get_res_value(daily_shop_3_item_120006_buy_tms);
	info[2].buy_limit = 5;

	info[3].item_id = 120007;
	info[3].buy_tms = this->res_mgr->get_res_value(daily_shop_3_item_120007_buy_tms);
	info[3].buy_limit = this->calc_energy_buy_max_tms();

	info[4].item_id = 120008;
	info[4].buy_tms = this->res_mgr->get_res_value(daily_shop_3_item_120008_buy_tms);
	info[4].buy_limit = this->calc_endurance_buy_max_tms();

	info[5].item_id = 120001;
	info[5].buy_tms = this->res_mgr->get_res_value(daily_shop_3_item_120001_buy_tms);
	info[5].buy_limit = 5;

	info[6].item_id = 121000;
	info[6].buy_tms = this->res_mgr->get_res_value(10207);
	info[6].buy_limit = 5;

	info[7].item_id = 3;
	info[7].buy_tms = this->res_mgr->get_res_value(daily_skill_point_buy_tms);
	info[7].buy_limit = 1;

	info[8].item_id = 4;
	info[8].buy_tms = this->res_mgr->get_res_value(daily_soldier_train_point_buy_tms);
	info[8].buy_limit = 1;

	for (int i = 0; i < 9; i++) {
		vec.push_back(info[i]);
	}
}

void
Player::pack_player_troop_list(cli_get_troop_list_out &cli_out)
{
	TroopMap::iterator it = troop_map.begin();
	for (; it != troop_map.end(); ++it) {
		troop_info_t *p_info = &(it->second);
		cli_troop_info_t info;
		info.type = p_info->type;
		for (uint32_t i = 0; i < p_info->heros.size(); i++) {
			info.heros.push_back(p_info->heros[i]);
		}
		for (uint32_t i = 0; i < p_info->soldiers.size(); i++) {
			info.soldiers.push_back(p_info->soldiers[i]);
		}
		cli_out.troop_list.push_back(info);
	}
}

int
Player::send_msg_to_cli(uint16_t cmd, Cmessage *msg)
{
	cli_proto_head_t st;
    st.len = sizeof(cli_proto_head_t);
    st.cmd = cmd;
    st.ret = 0;
    st.seq_num = seqno;
    st.user_id = user_id;
    return send_msg_to_client(fdsess, (char *)&st, msg);
}

int
Player::send_to_self(uint16_t cmd, Cmessage *c_out, int completed)
{
	if (send_msg_to_cli(cmd, c_out) == -1) {
		ERROR_LOG("failed to send pkg to client: uid=%u cmd=%u fd=%d", user_id, cmd, fdsess->fd);
		return -1;     
	}    

	if (completed) {
		wait_cmd = 0; 
	}    

	KTRACE_LOG(user_id ? user_id : 0, "cmd=%u", cmd);
	return 0;
}

int
Player::send_to_self_error(uint16_t cmd, int err, int completed)
{
	cli_proto_head_t proto = { };
	proto.user_id = user_id;
	proto.cmd = cmd;
	proto.len = sizeof(cli_proto_head_t);
	proto.ret = err;

	if (send_pkg_to_client(fdsess, &proto, sizeof(cli_proto_head_t)) == -1) {
		ERROR_LOG("FAILED TO ERR TO CLIENT [UID=%u] [CMD=%u]", user_id, cmd);
		return -1;
	}

	if (completed) {
		wait_cmd = 0;
	}

	KERROR_LOG(user_id, "ERR RETURN [CMD %u] [ERR CODE %d]", cmd, err);

	return 0;
}

int
Player::gen_session_key()
{
	const char *str = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";	
	char salt[64] = {0};
	uint32_t r = rand() % 62;
	for (uint32_t i = 0; i < r; i++) {
		salt[i] = str[rand() % 62];
	}

	char buf[1024] = {0};
	sprintf(buf, "%u%s", this->user_id, salt);

	session_key = murmur_hash2(buf, strlen(buf));

	return 0;
}

int
Player::update_session_key()
{
	session_key++;

	return 0;
}

int
Player::gen_session(char *session)
{
	utils::MD5 md5;

	const char *key = "Kingsoft";
	char buf[1024] = {0};
	sprintf(buf, "%u%s", session_key, key);

	md5.update(buf, strlen(buf));

	memcpy(session, md5.toString().c_str(), 32);

	return 0;
}

int
Player::send_login_info_to_client()
{	
	cli_proto_login_out cli_out;
	this->pack_player_login_info(cli_out);
	this->send_to_self(this->wait_cmd, &cli_out, 0);

	//物品和装备信息
	cli_get_items_info_out cli_out2;
	this->items_mgr->pack_all_items_info(cli_out2);
	this->equip_mgr->pack_all_equips_info(cli_out2.equips);
	this->send_to_self(cli_get_items_info_cmd, &cli_out2, 0); 
	
	//战魂信息
	cli_get_btl_soul_list_out cli_out3;
	this->btl_soul_mgr->pack_all_btl_soul_info(cli_out3);
	this->send_to_self(cli_get_btl_soul_list_cmd, &cli_out3, 0); 

	//英雄信息
	cli_get_heros_info_out cli_out4;
	this->hero_mgr->pack_all_heros_info(cli_out4);
	this->send_to_self(cli_get_heros_info_cmd, &cli_out4, 0);

	//小兵信息
	cli_get_soldiers_info_out cli_out5;
	this->soldier_mgr->pack_all_soldiers_info(cli_out5);
	this->send_to_self(cli_get_soldiers_info_cmd, &cli_out5, 0);

	//副本信息
	cli_get_instance_list_out cli_out6;
	this->instance_mgr->pack_instance_list_info(cli_out6);
	return this->send_to_self(cli_get_instance_list_cmd, &cli_out6, 1);

	//返回十连抽信息
	/*
	cli_get_ten_even_draw_info_out cli_out7;
	this->ten_even_draw_mgr->pack_ten_even_draw_info(cli_out7);
	return this->send_to_self(cli_get_ten_even_draw_info_cmd, &cli_out7, 1);
	*/
}

/********************************************************************************/
/*							PlayerManager Class									*/
/********************************************************************************/
PlayerManager::PlayerManager()
{
	uid_map.clear();
	fd_map.clear();
	acct_map.clear();
}

PlayerManager::~PlayerManager()
{

}

/* @brief 通过acct_id查找玩家
 */
Player*
PlayerManager::get_player_by_acct_id(uint32_t acct_id)
{
	PlayerAcctMap::iterator it = acct_map.find(acct_id);
	if (it != acct_map.end()) {
		return it->second;
	}

	return 0;
}

/* @brief 通过uid查找玩家
 */
Player*
PlayerManager::get_player_by_uid(uint32_t uid)
{
	PlayerUidMap::iterator it = uid_map.find(uid);
	if (it != uid_map.end()) {
		return it->second;
	}

	return 0;
}

/* @brief 通过fd查找玩家
 */
Player*
PlayerManager::get_player_by_fd(uint32_t fd)
{
	PlayerFdMap::iterator it = fd_map.find(fd);
	if (it != fd_map.end()) {
		return it->second;
	}

	return 0;
}

/* @brief 初始化player
 */
int
PlayerManager::init_player(uint32_t uid, Player *p)
{
	p->user_id = uid;

	uid_map.insert(PlayerUidMap::value_type(uid, p));
	acct_map.insert(PlayerAcctMap::value_type(p->account_id, p));

	return 0;
}

/* @brief 加入玩家 
 */
Player*
PlayerManager::add_player(uint32_t account_id, fdsession_t *fds, bool *cache_flag)
{
	Player *p;
	PlayerAcctMap::iterator it = acct_map.find(account_id);
	if (it != acct_map.end()) {
		p = it->second;
		if (p) {
			//如果之前连接的没断开，又有新的连接进来,即重复登录
			if (get_player_by_fd(p->fdsess->fd)) {
				fd_map.erase(p->fdsess->fd);
			}

			p->fdsess = fds;
			p->last_conn_tm = time(0);
			p->cur_login_tm = time(0);
			fd_map.insert(PlayerFdMap::value_type(fds->fd, p));
			uid_map.insert(PlayerUidMap::value_type(p->user_id, p));
			*cache_flag = true;
		}
	} else {
		p = new Player(account_id, fds);
		p->account_id = account_id;
		p->session_key = p->gen_session_key();
		p->last_conn_tm = time(0);

		fd_map.insert(PlayerFdMap::value_type(fds->fd, p));
		*cache_flag = false;
	}

	T_KDEBUG_LOG(p->user_id, "PLAYER ADD\t[addt_id=%u uid=%u fd=%d]", account_id, p->user_id, p->fdsess->fd);

	return p;
}

/* @brief 删除玩家
 */
void 
PlayerManager::del_player(Player *p)
{
	if (!p) {
		return;
	}
	//从fdmap中删除
	fd_map.erase(p->fdsess->fd);
	uid_map.erase(p->user_id);
	//设置登入登出时间
	p->set_player_last_login_tm();
	p->set_player_last_logout_tm();

	T_KDEBUG_LOG(p->user_id, "PLAYER DEL\t[uid=%u fd=%u]", p->user_id, p->fdsess->fd);

	//启动活跃定时检测
	//set_timeout(check_active_player, p, 0, 600 * 1000);

	//TODO 删除缓存
	acct_map.erase(p->account_id);
	SAFE_DELETE(p);

	return;
}

/* @brief 删除过期玩家缓存
 */
void 
PlayerManager::del_expire_player(Player *check_p)
{
	uint32_t cur_tm = get_now_tv()->tv_sec;
	PlayerAcctMap::iterator it = acct_map.begin();
	for (; it != acct_map.end(); ++it) {
		Player *p = it->second;
		if (p == check_p) {
			PlayerUidMap::iterator uid_it = uid_map.find(p->user_id);
			if (uid_it == uid_map.end() && p->last_logout_tm + 600 <= cur_tm) {//自上次断开连接后连续不在线2小时
				T_KDEBUG_LOG(p->user_id, "DELETE EXPIRE PLAYER\t[last_logout_tm=%u, cur_tm=%u]", p->last_logout_tm, cur_tm);
				acct_map.erase(it);
				SAFE_DELETE(p);
			}
			break;
		}
	}
}

/* @brief 删除所有玩家
 */
void
PlayerManager::del_all_player()
{
	PlayerUidMap::iterator it = uid_map.begin();
	while (it != uid_map.end()) {
		Player *p = it->second;
		++it;
		del_player(p);
	}

	return;
}

/********************************************************************************/
/*								RoleSkillXmlManager								*/
/********************************************************************************/
RoleSkillXmlManager::RoleSkillXmlManager()
{
	role_skill_xml_map.clear();
}

RoleSkillXmlManager::~RoleSkillXmlManager()
{

}

const role_skill_xml_info_t*
RoleSkillXmlManager::get_role_skill_xml_info(uint32_t id)
{
	RoleSkillXmlMap::iterator it = role_skill_xml_map.find(id);
	if (it != role_skill_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

const role_skill_xml_info_t*
RoleSkillXmlManager::get_role_skill_xml_info_by_skill(uint32_t skill_id)
{
	RoleSkillXmlMap::iterator it = role_skill_xml_map.begin();
	for (; it != role_skill_xml_map.end(); ++it) {
		const role_skill_xml_info_t *p_xml_info = &(it->second);
		if (p_xml_info->skill_id == skill_id) {
			return p_xml_info;
		}
	}

	return 0;
}

void
RoleSkillXmlManager::pack_unlock_role_skill(Player *p, vector<cli_role_skills_info_t> &skills)
{
	if (!p) {
		return;
	}
	RoleSkillXmlMap::iterator it = role_skill_xml_map.begin();
	for (; it != role_skill_xml_map.end(); ++it) {
		const role_skill_xml_info_t *p_xml_info = &(it->second);
		uint32_t res_type = forever_main_hero_skill_1_lv + p_xml_info->id - 1;
		uint32_t res_value = p->res_mgr->get_res_value(res_type);
		cli_role_skills_info_t info;
		info.skill_id = p_xml_info->skill_id;
		info.skill_lv = res_value;
		info.unlock_lv = p_xml_info->unlock_lv;
		info.state = p->get_role_skill_state(p_xml_info->skill_id);
		skills.push_back(info);
	}

}

int
RoleSkillXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_role_skill_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

int
RoleSkillXmlManager::load_role_skill_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("roleSkill"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			RoleSkillXmlMap::iterator it = role_skill_xml_map.find(id);
			if (it != role_skill_xml_map.end()) {
				ERROR_LOG("load role skill xml info err\t[id=%u]", id);
				return -1;
			}

			role_skill_xml_info_t info = {};
			info.id = id;
			get_xml_prop(info.skill_id, cur, "skill_id");
			get_xml_prop(info.unlock_lv, cur, "unlock_lv");

			TRACE_LOG("load role skill xml info\t[%u %u %u]", info.id, info.skill_id, info.unlock_lv);

			role_skill_xml_map.insert(RoleSkillXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 拉取阵容列表
 */
int cli_get_troop_list(Player *p, Cmessage *c_in)
{
	return send_msg_to_dbroute(p, db_get_troop_list_cmd, 0, p->user_id);
}

/* @brief 设置阵容列表
 */
int cli_set_troop_info(Player *p, Cmessage *c_in)
{
	cli_set_troop_info_in *p_in = P_IN;

	int ret = p->set_troop(p_in->troop_info);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "SET TROOP INFO");

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 完成新手引导
 */
int cli_complete_new_player_lead_task(Player *p, Cmessage *c_in)
{
	cli_complete_new_player_lead_task_in *p_in = P_IN;

	int ret = p->complete_new_player_lead_task(p_in->lead_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_complete_new_player_lead_task_out cli_out;
	cli_out.lead_id = p_in->lead_id;

	T_KDEBUG_LOG(p->user_id, "COMPLETE NEW PLAYER LEAD TASK\t[lead_id=%u]", p_in->lead_id);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 设置玩家昵称
 */
int cli_set_player_nick(Player *p, Cmessage *c_in)
{
	cli_set_player_nick_in *p_in = P_IN;

	if (!strlen(p_in->nick)) {
		return p->send_to_self_error(p->wait_cmd, cli_invalid_nick_err, 1);
	}

	//脏词检测
	p_in->nick[NICK_LEN - 1] = '\0';
	//CHECK_DIRTYWORD_ERR_RET(p, p_in->nick);

	//更新缓存
	memcpy(p->nick, p_in->nick, NICK_LEN);

	//更新DB
	db_set_player_nick_in db_in;
	memcpy(db_in.nick, p_in->nick, NICK_LEN);
	send_msg_to_dbroute(0, db_set_player_nick_cmd, &db_in, p->user_id);

	//更新redis
	redis_mgr->set_user_nick(p);

	cli_set_player_nick_out cli_out;
	memcpy(cli_out.nick, p_in->nick, NICK_LEN);

	T_KDEBUG_LOG(p->user_id, "SET PLAYER NICK\t[nick=%s]", p_in->nick);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 设置玩家性别
 */
int cli_set_player_sex(Player *p, Cmessage *c_in)
{
	cli_set_player_sex_in *p_in = P_IN;

	if (!p_in->sex || p_in->sex > 2) {
		T_KWARN_LOG(p->user_id, "set player sex err, sex=%u", p_in->sex);
		return p->send_to_self_error(p->wait_cmd, cli_player_sex_err, 1);
	}

	//更新缓存
	p->sex = p_in->sex;

	//更新DB
	db_set_player_sex_in db_in;
	db_in.sex = p_in->sex;
	send_msg_to_dbroute(0, db_set_player_sex_cmd, &db_in, p->user_id);

	T_KDEBUG_LOG(p->user_id, "SET PLAYER SEX\t[sex=%u]", p_in->sex);

	cli_set_player_sex_out cli_out;
	cli_out.sex = p_in->sex;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 购买道具
 */
int cli_buy_item(Player *p, Cmessage *c_in)
{
	cli_buy_item_in *p_in = P_IN;

	int ret = p->buy_item(p_in->type);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_buy_item_out cli_out;
	cli_out.type = p_in->type;
	cli_out.left_tms = p->get_buy_item_left_tms(p_in->type);

	T_KDEBUG_LOG(p->user_id, "BUY ITEM\t[type=%u]", p_in->type);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 角色使用道具
 */
int cli_use_items_for_role(Player *p, Cmessage *c_in)
{
	cli_use_items_for_role_in *p_in = P_IN;
	const item_xml_info_t *p_item_info = items_xml_mgr->get_item_xml_info(p_in->item_id);
	if (!p_item_info) {
		T_KWARN_LOG(p->user_id, "invalid item id\t[item_id=%u]", p_in->item_id);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_item_err, 1);
	}

	int ret = 0;
	if (p_item_info->type == em_item_type_for_energy) {//体力道具
		ret = p->eat_energy_items(p_in->item_id, p_in->item_cnt);
	} else if (p_item_info->type == em_item_type_for_endurance) {//耐力道具
		ret = p->eat_endurance_items(p_in->item_id, p_in->item_cnt);
	} else if (p_item_info->type == em_item_type_for_adventure) {//
		ret = p->eat_adventure_items(p_in->item_id, p_in->item_cnt);
	} else if (p_item_info->type == em_item_type_for_battle) {//战斗道具
		ret = p->use_battle_items(p_in->item_id, p_in->item_cnt);
	} else if (p_item_info->type == em_item_type_for_treasure_box) {//宝箱
		ret = p->items_mgr->open_treasure_box(p_in->item_id, p_in->item_cnt);
	} else if (p_item_info->type == em_item_type_for_random_gift) {//随机礼包
		ret = p->items_mgr->open_random_gift(p_in->item_id, p_in->item_cnt);
	} else {
		T_KWARN_LOG(p->user_id, "invalid item id\t[item_id=%u, item_type=%u]", p_in->item_id, p_item_info->type);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_item_err, 1);
	}	

	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_use_items_for_role_out cli_out;
	cli_out.item_id = p_in->item_id;
	cli_out.item_cnt = p_in->item_cnt;

	T_KDEBUG_LOG(p->user_id, "USE ITEMS FOR ROLE\t[item_id=%u, item_cnt=%u]", p_in->item_id, p_in->item_cnt);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 设置主公技能
 */
int cli_set_role_skills(Player *p, Cmessage *c_in)
{
	cli_set_role_skills_in *p_in = P_IN;

	int ret = p->set_role_skills(p_in->skills);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_set_role_skills_out cli_out;
	for (uint32_t i = 0; i < p_in->skills.size(); i++) {
		uint32_t skill_id = p_in->skills[i];
		cli_out.skills.push_back(skill_id);
	}

	T_KDEBUG_LOG(p->user_id, "SET ROLE SKILLS");

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/********************************************************************************/
/*								DB Return										*/
/********************************************************************************/
/* @brief 拉取阵容列表
 */
int db_get_troop_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);
	db_get_troop_list_out *p_in = P_IN;
	p->init_troop_list(p_in);

	if (p->wait_cmd == cli_proto_login_cmd) {
		p->login_step++;

		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u TROOPS INFO", p->login_step);

		cli_get_troop_list_out cli_out;
		p->pack_player_troop_list(cli_out);
		p->send_to_self(cli_get_troop_list_cmd, &cli_out, 0);

		//拉取副本信息
		return send_msg_to_dbroute(p, db_get_instance_list_cmd, 0, p->user_id);
	} else if (p->wait_cmd == cli_get_troop_list_cmd) {
		cli_get_troop_list_out cli_out;
		p->pack_player_troop_list(cli_out);
	
		return p->send_to_self(p->wait_cmd, &cli_out, 1);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}

/********************************************************************************/
/*								Lua Interface									*/
/********************************************************************************/
static int lua_send_to_self(lua_State *L)
{
	Player *p = (Player *)lua_touserdata(L, 1);
	Cmessage *c_out = (Cmessage *)lua_touserdata(L, 2);
	int completed = luaL_checkinteger(L, 3);

	if (p) {
		p->send_to_self(p->wait_cmd, c_out, completed);
	}

	return 0;
}

static int lua_send_to_self_error(lua_State *L)
{
	Player *p = (Player *)lua_touserdata(L, 1);
	int err = luaL_checkinteger(L, 2);
	
	if (p) {
		p->send_to_self_error(p->wait_cmd, err, 1);
	}

	return 0;
}

static int lua_add_role_exp(lua_State *L)
{
	Player *p = (Player *)lua_touserdata(L, 1);
	int add_value = luaL_checkinteger(L, 2);
	
	if (p) {
		p->add_role_exp(add_value);
	}

	return 0;
}

static int lua_chg_golds(lua_State *L)
{
	//Player *p = (Player *)lua_touserdata(L, 1);
	Player **p = (Player **)luaL_checkudata(L, 1, "Player");
	int chg_value = luaL_checkinteger(L, 2);
	
	if ((*p)) {
		(*p)->chg_golds(chg_value);
	}

	return 0;
}

static int lua_get_res_value(lua_State *L)
{
	Player **p = (Player **)luaL_checkudata(L, 1, "Player");
	uint32_t res_type = luaL_checkinteger(L, 2);

	uint32_t res_value = (*p)->res_mgr->get_res_value(res_type);
	lua_pushinteger(L, res_value);

	return 1;
}

static int lua_set_res_value(lua_State *L)
{
	Player **p = (Player **)luaL_checkudata(L, 1, "Player");
	uint32_t res_type = luaL_checkinteger(L, 2);
	uint32_t res_value = luaL_checkinteger(L, 3);

	(*p)->res_mgr->set_res_value(res_type, res_value);

	return 0;
}

static int lua_add_item(lua_State *L)
{
	Player **p = (Player **)luaL_checkudata(L, 1, "Player");
	uint32_t item_id = luaL_checkinteger(L, 2);
	uint32_t item_cnt = luaL_checkinteger(L, 3);

	(*p)->items_mgr->add_item_without_callback(item_id, item_cnt);

	return 0;
}

int luaopen_player(lua_State *L)
{
	luaL_checkversion(L);

	luaL_Reg l[] = {
#define RegFunc(func) {#func, lua_##func},
		RegFunc(send_to_self)
		RegFunc(send_to_self_error)
		RegFunc(add_role_exp)
		RegFunc(chg_golds)
		RegFunc(get_res_value)
		RegFunc(set_res_value)
		RegFunc(add_item)
#undef RegFunc
		{NULL, NULL},
	};

	if (luaL_newmetatable(L, PLAYERMETA)) {
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		luaL_setfuncs(L, l, 0);
	}
	//luaL_newlib(L, l);

	return 1;
}
