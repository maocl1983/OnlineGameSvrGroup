/*
 * =====================================================================================
 *
 *  @file  astrology.cpp 
 *
 *  @brief  占星系统
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

#include "astrology.hpp"
#include "player.hpp"
#include "dbroute.hpp"
#include "utils.hpp"

using namespace std;
using namespace project;

#define MAX_LIGHT_TMS 16

/********************************************************************************/
/*							AstrologyManager Class								*/
/********************************************************************************/
AstrologyManager::AstrologyManager(Player *p) : owner(p)
{
}

AstrologyManager::~AstrologyManager()
{

}

void
AstrologyManager::init_astrology_info()
{
	if (!owner->res_mgr->get_res_value(daily_astrology_init_flag)) {
		owner->res_mgr->set_res_value(daily_astrology_init_flag, 1);
		for (int i = 0; i < 16; i++) {
			uint32_t r = rand() % 15 + 1;
			uint32_t res_type = daily_astrology_dst_star_1 + i;
			owner->res_mgr->set_res_value(res_type, r);
		}
		for (int i = 0; i < 5; i++) {
			uint32_t r = rand() % 15 + 1;
			uint32_t res_type = daily_astrology_select_star_1 + i;
			owner->res_mgr->set_res_value(res_type, r);
		}
	}
}

void
AstrologyManager::refresh_select_stars()
{
	for (int i = 0; i < 4; i++) {
		uint32_t r = rand() % 15 + 1;
		uint32_t res_type = daily_astrology_select_star_1 + i;
		owner->res_mgr->set_res_value(res_type, r);
	}
	uint32_t refresh_tms = owner->res_mgr->get_res_value(daily_astrology_refresh_tms);
	refresh_tms++;
	owner->res_mgr->set_res_value(daily_astrology_refresh_tms, refresh_tms);
}

int
AstrologyManager::get_refresh_price()
{
	uint32_t refresh_tms = owner->res_mgr->get_res_value(daily_astrology_refresh_tms);
	if (refresh_tms) {
		return 10;
	}

	return 0;
}

int
AstrologyManager::light_star(uint32_t dst_pos, uint32_t select_pos)
{
	if (dst_pos > 16 || !select_pos || select_pos > 5) {
		return cli_invalid_input_arg_err;
	}

	uint32_t light_tms = owner->res_mgr->get_res_value(daily_astrology_tms);
	if (light_tms >= MAX_LIGHT_TMS) {
		return cli_astrology_left_tms_not_enough_err;
	}

	if (!dst_pos) {//直接放弃
		refresh_select_stars();
	} else {
		uint32_t dst_res_type = daily_astrology_dst_star_1 + dst_pos - 1;
		uint32_t dst_star = owner->res_mgr->get_res_value(dst_res_type);
		uint32_t select_res_type = daily_astrology_select_star_1 + select_pos - 1;
		uint32_t select_star = owner->res_mgr->get_res_value(select_res_type);

		uint32_t star_stat = owner->res_mgr->get_res_value(daily_astrology_light_star_stat);
		if (test_bit_on(star_stat, dst_pos)) {
			T_KWARN_LOG(owner->user_id, "astrology star already lightted\t[dst_pos=%u]", dst_pos);
			return cli_astrology_star_already_lightted_err;
		}

		if (dst_star == select_star) {//点亮
			//设置状态
			star_stat = set_bit_on(star_stat, dst_pos);
			owner->res_mgr->set_res_value(daily_astrology_light_star_stat, star_stat);
			uint32_t light_num = utils_mgr.get_binary_one_cnt(star_stat);

			//增加星数
			uint32_t add_star = 1;
			if (light_num == 4) {
				add_star += 20;
			} else if (light_num == 8) {
				add_star += 30;
			} else if (light_num == 12) {
				add_star += 35;
			} else if (light_num == 16) {
				add_star += 40;
			}
			uint32_t star = owner->res_mgr->get_res_value(daily_astrology_star_num);
			star += add_star;
			owner->res_mgr->set_res_value(daily_astrology_star_num, star);

			//钻石奖励
			uint32_t add_diamond = 0;
			if (light_num == 1) {	
				add_diamond = 10;
			} else if (light_num == 2) {
				add_diamond = 30;
			} else if (light_num == 3) {
				add_diamond = 30;
			} else if (light_num == 4) {
				add_diamond = 60;
			}

			owner->chg_diamond(add_diamond);
		} else {
			refresh_select_stars();
		}
	}

	light_tms++;
	owner->res_mgr->set_res_value(daily_astrology_tms, light_tms);

	return 0;
}

