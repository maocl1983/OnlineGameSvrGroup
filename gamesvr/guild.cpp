/*
 * =====================================================================================
 *
 *  @file  guild.cpp 
 *
 *  @brief  公会系统
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
#include "guild.hpp"
#include "player.hpp"
#include "dbroute.hpp"
*/

#include "stdafx.hpp"
using namespace std;
using namespace project;

//GuildManager guild_mgr;

/********************************************************************************/
/*									GuildManager Class									*/
/********************************************************************************/
GuildManager::GuildManager()
{
	guild_map.clear();
}

GuildManager::~GuildManager()
{
	guild_map.clear();
}

int
GuildManager::get_guild_count()
{
	return send_msg_to_dbroute(0, db_get_guild_count_cmd, 0, 0);
}

void
GuildManager::first_init_guild()
{
	char guild_name[3][NICK_LEN] = {"我来征服世界", "我是兔子", "我最厉害"};
	char nick[3][NICK_LEN] = {"Mark", "Rock", "Frankie"};
	char declaration[3][GUILD_DECLARATION_LEN] = {"life is beautiful!", "too young too simple!", "I have a dream!"};
	uint32_t now_sec = time(0);

	for (int i = 0; i < 3; i++) {
		//更新缓存
		guild_info_t info = {};
		info.guild_id = i + 1;
		memcpy(info.guild_name, guild_name[i], NICK_LEN);
		info.create_tm = now_sec + i;
		info.owner_uid = i + 1;
		memcpy(info.owner_nick, nick[i], NICK_LEN);
		info.lv = 1;
		memcpy(info.declaration, declaration[i], GUILD_DECLARATION_LEN);

		guild_member_info_t member_info = {};
		member_info.user_id = i + 1;
		memcpy(member_info.nick, nick[i], NICK_LEN);
		member_info.guild_id = i + 1;
		member_info.title = 1;
		member_info.role_lv = 40;
		info.members.insert(GuildMemberMap::value_type(member_info.user_id, member_info));

		//更新DB
		db_add_guild_in db_in;
		db_in.guild_info.guild_id = i + 1;
		memcpy(db_in.guild_info.guild_name, guild_name[i], NICK_LEN);
		db_in.guild_info.create_tm = now_sec + i;
		db_in.guild_info.owner_uid = i + 1;
		memcpy(db_in.guild_info.owner_nick, nick[i], NICK_LEN);
		db_in.guild_info.lv = 1;
		memcpy(db_in.guild_info.declaration, declaration[i], GUILD_DECLARATION_LEN);
		send_msg_to_dbroute(0, db_add_guild_cmd, &db_in, 0);

		db_add_guild_member_in db_in2;
		db_in2.member_info.user_id = i + 1;
		memcpy(db_in2.member_info.nick, nick[i], NICK_LEN);
		db_in2.member_info.guild_id = i + 1;
		db_in2.member_info.title = 1;
		db_in2.member_info.role_lv = 40;
		send_msg_to_dbroute(0, db_add_guild_member_cmd, &db_in2, 0);
	}

	DEBUG_LOG("FIRST INIT GUILD");
}

void
GuildManager::init_guild_list(db_get_guild_list_out *p_in)
{
	for (uint32_t i = 0; i < p_in->guild_list.size(); i++) {
		const db_guild_info_t *p_info = &(p_in->guild_list[i]);
		guild_info_t info = {};
		info.guild_id = p_info->guild_id;
		memcpy(info.guild_name, p_info->guild_name, NICK_LEN);
		info.create_tm = p_info->create_tm;
		info.lv = p_info->lv;
		info.owner_uid = p_info->owner_uid;
		memcpy(info.owner_nick, p_info->owner_nick, NICK_LEN);
		memcpy(info.declaration, p_info->declaration, 64);

		guild_map.insert(GuildMap::value_type(info.guild_id, info));

		TRACE_LOG("init guild list\t[%u '%s' %u %u %u '%s' '%s']", 
				info.guild_id, info.guild_name, info.create_tm, info.lv, info.owner_uid, info.owner_nick, info.declaration);
	}
}

