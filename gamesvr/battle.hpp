/*
 * =====================================================================================
 *
 *  @file  battle.hpp 
 *
 *  @brief  处理战斗相关逻辑
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#ifndef BATTLE_HPP_
#define BATTLE_HPP_

#include "common_def.hpp"

class Player;
class cli_instance_monster_info_t;
class cli_instance_battle_request_out;
class cli_item_info_t;

struct battle_cache_info_t {
	uint32_t user_id;
	uint32_t battle_id;
	uint32_t exp;
	uint32_t golds;
	uint32_t stat;
	std::vector<cli_item_info_t> rewards;
};
typedef std::map<uint32_t, battle_cache_info_t*> BattleCacheMap;
//extern BattleCacheMap g_battle_cache_map;

class Battle {
public:
	Battle(Player* p);
	~Battle();

	int init_battle();
	int battle_start(uint32_t battle_id);
	int battle_end();
	int cache_battle_info(uint32_t battle_id, uint32_t stat);
	battle_cache_info_t* get_battle_cache_info();
	int cache_instance_reward_info(uint32_t exp, uint32_t golds, std::vector<cli_item_info_t> &reward_vec);

	int pack_instance_monster_info(std::vector<cli_instance_monster_info_t> &out_vec, uint32_t instance_id);
	int pack_instance_reward_info(cli_instance_battle_request_out &out, uint32_t drop_id);

private:
	Player* owner;
	uint32_t stat;
};


#endif //BATTLE_HPP_
