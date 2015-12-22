/*
 * =====================================================================================
 *
 *  @file trial_tower.hpp 
 *
 *  @brief  试练塔系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef TRIAL_TOWER_HPP_
#define TRIAL_TOWER_HPP_

#include "common_def.hpp"

class Player;
class cli_get_trial_tower_panel_info_out;

/********************************************************************************/
/*							TrialTowerManager Class								*/
/********************************************************************************/
class TrialTowerManager {
public:
	TrialTowerManager(Player *p);
	~TrialTowerManager();

	void update_trial_tower();
	int get_sweep_left_tm();
	int trial_tower_sweeping();
	int reset_trial_tower();
	int send_sweep_reward(uint32_t start_floor, uint32_t end_floor);

	int battle_request(uint32_t battle_floor);
	int battle_end(uint32_t battle_floor, uint32_t is_win);

	void pack_client_trial_tower_info(cli_get_trial_tower_panel_info_out &out);

private:
	Player *owner;
};

/********************************************************************************/
/*					TrialTowerRewardXmlManager Class							*/
/********************************************************************************/
struct trial_tower_reward_xml_info_t {
	uint32_t floor;
	uint32_t golds;
	uint32_t items[3][2];
};
typedef std::map<uint32_t, trial_tower_reward_xml_info_t> TrialTowerRewardXmlMap;

class TrialTowerRewardXmlManager {
public:
	TrialTowerRewardXmlManager();
	~TrialTowerRewardXmlManager();

	int read_from_xml(const char *filename);
	const trial_tower_reward_xml_info_t * get_trial_tower_reward_xml_info(uint32_t floor);

private:
	int load_trial_tower_reward_xml_info(xmlNodePtr cur);

private:
	TrialTowerRewardXmlMap trial_tower_reward_xml_map;
};
extern TrialTowerRewardXmlManager trial_tower_reward_xml_mgr;

#endif //TRIAL_TOWER_HPP_