void
GuildManager::init_guild_member_list(db_get_guild_member_list_out *p_in)
{
	for (uint32_t i = 0; i < p_in->guild_members.size(); i++) {
		const db_guild_member_info_t *p_info = &(p_in->guild_members[i]);
		guild_member_info_t info = {};
		info.user_id = p_info->user_id;
		memcpy(info.nick, p_info->nick, NICK_LEN);
		info.guild_id = p_info->guild_id;
		info.title = p_info->title;
		info.contribution = p_info->contribution;
		info.worship_user1 = p_info->worship_user1;
		info.worship_user2 = p_info->worship_user2;
		for (int j = 0; j < 2; j++) {
			info.heros[i].hero_id = p_info->heros[i].hero_id;
			info.heros[i].hero_lv = p_info->heros[i].hero_lv;
			info.heros[i].hero_tm = p_info->heros[i].hero_tm;
		}

		GuildMap::iterator it = guild_map.find(info.guild_id);
		if (it == guild_map.end()) {
			continue;
		} else {
			it->second.members.insert(GuildMemberMap::value_type(info.user_id, info));
		}

		TRACE_LOG("init guild member list\t[%u '%s' %u %u %u %u %u %u %u %u %u %u %u]", 
				info.user_id, info.nick, info.guild_id, info.title, info.contribution, info.worship_user1, info.worship_user2, 
				info.heros[0].hero_id, info.heros[0].hero_lv, info.heros[0].hero_tm, info.heros[1].hero_id, info.heros[1].hero_lv, info.heros[1].hero_tm);
	}
}

const guild_info_t *
GuildManager::get_guild_info(uint32_t guild_id)
{
	GuildMap::iterator it = guild_map.find(guild_id);
	if (it != guild_map.end()) {
		return &(it->second);
	}

	return 0;
}

const guild_member_info_t *
GuildManager::get_guild_member_info(uint32_t guild_id, uint32_t user_id)
{
	const guild_info_t *p_info = get_guild_info(guild_id);
	if (!p_info) {
		return 0;
	}

	GuildMemberMap::const_iterator it = p_info->members.find(user_id);
	if (it == p_info->members.end()) {
		return 0;
	}

	return &(it->second);
}

int
GuildManager::join_guild(Player *p, uint32_t guild_id)
{
	GuildMap::iterator it = guild_map.find(guild_id);
	if (it == guild_map.end()) {
		T_KWARN_LOG(p->user_id, "join guild err, guild not exist\t[guild_id=%u]", guild_id);
		return cli_guild_not_exist_err;
	}
	guild_info_t *p_info = &(it->second);
	if (p->guild_id) {
		return cli_player_already_in_guild_err;
	}

	uint32_t size = p_info->members.size();
	if (size >= 20) {
		return cli_guild_member_is_full_err;
	}
	
	//更新缓存
	guild_member_info_t info = {};
	info.user_id = p->user_id;
	memcpy(info.nick, p->nick, NICK_LEN);
	info.guild_id = guild_id;
	info.role_lv = p->lv;
	info.title = 0;
	info.contribution = 0;
	info.worship_user1 = 0;
	info.worship_user2 =0;
	p_info->members.insert(GuildMemberMap::value_type(p->user_id, info));

	//更新DB
	db_add_guild_member_in db_in;
	db_in.member_info.user_id = p->user_id;
	memcpy(db_in.member_info.nick, p->nick, NICK_LEN);
	db_in.member_info.guild_id = guild_id;
	db_in.member_info.role_lv = p->lv;
	send_msg_to_dbroute(0, db_add_guild_member_cmd, &db_in, p->user_id);

	//设置玩家公会ID
	p->guild_id = guild_id;
	db_set_player_guild_in db_in2;
	db_in2.guild_id = guild_id;
	send_msg_to_dbroute(0, db_set_player_guild_cmd, &db_in2, p->user_id);

	return 0;
}

