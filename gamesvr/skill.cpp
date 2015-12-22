/*
 * =====================================================================================
 *
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

#include "skill.hpp"


using namespace std;
using namespace project;

SkillXmlManager skill_xml_mgr;
SkillEffectXmlManager skill_effect_xml_mgr;
SkillLevelupGoldsXmlManager skill_levelup_golds_xml_mgr;


/********************************************************************************/
/*							SkillXmlManager										*/
/********************************************************************************/
SkillXmlManager::SkillXmlManager()
{

}

SkillXmlManager::~SkillXmlManager()
{

}

const skill_xml_info_t*
SkillXmlManager::get_skill_xml_info(uint32_t skill_id)
{
	SkillXmlMap::const_iterator it = skill_map.find(skill_id);
	if (it != skill_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
SkillXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_skill_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	return ret;
}

int
SkillXmlManager::load_skill_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("skill"))) {
			uint32_t skill_id = 0;
			get_xml_prop(skill_id, cur, "id");

			SkillXmlMap::iterator it = skill_map.find(skill_id);
			if (it != skill_map.end()) {
				ERROR_LOG("load skill xml info err, id exists, id=%u", skill_id);
				return -1;
			}

			skill_xml_info_t info;
			info.id = skill_id;

			get_xml_prop(info.type, cur, "type");
			get_xml_prop(info.cd, cur, "cd");
			get_xml_prop(info.target, cur, "target");
			get_xml_prop(info.target_type, cur, "target_type");
			get_xml_prop(info.shape, cur, "shape");
			get_xml_prop(info.target_pos, cur, "target_pos");
			get_xml_prop(info.target_num, cur, "target_num");
			get_xml_prop(info.immediate_effect, cur, "immediate_effect");
			get_xml_prop(info.chant_effect, cur, "chant_effect");
			get_xml_prop(info.buff[0], cur, "buff_1");
			get_xml_prop(info.buff[1], cur, "buff_2");
			get_xml_prop(info.passive_buff, cur, "passive_buff");

			TRACE_LOG("load skill xml info\t[%u %u %u %u %u %u %u %u %u %u %u %u %u]", 
					info.id, info.type, info.cd, info.target, info.target_type, info.shape, info.target_pos, info.target_num, 
					info.immediate_effect, info.chant_effect, info.buff[0], info.buff[1], info.passive_buff); 

			skill_map.insert(SkillXmlMap::value_type(skill_id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*						SkillEffectXmlManager									*/
/********************************************************************************/
SkillEffectXmlManager::SkillEffectXmlManager()
{

}

SkillEffectXmlManager::~SkillEffectXmlManager()
{

}

const skill_effect_xml_info_t*
SkillEffectXmlManager::get_skill_effect_xml_info(uint32_t skill_id)
{
	SkillEffectXmlMap::const_iterator it = skill_effect_map.find(skill_id);
	if (it != skill_effect_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
SkillEffectXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_skill_effect_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	return ret;
}

int
SkillEffectXmlManager::load_skill_effect_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("skill"))) {
			uint32_t skill_id = 0;
			get_xml_prop(skill_id, cur, "id");
			SkillEffectXmlMap::iterator it = skill_effect_map.find(skill_id);
			if (it != skill_effect_map.end()) {
				ERROR_LOG("load skill effect xml info err, id exists, id=%u", skill_id);
				return -1;
			}

			skill_effect_xml_info_t info;
			info.id = skill_id;
			get_xml_prop_def(info.immediate_power, cur, "immediate_power", 0);
			get_xml_prop_def(info.immediate_scale, cur, "immediate_scale", 0);
			get_xml_prop_def(info.immediate_value, cur, "immediate_value", 0);
			get_xml_prop_def(info.immediate_rate, cur, "immediate_rate", 0);
			get_xml_prop_def(info.chant_power, cur, "chant_power", 0);
			get_xml_prop_def(info.chant_scale, cur, "chant_scale", 0);
			get_xml_prop_def(info.chant_tm, cur, "chant_tm", 0);
			get_xml_prop_def(info.chant_rate, cur, "chant_rate", 0);
			get_xml_prop_def(info.buff[0].buff_class, cur, "bf_1_class", 0);
			get_xml_prop_def(info.buff[0].value_type, cur, "bf_1_value_type", 0);
			get_xml_prop_def(info.buff[0].value, cur, "bf_1_value", 0);
			get_xml_prop_def(info.buff[0].buff_tm, cur, "bf_1_tm", 0);
			get_xml_prop_def(info.buff[0].buff_rate, cur, "bf_1_rate", 0);
			get_xml_prop_def(info.buff[1].buff_class, cur, "bf_2_class", 0);
			get_xml_prop_def(info.buff[1].value_type, cur, "bf_2_value_type", 0);
			get_xml_prop_def(info.buff[1].value, cur, "bf_2_value", 0);
			get_xml_prop_def(info.buff[1].buff_tm, cur, "bf_2_tm", 0);
			get_xml_prop_def(info.buff[1].buff_rate, cur, "bf_2_rate", 0);
			get_xml_prop_def(info.passive_class, cur, "passive_class", 0);
			get_xml_prop_def(info.passive_type, cur, "passive_type", 0);
			get_xml_prop_def(info.passive_value, cur, "passive_value", 0);

			TRACE_LOG("load skill effect xml info\t[%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u]", 
					info.id, info.immediate_power, info.immediate_scale, info.immediate_value, info.immediate_rate,
					info.chant_power, info.chant_scale, info.chant_tm, info.chant_rate, 
					info.buff[0].buff_class, info.buff[0].value_type, info.buff[0].value, info.buff[0].buff_tm, info.buff[0].buff_rate,
					info.buff[1].buff_class, info.buff[1].value_type, info.buff[1].value, info.buff[1].buff_tm, info.buff[1].buff_rate,
					info.passive_class, info.passive_type, info.passive_value); 

			skill_effect_map.insert(SkillEffectXmlMap::value_type(skill_id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*						SkillLevelupGoldsXmlManager								*/
/********************************************************************************/
SkillLevelupGoldsXmlManager::SkillLevelupGoldsXmlManager()
{
}

SkillLevelupGoldsXmlManager::~SkillLevelupGoldsXmlManager()
{
}

int
SkillLevelupGoldsXmlManager::get_skill_levelup_golds(uint32_t lv)
{
	SkillLevelupGoldsXmlMap::iterator it = levelup_golds_map.find(lv);
	if (it == levelup_golds_map.end()) {
		return 0;
	}

	return it->second.golds;
}

int
SkillLevelupGoldsXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_skill_levelup_golds_xml_info(cur);

	BOOT_LOG(ret, "Load File %s!", filename);

	return ret;
}

int
SkillLevelupGoldsXmlManager::load_skill_levelup_golds_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("level"))) {
			uint32_t lv = 0;
			get_xml_prop(lv, cur, "skill_lv");
			SkillLevelupGoldsXmlMap::iterator it = levelup_golds_map.find(lv);
			if (it != levelup_golds_map.end()) {
				ERROR_LOG("load skill levelup golds xml info err, lv exist, lv=%u", lv);
				return -1;
			}

			skill_levelup_golds_xml_info_t info;
			info.lv = lv;
			get_xml_prop(info.golds, cur, "value1");

			TRACE_LOG("load skill levelup golds xml info\t[%u %u]", info.lv, info.golds);

			levelup_golds_map.insert(SkillLevelupGoldsXmlMap::value_type(lv, info));
		}
		cur = cur->next;
	}
	return 0;
}
