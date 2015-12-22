/*
 * =====================================================================================
 *
 *  @file  astrology.hpp 
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

#ifndef ASTROLOGY_HPP_
#define ASTROLOGY_HPP_

#include "common_def.hpp"

class Player;
class cli_get_astrology_panel_info_out;
class cli_item_info_t;

class AstrologyManager {
public:
	AstrologyManager(Player *p);
	~AstrologyManager();

	void init_astrology_info();

	void refresh_select_stars();
	int get_refresh_price();
	int light_star(uint32_t dst_pos, uint32_t select_pos);
	int get_star_reward(uint32_t start_id, uint32_t end_id);
	int get_all_reward();

	void pack_client_astrology_info(cli_get_astrology_panel_info_out &out);
	void pack_astrology_reward_info(std::vector<cli_item_info_t> &vec);

private:
	Player *owner;
};

#endif //ASTROLOGY_HPP_