int
GuildManager::update_hire_hero(uint32_t guild_id, uint32_t user_id, uint32_t id, uint32_t hero_id, uint32_t hero_lv, uint32_t hero_tm)
{
	if (!id || id > 2) {
		return 0;
	}

	GuildMap::iterator it = guild_map.find(guild_id);
	if (it == guild_map.end()) {
		return 0;
	}
	GuildMemberMap::iterator it2 = it->second.members.find(user_id);
	if (it2 == it->second.members.end()) {
		return 0;
	}

	it2->second.heros[id - 1].hero_id = hero_id;
	it2->second.heros[id - 1].hero_lv = hero_lv;
	it2->second.heros[id - 1].hero_tm = hero_tm;

	//更新DB
	db_set_guild_member_hero_in db_in;
	db_in.id = id;
	db_in.hero_id = hero_id;
	db_in.hero_lv = hero_lv;
	db_in.hero_tm = hero_tm;
	send_msg_to_dbroute(0, db_set_guild_member_hero_cmd, &db_in, user_id);

	T_KDEBUG_LOG(user_id, "UPDATE HIRE HERO\t[%u %u %u %u]", id, hero_id, hero_lv, hero_tm);

	return 0;
}

int
GuildManager::add_guild_hire_hero(Player *p, uint32_t id, uint32_t hero_id)
{
	if (!p || !id || id > 2) {
		return cli_invalid_input_arg_err;
	}

	Hero *p_hero = p->hero_mgr->get_hero(hero_id);
	if (!p_hero) {
		T_KWARN_LOG(p->user_id, "add guild hire hero err, hero not exist\t[hero_id=%u]", hero_id);
		return cli_hero_not_exist_err;
	}

	if (!p->guild_id) {
		return cli_player_not_join_guild_err;
	}

	const guild_member_info_t *p_info = get_guild_member_info(p->guild_id, p->user_id);
	if (!p_info) {
		return cli_player_not_join_guild_err;
	}

	if (p_info->heros[id - 1].hero_id) {
		T_KWARN_LOG(p->user_id, "guild hire hero already exists\t[cur_hero_id=%u, hero_id=%u]", p_info->heros[id - 1].hero_id, hero_id);
		return cli_guild_hire_hero_already_setted_err;
	}

	for (int i = 0; i < 2; i++) {
		if (p_info->heros[i].hero_id == hero_id) {
			T_KWARN_LOG(p->user_id, "guild hire hero already exists\t[hero_id=%u]", hero_id);
			return cli_guild_hire_hero_already_setted_err;
		}
	}

	//设置雇佣英雄
	uint32_t now_sec = get_now_tv()->tv_sec;
	update_hire_hero(p->guild_id, p->user_id, id, hero_id, p_hero->lv, now_sec);

	return 0;
}

int
GuildManager::revoke_guild_hire_hero(Player *p, uint32_t hero_id, uint32_t &golds)
{
	if (!p) {
		return cli_invalid_input_arg_err;
	}

	Hero *p_hero = p->hero_mgr->get_hero(hero_id);
	if (!p_hero) {
		T_KWARN_LOG(p->user_id, "revoke guild hire hero err, hero not exist\t[hero_id=%u]", hero_id);
		return cli_hero_not_exist_err;
	}

	if (!p->guild_id) {
		return cli_player_not_join_guild_err;
	}

	const guild_member_info_t *p_info = get_guild_member_info(p->guild_id, p->user_id);
	if (!p_info) {
		return cli_player_not_join_guild_err;
	}

	uint32_t id = 0;
	uint32_t add_tm = 0;
	for (int i = 0; i < 2; i++) {
		if (p_info->heros[i].hero_id == hero_id) {
			id = i + 1;
			add_tm = p_info->heros[i].hero_tm;
			break;
		}
	}

	if (!id) {
		T_KWARN_LOG(p->user_id, "revoke guild hire hero err, not hire hero\t[hero_id=%u]", hero_id);
		return cli_hero_not_exist_err;
	}

	//更新雇佣信息
	update_hire_hero(p->guild_id, p->user_id, id, 0, 0, 0);

	uint32_t now_sec = get_now_tv()->tv_sec;
	uint32_t diff_tm = add_tm < now_sec ? now_sec - add_tm : 0;
	uint32_t add_hour = diff_tm / 3600;
	golds = 5000 * add_hour;

	p->chg_golds(golds);

	T_KDEBUG_LOG(p->user_id, "REVOKE GUILD HIRE HERO\t[hero_id=%u, golds=%u]", hero_id, golds);

	return 0;
}

