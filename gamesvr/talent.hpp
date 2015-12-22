/*
 * =====================================================================================
 *
 *  @file  talent.hpp 
 *
 *  @brief  处理天赋相关逻辑
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 * copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#ifndef TALENT_HPP_
#define TALENT_HPP_

#include "common_def.hpp"

/* @brief 被动型buff类型
 */
enum passive_buff_type_t {
	passive_buff_start			= 0,
	passive_buff_maxhp			= 1,	/*! 血量上限 */
	passive_buff_hp_regain		= 2,	/*! 回血 */
	passive_buff_atk_spd		= 3,	/*! 攻速 */
	passive_buff_ad				= 4,	/*! 攻击 */
	passive_buff_armor			= 5,	/*! 护甲 */
	passive_buff_resist			= 6,	/*! 魔抗 */
	passive_buff_all_def		= 7,	/*! 双防 */
	passive_buff_ad_cri			= 8,	/*! 物暴 */
	passive_buff_ad_ren			= 9,	/*! 物韧 */
	passive_buff_ad_chuan		= 10,	/*! 物穿 */
	passive_buff_ap_chuan		= 11,	/*! 法穿 */
	passive_buff_hit			= 12,	/*! 命中 */
	passive_buff_miss			= 13,	/*! 闪避 */
	passive_buff_cri_damage		= 14,	/*! 暴伤 */
	passive_buff_cri_avoid		= 15,	/*! 抗暴 */
	passive_buff_hp_steal		= 16,	/*! 吸血 */	
	passive_buff_ad_avoid		= 17,	/*! 物理免伤 */	
	passive_buff_ap_avoid		= 18,	/*! 法术免伤 */	

	passive_buff_end
};

/********************************************************************************/
/*							HeroTalentXmlManager								*/
/********************************************************************************/

/* @brief 天赋配置信息
 */
struct hero_talent_xml_info_t {
	uint32_t id;				/*! 天赋id */
	uint32_t type;				/*! 天赋类型 */
	uint32_t passive_class;		/*! 被动buff增益方向 */
	uint32_t passive_type;		/*! 被动buff数值类型 */
	uint32_t passive_value;		/*! 被动buff数值 */
};
typedef std::map<uint32_t, hero_talent_xml_info_t> HeroTalentXmlMap;

class HeroTalentXmlManager {
public:
	HeroTalentXmlManager();
	~HeroTalentXmlManager();

	int read_from_xml(const char *filename);
	const hero_talent_xml_info_t* get_hero_talent_xml_info(uint32_t talent_id);

private:
	int load_hero_talent_xml_info(xmlNodePtr cur);

private:
	HeroTalentXmlMap hero_talent_map;
};
extern HeroTalentXmlManager hero_talent_xml_mgr;

/********************************************************************************/
/*							SoldierTalentXmlManager								*/
/********************************************************************************/

/* @brief 天赋配置信息
 */
struct soldier_talent_xml_info_t {
	uint32_t id;				/*! 天赋id */
	uint32_t type;				/*! 天赋类型 */
	uint32_t passive_class;		/*! 被动buff增益方向 */
	uint32_t passive_type;		/*! 被动buff数值类型 */
	uint32_t passive_value;		/*! 被动buff数值 */
};
typedef std::map<uint32_t, soldier_talent_xml_info_t> SoldierTalentXmlMap;

class SoldierTalentXmlManager {
public:
	SoldierTalentXmlManager();
	~SoldierTalentXmlManager();

	int read_from_xml(const char *filename);
	const soldier_talent_xml_info_t* get_soldier_talent_xml_info(uint32_t talent_id);

private:
	int load_soldier_talent_xml_info(xmlNodePtr cur);

private:
	SoldierTalentXmlMap soldier_talent_map;
};
extern SoldierTalentXmlManager soldier_talent_xml_mgr;



#endif //TALENT_HPP_
