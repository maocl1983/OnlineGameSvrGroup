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

#include "talent.hpp"


using namespace std;
using namespace project;

HeroTalentXmlManager hero_talent_xml_mgr;
SoldierTalentXmlManager soldier_talent_xml_mgr;


/********************************************************************************/
/*							HeroTalentXmlManager									*/
/********************************************************************************/
HeroTalentXmlManager::HeroTalentXmlManager()
{

}

HeroTalentXmlManager::~HeroTalentXmlManager()
{

}

const hero_talent_xml_info_t*
HeroTalentXmlManager::get_hero_talent_xml_info(uint32_t talent_id)
{
	HeroTalentXmlMap::const_iterator it = hero_talent_map.find(talent_id);
	if (it != hero_talent_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
HeroTalentXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_hero_talent_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
HeroTalentXmlManager::load_hero_talent_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("talent"))) {
			uint32_t talent_id = 0;
			get_xml_prop(talent_id, cur, "id");

			HeroTalentXmlMap::iterator it = hero_talent_map.find(talent_id);
			if (it != hero_talent_map.end()) {
				ERROR_LOG("load hero_talent xml info err, id exists, id=%u", talent_id);
				return -1;
			}

			hero_talent_xml_info_t info = {};
			info.id = talent_id;

			get_xml_prop(info.type, cur, "type");
			get_xml_prop(info.passive_class, cur, "passive_class");
			get_xml_prop(info.passive_type, cur, "passive_type");
			get_xml_prop(info.passive_value, cur, "passive_value");

			TRACE_LOG("load hero talent xml info\t[%u %u %u %u %u]", 
					info.id, info.type, info.passive_class, info.passive_type, info.passive_value);

			hero_talent_map.insert(HeroTalentXmlMap::value_type(talent_id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							SoldierTalentXmlManager									*/
/********************************************************************************/
SoldierTalentXmlManager::SoldierTalentXmlManager()
{

}

SoldierTalentXmlManager::~SoldierTalentXmlManager()
{

}

const soldier_talent_xml_info_t*
SoldierTalentXmlManager::get_soldier_talent_xml_info(uint32_t talent_id)
{
	SoldierTalentXmlMap::const_iterator it = soldier_talent_map.find(talent_id);
	if (it != soldier_talent_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
SoldierTalentXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_soldier_talent_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
SoldierTalentXmlManager::load_soldier_talent_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("talent"))) {
			uint32_t talent_id = 0;
			get_xml_prop(talent_id, cur, "id");

			SoldierTalentXmlMap::iterator it = soldier_talent_map.find(talent_id);
			if (it != soldier_talent_map.end()) {
				ERROR_LOG("load soldier_talent xml info err, id exists, id=%u", talent_id);
				return -1;
			}

			soldier_talent_xml_info_t info = {};
			info.id = talent_id;

			get_xml_prop(info.type, cur, "type");
			get_xml_prop(info.passive_class, cur, "passive_class");
			get_xml_prop(info.passive_type, cur, "passive_type");
			get_xml_prop(info.passive_value, cur, "passive_value");

			TRACE_LOG("load soldier talent xml info\t[%u %u %u %u %u]", 
					info.id, info.type, info.passive_class, info.passive_type, info.passive_value);

			soldier_talent_map.insert(SoldierTalentXmlMap::value_type(talent_id, info));
		}
		cur = cur->next;
	}

	return 0;
}