void
GuildManager::pack_client_guild_list(cli_get_guild_list_out &out)
{
	GuildMap::iterator it = guild_map.begin();
	for (; it != guild_map.end(); ++it) {
		const guild_info_t *p_info = &(it->second);
		cli_guild_info_t info;
		info.guild_id = p_info->guild_id;
		memcpy(info.guild_name, p_info->guild_name, NICK_LEN);
		info.create_tm = p_info->create_tm;
		info.owner_uid = p_info->owner_uid;
		memcpy(info.owner_nick, p_info->owner_nick, NICK_LEN);
		info.lv = p_info->lv;
		info.count = p_info->members.size();
		memcpy(info.declaration, p_info->declaration, GUILD_DECLARATION_LEN);

		out.guild_list.push_back(info);

		TRACE_LOG("pack client guild list\t[%u '%s' %u %u '%s' %u %u '%s']", 
				info.guild_id, info.guild_name, info.create_tm, info.owner_uid, info.owner_nick, info.lv, info.count, info.declaration);
	}
}

void
GuildManager::pack_player_guild_info(Player *p, cli_get_player_guild_info_out &out)
{
	const guild_info_t *p_info = get_guild_info(p->guild_id);
	if (!p_info) {
		return;
	}

	out.guild_info.guild_id = p->guild_id;
	memcpy(out.guild_info.guild_name, p_info->guild_name, NICK_LEN);
	out.guild_info.create_tm = p_info->create_tm;
	out.guild_info.owner_uid = p_info->owner_uid;
	memcpy(out.guild_info.owner_nick, p_info->owner_nick, NICK_LEN);
	out.guild_info.lv = p_info->lv;
	out.guild_info.count = p_info->members.size();
	memcpy(out.guild_info.declaration, p_info->declaration, GUILD_DECLARATION_LEN);

	GuildMemberMap::const_iterator it = p_info->members.begin();
	for (; it != p_info->members.end(); ++it) {
		const guild_member_info_t *p_member_info = &(it->second);
		cli_guild_member_info_t info;
		info.user_id = p_member_info->user_id;
		memcpy(info.nick, p_member_info->nick, NICK_LEN);
		info.role_lv = p_member_info->role_lv;
		info.title = p_member_info->title; 
		if (info.user_id == p->user_id || info.user_id == p_member_info->worship_user1 || info.user_id == p_member_info->worship_user2) {
			info.is_worship = 0;
		} else {
			info.is_worship = 1;
		}
		out.members.push_back(info);
	}

	T_KTRACE_LOG(p->user_id, "pack player guild info\t[%u '%s' %u %u '%s' %u %u '%s']", 
			out.guild_info.guild_id, out.guild_info.guild_name, out.guild_info.create_tm, out.guild_info.owner_uid, out.guild_info.owner_nick, 
			out.guild_info.lv, out.guild_info.count, out.guild_info.declaration);
}

void
GuildManager::pack_player_guild_hire_heros_info(Player *p, cli_get_guild_hire_heros_info_out &out)
{
	const guild_info_t *p_info = get_guild_info(p->guild_id);
	if (!p_info) {
		return;
	}
	//uint32_t now_sec = get_now_tv()->tv_sec;
	GuildMemberMap::const_iterator it = p_info->members.begin();
	/*for (; it != p_info->members.end(); ++it) {
		const guild_member_info_t *p_member_info = &(it->second);
		cli_guild_member_hero_info_t info;
		info.user_id = p_member_info->user_id;
		memcpy(info.nick, p_member_info->nick, NICK_LEN);
		for (int i = 0; i < 2; i++) {
			if (p_member_info->heros[i].hero_id) {
				common_guild_member_hero_info_t hero_info;
				hero_info.hero_id = p_member_info->heros[i].hero_id;
				hero_info.hero_lv = p_member_info->heros[i].hero_lv;
				hero_info.hero_tm = p_member_info->heros[i].hero_tm < now_sec ? now_sec - p_member_info->heros[i].hero_tm : 0;
				info.heros.push_back(hero_info);
			}
		}
		out.hire_heros.push_back(info);
		if (p_member_info->user_id == p->user_id) {
			out.self_heros.user_id = p->user_id;
			memcpy(out.self_heros.nick, p->nick, NICK_LEN);
			for (int i = 0; i < 2; i++) {
				if (p_member_info->heros[i].hero_id) {
					common_guild_member_hero_info_t hero_info;
					hero_info.hero_id = p_member_info->heros[i].hero_id;
					hero_info.hero_lv = p_member_info->heros[i].hero_lv;
					hero_info.hero_tm = p_member_info->heros[i].hero_tm < now_sec ? now_sec - p_member_info->heros[i].hero_tm : 0;
					out.self_heros.heros.push_back(hero_info);
				}
			}
		}

	}*/

}