int
AstrologyManager::get_all_reward()
{
	uint32_t reward_stat = owner->res_mgr->get_res_value(daily_astrology_reward_stat);
	uint32_t get_num = utils_mgr.get_binary_one_cnt(reward_stat);

	uint32_t need_star_num[10] = {15, 30, 40, 50, 60, 80, 90, 100, 115, 130};
	uint32_t star_num = owner->res_mgr->get_res_value(daily_astrology_star_num);
	uint32_t i = 0;
	for (; i < 10; i++) {
		if (star_num < need_star_num[i]) {
			break;
		}
	}
	if (i == 0) {
		return cli_astrology_reward_need_star_num_not_enough_err;
	}

	if (get_num >= i) {
		T_KWARN_LOG(owner->user_id, "already get astrology star reward\t[reward_id=%u]", i);
		return cli_astrology_reward_already_gotted_err;
	}

	get_star_reward(get_num + 1, i);

	return 0;
}

int
AstrologyManager::get_star_reward(uint32_t start_id, uint32_t end_id)
{
	if (!start_id || !end_id || start_id > end_id || end_id > 10) {
		return 0;
	}

	uint32_t reward_stat = owner->res_mgr->get_res_value(daily_astrology_reward_stat);

	cli_send_get_common_bonus_noti_out noti_out;
	for (uint32_t i = start_id; i <= end_id; i++) {
		if (test_bit_on(reward_stat, i)) {
			continue;
		}
		switch (i) {
		case 1://金币5000
			owner->chg_golds(5000);
			noti_out.golds += 5000;
			break;
		case 2://钻石10
			owner->chg_diamond(10);
			noti_out.diamond += 10;
			break;
		case 3://绿项链
			owner->items_mgr->add_reward(801004, 1);
			owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 801004, 1);
			break;
		case 4://金币10000
			owner->chg_golds(10000);
			noti_out.golds += 10000;
			break;
		case 5://钻石20
			owner->chg_diamond(20);
			noti_out.diamond += 20;
			break;
		case 6://小兵经验卡102001
			owner->items_mgr->add_item_without_callback(102001, 1);
			owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 102001, 1);
			break;
		case 7://金币20000
			owner->chg_golds(20000);
			noti_out.golds += 20000;
			break;
		case 8://蓝项链碎片532004
			owner->items_mgr->add_item_without_callback(532004, 1);
			owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 532004, 1);
			break;
		case 9://钻石30
			owner->chg_diamond(30);
			noti_out.diamond += 30;
			break;
		case 10://紫项链碎片533004
			owner->items_mgr->add_item_without_callback(533004, 1);
			owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 533004, 1);
			break;
		default:
			break;
		}
		//设置奖励状态
		reward_stat = set_bit_on(reward_stat, i);
	}

	owner->res_mgr->set_res_value(daily_astrology_reward_stat, reward_stat);

	owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	return 0;
}

void 
AstrologyManager::pack_astrology_reward_info(std::vector<cli_item_info_t> &vec)
{
	uint32_t items[10][2] = {{1, 5000}, {2, 10}, {801004, 1}, {1, 10000}, {2, 20}, {102001, 1}, {1, 20000}, {532004, 1}, {2, 30}, {533004, 1}};
	for (int i = 0; i < 10; i++) {
		cli_item_info_t info;
		info.item_id = items[i][0];
		info.item_cnt = items[i][1];
		vec.push_back(info);
	}
}

