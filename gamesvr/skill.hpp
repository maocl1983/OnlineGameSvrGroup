/* * ===================================================================================== *
 *  @file  skill.hpp 
 *
 *  @brief  处理技能相关逻辑
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 * copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#ifndef SKILL_HPP_
#define SKILL_HPP_

#include "common_def.hpp"

/********************************************************************************/
/*							SkillXmlManager									*/
/********************************************************************************/

/* @brief 技能配置信息
 */
struct skill_xml_info_t {
	uint32_t id;				/*! 技能id */
	uint32_t type;				/*! 技能类型 */
	uint32_t cd;				/*! 冷却时间 */
	uint32_t target;			/*! 作用目标 */
	uint32_t target_type;		/*! 作用类型 */
	uint32_t shape;				/*! 影响形状 */
	uint32_t target_pos;		/*! 影响距离 */
	uint32_t target_num;		/*! 影响个数 */
	uint32_t immediate_effect;	/*! 立即型效果 */
	uint32_t chant_effect;		/*! 吟唱型效果 */
	uint32_t buff[2];			/*! 持续型buff */
	uint32_t passive_buff;		/*! 被动型buff */
};
typedef std::map<uint32_t, skill_xml_info_t> SkillXmlMap;

class SkillXmlManager {
public:
	SkillXmlManager();
	~SkillXmlManager();

	int read_from_xml(const char *filename);
	const skill_xml_info_t* get_skill_xml_info(uint32_t skill_id);

private:
	int load_skill_xml_info(xmlNodePtr cur);

private:
	SkillXmlMap skill_map;
};
//extern SkillXmlManager skill_xml_mgr;

/********************************************************************************/
/*						SkillEffectXmlManager									*/
/********************************************************************************/

struct skill_effect_buff_xml_info_t {
	uint32_t buff_class;	/*! buff增益 */
	uint32_t value_type;	/*! 数值类型 0:数值 1:比例 */
	uint32_t value;			/*! 数值 */
	uint32_t buff_tm;		/*! buff持续时间 */
	uint32_t buff_rate;		/*! buff触发几率 */
};

struct skill_effect_xml_info_t {
	uint32_t id;							/*! 技能id */
	uint32_t immediate_power;				/*! 立即型伤害威力 */
	uint32_t immediate_scale;				/*! 立即型伤害系数 */
	uint32_t immediate_value;				/*! 立即型伤害附加定额伤害 */
	uint32_t immediate_rate;				/*! 立即型伤害触发几率 */
	uint32_t chant_power;					/*! 吟唱型伤害威力 */
	uint32_t chant_scale;					/*! 吟唱型伤害系数 */
	uint32_t chant_tm;						/*! 吟唱型持续时间 */
	uint32_t chant_rate;					/*! 吟唱型伤害触发几率 */
	skill_effect_buff_xml_info_t buff[2];	/*! 持续型buff */
	uint32_t passive_class;					/*! 被动buff增益方向 */
	uint32_t passive_type;					/*! 被动buff数值类型 */
	uint32_t passive_value;					/*! 被动buff数值 */
};
typedef std::map<uint32_t, skill_effect_xml_info_t> SkillEffectXmlMap;

class SkillEffectXmlManager {
public:
	SkillEffectXmlManager();
	~SkillEffectXmlManager();

	const skill_effect_xml_info_t* get_skill_effect_xml_info(uint32_t skill_id);
	int read_from_xml(const char *filename);

private:
	int load_skill_effect_xml_info(xmlNodePtr cur);

private:
	SkillEffectXmlMap skill_effect_map;
};
//extern SkillEffectXmlManager skill_effect_xml_mgr;

/********************************************************************************/
/*						SkillLevelupGoldsXmlManager								*/
/********************************************************************************/
struct skill_levelup_golds_xml_info_t {
	uint32_t lv;
	uint32_t golds;
};
typedef std::map<uint32_t, skill_levelup_golds_xml_info_t> SkillLevelupGoldsXmlMap;

class SkillLevelupGoldsXmlManager {
public:
	SkillLevelupGoldsXmlManager();
	~SkillLevelupGoldsXmlManager();

	int read_from_xml(const char *filename);
	int get_skill_levelup_golds(uint32_t lv);

private:
	int load_skill_levelup_golds_xml_info(xmlNodePtr cur);

private:
	SkillLevelupGoldsXmlMap levelup_golds_map;
};
//extern SkillLevelupGoldsXmlManager skill_levelup_golds_xml_mgr;

#endif //SKILL_HPP_