/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 拉取公会列表
 */
int cli_get_guild_list(Player *p, Cmessage *c_in)
{
	cli_get_guild_list_out cli_out;
	guild_mgr->pack_client_guild_list(cli_out);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 申请加入公会
 */
int cli_join_guild_request(Player *p, Cmessage *c_in)
{
	cli_join_guild_request_in *p_in = P_IN;

	int ret = guild_mgr->join_guild(p, p_in->guild_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "JOIN GUILD REQUEST\[guild_id=%u]", p_in->guild_id);

	cli_join_guild_request_out cli_out;
	cli_out.guild_id = p_in->guild_id;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 拉取玩家公会信息
 */
int cli_get_player_guild_info(Player *p, Cmessage *c_in)
{
	cli_get_player_guild_info_out cli_out;
	guild_mgr->pack_player_guild_info(p, cli_out);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 拉取玩家公会佣兵信息
 */
int cli_get_guild_hire_heros_info(Player *p, Cmessage *c_in)
{
	cli_get_guild_hire_heros_info_out cli_out;
	guild_mgr->pack_player_guild_hire_heros_info(p, cli_out);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 设置公会佣兵
 */
int cli_set_guild_hire_hero(Player *p, Cmessage *c_in)
{
	cli_set_guild_hire_hero_in *p_in = P_IN;

	int ret = guild_mgr->add_guild_hire_hero(p, p_in->id, p_in->hero_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_set_guild_hire_hero_out cli_out;
	cli_out.id = p_in->id;
	cli_out.hero_id = p_in->hero_id;

	T_KDEBUG_LOG(p->user_id, "SET GUILD HIRE HERO\t[id=%u hero_id=%u]", p_in->id, p_in->hero_id);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 收回公会佣兵
 */
int cli_revoke_guild_hire_hero(Player *p, Cmessage *c_in)
{
	cli_revoke_guild_hire_hero_in *p_in = P_IN;

	uint32_t golds = 0;
	int ret = guild_mgr->revoke_guild_hire_hero(p, p_in->hero_id, golds);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_revoke_guild_hire_hero_out cli_out;
	cli_out.hero_id = p_in->hero_id;
	cli_out.golds = golds;

	T_KDEBUG_LOG(p->user_id, "REVOKE GUILD HIRE HERO\t[hero_id=%u, golds=%u]", p_in->hero_id, golds);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/********************************************************************************/
/*									DB Return									*/
/********************************************************************************/
/* @brief 拉取公会列表
 */
int db_get_guild_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	if (ret) {
		return 0;
	}
	db_get_guild_list_out *p_in = P_IN;

	guild_mgr->init_guild_list(p_in);

	return send_msg_to_dbroute(0, db_get_guild_member_list_cmd, 0, 0);
}

/* @brief 拉取公会成员列表
 */
int db_get_guild_member_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	if (ret) {
		return 0;
	}

	db_get_guild_member_list_out *p_in = P_IN;

	guild_mgr->init_guild_member_list(p_in);

	return 0;
}

/* @brief 拉取公会个数
 */
int db_get_guild_count(Player *p, Cmessage *c_in, uint32_t ret)
{
	if (ret) {
		return 0;
	}

	db_get_guild_count_out *p_in = P_IN;
	if (p_in->count == 0) {
		guild_mgr->first_init_guild();
	} else {
		return send_msg_to_dbroute(0, db_get_guild_list_cmd, 0, 0);
	}

	return 0;
}
