/*
 * =====================================================================================
 *
 *  @file  vip.hpp 
 *
 *  @brief  VIP系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef VIP_HPP_
#define VIP_HPP_

#include "common_def.hpp"

class Player;

enum vip_limit_type_t {
	em_vip_limit_start	= 0,
	em_vip_limit_energy					= 1,	/*! 体力丹 */
	em_vip_limit_endurance				= 2,	/*! 耐力丹 */
	em_vip_limit_hero_exp_item			= 3,	/*! 英雄经验道具 */
	em_vip_limit_soldier_exp_item		= 4,	/*! 兵种经验道具 */
	em_vip_limit_adventure				= 5,	/*！奇遇令 */
	em_vip_limit_complete_adventure		= 6,	/*! 完成奇遇次数 */
	em_vip_limit_hard_instance			= 7,	/*! 精英副本购买次数 */
	em_vip_limit_gem1					= 8,	/*! 1级宝石 */
	em_vip_limit_soldier_train_point	= 9,	/*! 兵种训练点 */
	em_vip_limit_internal_affairs		= 10,	/*! 内政 */
	em_vip_limit_arena					= 11,	/*! 竞技场挑战次数 */
	em_vip_limit_golds_item				= 12,	/*! 招财符 */
	em_vip_limit_iron_item				= 13,	/*! 陨铁 */
	em_vip_limit_refining_item			= 14,	/*! 精炼石 */
	em_vip_limit_clean_ticket			= 15,	/*! 扫荡券 */
	em_vip_limit_skill_point			= 16,	/*! 武将技能点 */
	em_vip_limit_100_cut_item			= 17,	/*! 百斩道具 */

	em_vip_limit_end
};

/********************************************************************************/
/*									VipManager									*/
/********************************************************************************/
class VipManager {
public:
	VipManager(Player *p);
	~VipManager();

	int buy_daily_gift(uint32_t gift_id);

private:
	Player *owner;
};

/********************************************************************************/
/*								VipXmlManager									*/
/********************************************************************************/
struct vip_xml_info_t {
	uint32_t type;
	uint32_t vip[15];
};
typedef std::map<uint32_t, vip_xml_info_t> VipXmlMap;

class VipXmlManager {
public:
	VipXmlManager();
	~VipXmlManager();

	int read_from_xml(const char *filename);
	const vip_xml_info_t* get_vip_xml_info(uint32_t type);
	int get_vip_max_limit(uint32_t type, uint32_t vip_lv);

private:
	int load_vip_xml_info(xmlNodePtr cur);

private:
	VipXmlMap vip_xml_map;
};
//extern VipXmlManager vip_xml_mgr;


#endif //VIP_HPP_

