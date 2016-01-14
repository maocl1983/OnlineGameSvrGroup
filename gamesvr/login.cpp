/*
 * =====================================================================================
 *
 *  @file  login.cpp
 *
 *  @brief  处理登入相关的信息
 *
 *  compiler  gcc4.4.7 
 *	
 *  platform  Linux
 *
 * copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"
#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"
#include "./proto/account_db.hpp"
#include "./proto/account_db_enum.hpp"

#include "global_data.hpp"
#include "login.hpp"
#include "player.hpp"
#include "dbroute.hpp"
#include "stat_log.hpp"
#include "log_thread.hpp"
#include "item.hpp"
#include "hero.hpp"
#include "instance.hpp"

/*
static int
send_login_info_step_by_step(Player *p)
{
	if (!p) {
		return 0;
	}

	//人物基础信息
	cli_proto_login_out cli_login_out;
	p->pack_player_login_info(cli_login_out);
	p->send_to_self(p->wait_cmd, &cli_login_out, 0);

	//物品信息
	cli_get_items_info_out cli_item_out;
	p->items_mgr->pack_all_items_info(cli_item_out);
	p->equip_mgr->pack_all_equips_info(cli_item_out.equips);
	p->send_to_self(cli_get_items_info_cmd, &cli_item_out, 0); 

	//英雄信息
	cli_get_heros_info_out cli_hero_out;
	p->hero_mgr->pack_all_heros_info(cli_hero_out);
	p->send_to_self(cli_get_heros_info_cmd, &cli_hero_out, 0);

	//副本信息
	cli_get_instance_list_out cli_instance_out;
	p->instance_mgr->pack_instance_list_info(cli_instance_out);
	return p->send_to_self(cli_get_instance_list_cmd, &cli_instance_out, 1); 
}*/

/************************************************************************/
/*                       Client  Request                                */
/************************************************************************/

/* @brief 登录命令
 */
int cli_proto_login(Player *p, Cmessage *c_in)
{
	cli_proto_login_in * p_in = P_IN;

	bool cache_flag = false;
	Player *new_player = g_player_mgr->add_player(p_in->account_id, p->fdsess, &cache_flag);
	new_player->wait_cmd = cli_proto_login_cmd;

	/* TODO
	if (cache_flag) {//有缓存
		return send_login_info_step_by_step(new_player);
	}*/

	db_get_player_login_info_in db_in;
	db_in.account_id = p_in->account_id;

	return send_msg_to_dbroute(new_player, db_get_player_login_info_cmd, &db_in, p_in->account_id);

	/*加入玩家信息*/
	/*
	Player* new_player = g_player_mgr->add_player(p->user_id, p->fdsess);
	new_player->wait_cmd = cli_proto_login_cmd;

	if (cur_version != p_in->version) {
		char login_ip[16]={0};
		uint32_t cli_ip = get_client_ip(p->fdsess);
		inet_ntop(AF_INET, (void *)&cli_ip, login_ip, 16);
		KERROR_LOG(new_player->user_id, "version error[%u %u],ip=[%s]", cur_version, p_in->version, login_ip);
		return p->send_to_self_error(p->wait_cmd, cli_invalid_login_version_err, 1);
	}

	//check session
	db_check_session_in db_in;
	db_in.keep_flag = 0;
	memcpy(db_in.session, p_in->session, 16);
	return send_msg_to_dbroute(new_player, db_check_session_cmd, &db_in, new_player->user_id);
	*/
}

/* @brief 拉取用户登录session 
 */
int cli_get_login_session(Player *p, Cmessage *c_in)
{
	return send_msg_to_dbroute(p, db_get_session_cmd, 0, p->user_id);
}

/************************************************************************/
/*                       dbser return   Request                         */
/************************************************************************/

/* @brief 取得用户登录信息
 */
int db_get_player_login_info(Player *p, Cmessage *c_in, uint32_t ret)
{
	if (ret) {
		return 0;
	}

	db_get_player_login_info_out *p_in = P_IN;
	g_player_mgr->init_player(p_in->user_id, p);
	p->init_player_login_info(p_in);

	p->login_step++;

	T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u GET LOGIN INFO", p->login_step);

	//拉取限制号信息
	return send_msg_to_dbroute(p, db_get_res_value_info_cmd, 0, p->user_id);
}

/* @brief 检查session
 */
int db_check_session(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p,ret);
	
	return send_msg_to_dbroute(p, db_get_player_login_info_cmd, NULL, p->user_id);
}


/*
int db_get_session(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p,ret);

	db_get_session_out *p_in = P_IN;

	cli_get_login_session_out cli_out;
	memcpy(cli_out.session, p_in->session, 16);
	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}*/