void
AstrologyManager::pack_client_astrology_info(cli_get_astrology_panel_info_out &out)
{
	out.star_stat = owner->res_mgr->get_res_value(daily_astrology_light_star_stat);
	out.star_num = owner->res_mgr->get_res_value(daily_astrology_star_num);
	out.astrology_tms = owner->res_mgr->get_res_value(daily_astrology_tms);
	out.reward_stat = owner->res_mgr->get_res_value(daily_astrology_reward_stat);
	out.refresh_price = get_refresh_price();
	for (int i = 0; i < 16; i++) {
		uint32_t res_type = daily_astrology_dst_star_1 + i;
		uint32_t res_value = owner->res_mgr->get_res_value(res_type);
		out.dst_stars.push_back(res_value);
	}
	for (int i = 0; i < 5; i++) {
		uint32_t res_type = daily_astrology_select_star_1 + i;
		uint32_t res_value = owner->res_mgr->get_res_value(res_type);
		out.select_stars.push_back(res_value);
	}
	pack_astrology_reward_info(out.rewards);
}


/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 拉取占星面板信息
 */
int cli_get_astrology_panel_info(Player *p, Cmessage *c_in)
{
	cli_get_astrology_panel_info_out cli_out;

	p->astrology_mgr->pack_client_astrology_info(cli_out);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 占星点亮星座
 */
int cli_astrology_light_star(Player *p, Cmessage *c_in)
{
	cli_astrology_light_star_in *p_in = P_IN;

	int ret = p->astrology_mgr->light_star(p_in->light_pos, p_in->select_pos);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}
	
	cli_astrology_light_star_out cli_out;
	cli_out.star_stat = p->res_mgr->get_res_value(daily_astrology_light_star_stat);
	cli_out.star_num = p->res_mgr->get_res_value(daily_astrology_star_num);
	cli_out.astrology_tms = p->res_mgr->get_res_value(daily_astrology_tms);
	for (int i = 0; i < 5; i++) {
		uint32_t res_type = daily_astrology_select_star_1 + i;
		uint32_t res_value = p->res_mgr->get_res_value(res_type);
		cli_out.select_stars.push_back(res_value);
	}

	T_KDEBUG_LOG(p->user_id, "ASTROLOGY LIGHT STAR\t[dst_pos=%u, select_pos=%u]", p_in->light_pos, p_in->select_pos);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 刷新可供选择的星座
 */
int cli_astrology_refresh_select_stars(Player *p, Cmessage *c_in)
{
	uint32_t price = p->astrology_mgr->get_refresh_price();

	if (p->diamond < price) {
		return p->send_to_self_error(p->wait_cmd, cli_not_enough_diamond_err, 1);
	}
	//扣除钻石
	if (price) {
		p->chg_diamond(-price);
	}

	//刷新
	p->astrology_mgr->refresh_select_stars();

	cli_astrology_refresh_select_stars_out cli_out;
	cli_out.refresh_price = p->astrology_mgr->get_refresh_price();
	for (int i = 0; i < 5; i++) {
		uint32_t res_type = daily_astrology_select_star_1 + i;
		uint32_t res_value = p->res_mgr->get_res_value(res_type);
		cli_out.select_stars.push_back(res_value);
	}

	T_KDEBUG_LOG(p->user_id, "ASTROLOGY REFRESH SELECT STARS");

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 领取占星奖励
 */
int cli_astrology_get_reward(Player *p, Cmessage *c_in)
{
	int ret = p->astrology_mgr->get_all_reward();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_astrology_get_reward_out cli_out;
	cli_out.reward_stat = p->res_mgr->get_res_value(daily_astrology_reward_stat);

	T_KDEBUG_LOG(p->user_id, "ASTROLOGY GET REWARD\t");

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}
