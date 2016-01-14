/*
 * =====================================================================================
 *
 *  @file  guild.hpp 
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

#ifndef GUILD_HPP_
#define GUILD_HPP_

#include "common_def.hpp"

#define GUILD_DECLARATION_LEN 128

class Player;
class db_get_guild_list_out;
class db_get_guild_member_list_out;
class cli_get_guild_list_out;
class cli_get_player_guild_info_out;
class cli_get_guild_hire_heros_info_out;

/********************************************************************************/
/*									Guild Class									*/
/********************************************************************************/
struct guild_member_hero_info_t {
	uint32_t hero_id;
	uint32_t hero_lv;
	uint32_t hero_tm;
};

struct guild_member_info_t {
	uint32_t user_id;
	char nick[NICK_LEN];
	uint32_t guild_id;
	uint32_t role_lv;
	uint32_t title;
	uint32_t contribution;
	uint32_t worship_user1;
	uint32_t worship_user2;
	guild_member_hero_info_t heros[2];
};
typedef std::map<uint32_t, guild_member_info_t> GuildMemberMap;

struct guild_info_t {
	uint32_t guild_id;
	char guild_name[NICK_LEN];
	uint32_t create_tm;
	uint32_t lv;
	uint32_t owner_uid;
	char owner_nick[NICK_LEN];
	char declaration[GUILD_DECLARATION_LEN];
	GuildMemberMap members;
};
typedef std::map<uint32_t, guild_info_t> GuildMap;

class GuildManager {
public:
	GuildManager();
	~GuildManager();

	int get_guild_count();
	void first_init_guild();

	void init_guild_list(db_get_guild_list_out *p_in);
	void init_guild_member_list(db_get_guild_member_list_out *p_in);

	const guild_info_t * get_guild_info(uint32_t guild_id);
	const guild_member_info_t * get_guild_member_info(uint32_t guild_id, uint32_t user_id);
	int join_guild(Player *p, uint32_t guild_id);

	int update_hire_hero(uint32_t guild_id, uint32_t user_id, uint32_t id, uint32_t hero_id, uint32_t hero_lv, uint32_t hero_tm);
	int add_guild_hire_hero(Player *p, uint32_t id, uint32_t hero_id);
	int revoke_guild_hire_hero(Player *p, uint32_t hero_id, uint32_t &golds);

	void pack_client_guild_list(cli_get_guild_list_out &out);
	void pack_player_guild_info(Player *p, cli_get_player_guild_info_out &out);
	void pack_player_guild_hire_heros_info(Player *p, cli_get_guild_hire_heros_info_out &out);

private:
	GuildMap guild_map;
};
//extern GuildManager guild_mgr;

#endif //GUILD_HPP_
