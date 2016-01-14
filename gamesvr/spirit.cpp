/*
 * =====================================================================================
 *
 *  @file  spirit.cpp 
 *
 *  @brief  处理玩家精灵相关信息
 *
 *  compiler  gcc4.3.2 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"
#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"

#include "spirit.hpp"
#include "player.hpp"
#include "equipment.hpp"
#include "skill.hpp"
#include "talent.hpp"
#include "dbroute.hpp"

using namespace project;
using namespace std;

SpiritXmlManager spirit_xml_mgr;
SpiritGrowthXmlManager spirit_growth_xml_mgr;
SpiritExpXmlManager spirit_exp_xml_mgr;
SpiritGradeAttrXmlManager spirit_grade_attr_xml_mgr;

/********************************************************************************/
/*							Spirit Class										*/
/********************************************************************************/
Spirit::Spirit(Player* p, uint32_t sp_id) : id(sp_id), owner(p)
{
	get_tm = 0;
	star = 0;
	grade = 0;
	lv = 0;
	lv_factor = 0;
	exp = 0;
	btl_power = 0;
	state = 0;

	str_growth = 0;
	int_growth = 0;
	agi_growth = 0;
	
	_str = 0;
	_int = 0;
	_agi = 0;
	_spd = 0;

	hp = 0;
	max_hp = 0;
	hp_recovery = 0;
	ep = 0;
	max_ep = 0;
	ep_recovery = 0;
	pa = 0;
	pd = 0;
	pcr = 0;
	pt = 0;
	ma = 0;
	md = 0;
	mcr = 0;
	mt = 0;
	hit = 0;
	miss = 0;
	pda = 0;
	pmr = 0;

	memset(equips, 0, sizeof(equips));
	memset(skills, 0, sizeof(skills));

	base_info = spirit_xml_mgr->get_spirit_xml_info(sp_id);

}

Spirit::~Spirit()
{

}

uint32_t
Spirit::get_upgrade_exp(uint32_t lv)
{
	return spirit_exp_xml_mgr->get_level_up_exp(lv);
}

int
Spirit::add_exp(uint32_t add_value)
{
	if (!add_value) {
		return 0;
	}

	uint32_t old_lv = lv;
	/*! 等级不能超过战队等级 */
	while (lv < owner->lv) {
		exp += add_value;
		uint32_t level_up_exp = get_upgrade_exp(lv);
		if (exp > level_up_exp) {//升级
			lv++;
			exp = 0;
			add_value -= level_up_exp;
		}
	}
	if (lv == owner->lv) {
		uint32_t upgrade_exp = get_upgrade_exp(owner->lv);
		if (exp >= upgrade_exp) {
			return 0;
		}
		uint32_t new_exp = exp + add_value;
		exp = (new_exp > upgrade_exp) ? upgrade_exp : new_exp;
	}
	
	if (lv > old_lv) {
		calc_all();
	}

	//更新DB
	db_set_spirit_simple_info_in db_in;
	db_in.sp_id = this->id;
	pack_spirit_simple_info(db_in.simple_info);

	return send_msg_to_dbroute(0, db_set_spirit_simple_info_cmd, &db_in, owner->user_id);
}

/* @brief 精灵穿戴装备
 */
int
Spirit::wear_equipment(uint32_t equip_id, uint32_t pos)
{
	const equip_xml_info_t* p_info = equip_xml_mgr->get_equip_xml_info(equip_id);
	if (!p_info) {
		return -1;
	}
	//检查等级是否足够
	if (lv < p_info->need_lv) {
		return -1;
	}
	if (!spirit_growth_xml_mgr->check_equip_is_can_wear(this->id, this->grade, equip_id, pos)) {
		return -1;
	}
	if (this->equips[pos-1]) {
		return -1;
	}

	this->equips[pos - 1] = equip_id;
	
	//删除装备
	owner->equip_mgr->del_equips(equip_id, 1);

	//更新DB
	db_spirit_wear_equipment_in db_in;
	db_in.sp_id = this->id;
	db_in.equip_id = equip_id;
	db_in.pos = pos;

	return send_msg_to_dbroute(owner, db_spirit_wear_equipment_cmd, &db_in, owner->user_id);
}

/* @brief 精灵升阶
 */
int
Spirit::rising_grade()
{
	if (grade >= MAX_SPIRIT_GRADE) {
		return 0;
	}

	for (uint32_t i = 0; i < 6; i++) {
		if (!equips[i]) {
			return cli_not_enough_equipment_err;
		} 
		if (!spirit_growth_xml_mgr->check_equip_is_can_wear(this->id, this->grade, this->equips[i], i + 1)) {
			return cli_invalid_equip_err;
		}
	}

	//扣除装备
	for (uint32_t i = 0; i < 6; i++) {
		equips[i] = 0;
	}

	//更新DB
	db_spirit_rising_grade_in db_in;
	db_in.sp_id = id;
	
	return send_msg_to_dbroute(0, db_spirit_rising_grade_cmd, &db_in, owner->user_id);
}

/* @brief 获取精灵升星所需碎片
 */
uint32_t
Spirit::get_need_star()
{
	uint32_t need_star[MAX_SPIRIT_STAR] = {10, 20, 50, 100, 150};

	return need_star[star];
}

/* @brief 精灵升星
 */
int
Spirit::rising_star()
{
	if (star >= MAX_SPIRIT_STAR) {
		return 0;
	}
	uint32_t need_cnt = get_need_star();
	uint32_t cur_cnt = owner->items_mgr->get_item_cnt(id);
	if (cur_cnt < need_cnt) {
		return cli_not_enough_item_err;
	}

	//扣除物品
	owner->items_mgr->del_item_without_callback(id, need_cnt);

	//升星
	star++;

	//更新成长
	calc_spirit_growth();

	//更新DB
	db_spirit_rising_star_in db_in;
	db_in.sp_id = id;

	KDEBUG_LOG(owner->user_id, "RISING STAR\t[sp_id=%u cur_cnt=%u need_cnt=%u]", id, cur_cnt, need_cnt);

	return send_msg_to_dbroute(0, db_spirit_rising_star_cmd, &db_in, owner->user_id);
}

/* @brief 精灵天赋升级
 */
int
Spirit::talent_level_up(uint32_t pos)
{
	if (!pos || pos > 4) {
		return 0;
	}

	const spirit_talent_xml_info_t* p_info = spirit_xml_mgr->get_talent_xml_info(this->id, pos);
	if (!p_info) {
		return cli_invalid_spirit_talent_id_err;
	}

	if (grade < p_info->unlock_grade) {
		return cli_not_enough_spirit_grade_err;
	}
	if (talent[pos-1][0] == 0) {
		talent[pos-1][1] = p_info->talent_id;
	}
	talent[pos-1][1]++;

	//更新DB
	db_spirit_talent_level_up_in db_in;
	db_in.sp_id = this->id;
	db_in.talent_id = talent[pos-1][0];
	db_in.pos = pos;

	return send_msg_to_dbroute(0, db_spirit_talent_level_up_cmd, &db_in, owner->user_id);
}

/* @brief 精灵吃经验道具
 */
int
Spirit::eat_exp_items(uint32_t item_id, uint32_t item_cnt)
{
	const item_xml_info_t* p_info = items_xml_mgr->get_item_xml_info(item_id);
	if (!p_info || p_info->type != em_item_type_for_exp) {
		return cli_invalid_item_err;
	}

	uint32_t cur_cnt = owner->items_mgr->get_item_cnt(item_id);
	if (cur_cnt < item_cnt) {
		return cli_not_enough_item_err;
	}

	//扣除物品
	owner->items_mgr->del_item_without_callback(item_id, item_cnt);

	//增加经验
	uint32_t total_exp = p_info->effect * item_cnt;
	add_exp(total_exp);

	return 0;
}

int
Spirit::calc_spirit_growth()
{
	if (!base_info || star > MAX_SPIRIT_STAR) {
		return 0;
	}

	str_growth = base_info->str_growth[star - 1];
	agi_growth = base_info->agi_growth[star - 1];
	int_growth = base_info->int_growth[star - 1];

	return 0;
}

int 
Spirit::calc_cur_grade_attr(spirit_attr_info_t& upgrade_attr)
{
	spirit_growth_xml_mgr->calc_grade_attr(this->id, this->grade, upgrade_attr);
	//spirit_grade_xml_mgr->

	return 0;
}

int
Spirit::calc_cur_equip_attr(spirit_attr_info_t& equip_attr)
{
	memset(&equip_attr, 0, sizeof(equip_attr));
	for (uint32_t i = 0; i < 6; i++) {
		uint32_t equip_id = this->equips[i];
		if (equip_id) {
			equip_xml_mgr->load_equip_attr_info(equip_id, equip_attr);
		}
	}

	return 0;
}

int 
Spirit::calc_cur_skill_attr1(spirit_attr_info_t& skill_attr1)
{
	for (uint32_t i = 0; i < 4; i++) {
		uint32_t skill_id = skills[i];
		skill_effect_xml_mgr->load_passive_skill_attr_info(1, skill_id, skill_attr1);

		uint32_t talent_id = talent[i][0];
		uint32_t talent_lv = talent[i][1];
		const talent_xml_info_t* p_talent = talent_xml_mgr->get_talent_xml_info(talent_id);
		if (p_talent) {
			for (uint32_t j = 0; j < 4; j++) {
				if (p_talent->effect_skills[j].skill_id == skill_id) {
					talent_effect_xml_mgr->load_passive_talent_attr_info(0, talent_id, talent_lv, skill_id, skill_attr1);
					break;
				}
			}
		}
	}
	return 0;
}

int
Spirit::calc_cur_skill_attr2(spirit_attr_info_t& skill_attr2)
{
	for (uint32_t i = 0; i < 4; i++) {
		uint32_t skill_id = skills[i];
		skill_effect_xml_mgr->load_passive_skill_attr_info(1, skill_id, skill_attr2);

		uint32_t talent_id = talent[i][0];
		uint32_t talent_lv = talent[i][1];
		const talent_xml_info_t* p_talent = talent_xml_mgr->get_talent_xml_info(talent_id);
		if (p_talent) {
			for (uint32_t j = 0; j < 4; j++) {
				if (p_talent->effect_skills[j].skill_id == skill_id) {
					talent_effect_xml_mgr->load_passive_talent_attr_info(1, talent_id, talent_lv, skill_id, skill_attr2);
					break;
				}
			}
		}
	}
	return 0;
}

int
Spirit::calc_level_factor()
{
	lv_factor = 0;
	return 0;
}

int 
Spirit::calc_all()
{
	//品阶加成
	spirit_attr_info_t upgrade_attr = {0};
   	calc_cur_grade_attr(upgrade_attr);

	//等级加成
	calc_level_factor();
	_str += lv_factor * str_growth;

	//装备加成
	spirit_attr_info_t equip_attr = {0};
	calc_cur_equip_attr(equip_attr);

	//技能加成 TODO
	//技能加成1 数值
	spirit_attr_info_t skill_attr1 = {0};
	calc_cur_skill_attr1(skill_attr1);

	//技能加成2 百分比
	spirit_attr_info_t skill_attr2 = {0};
	calc_cur_skill_attr2(skill_attr2);

	
	_str = (upgrade_attr._str + lv_factor * str_growth + equip_attr._str + skill_attr1._str) * (1 + skill_attr2._str / 100.0);
	_int = (upgrade_attr._int + lv_factor * str_growth + equip_attr._int + skill_attr1._int) * (1 + skill_attr2._int / 100.0);
	_agi = (upgrade_attr._agi + lv_factor * str_growth + equip_attr._agi + skill_attr1._agi) * (1 + skill_attr2._agi / 100.0);
	_spd = (_spd + skill_attr1._spd) * (1 + skill_attr2._spd / 100.0);

	//血量上限
	max_hp = (100 * _str + upgrade_attr.max_hp + equip_attr.max_hp + skill_attr1.max_hp) * (1 + skill_attr2.max_hp / 100.0);
	//能量上限
   	max_ep = 1000;

	//物攻
	pa = (20 * _agi + upgrade_attr.pa + equip_attr.pa + skill_attr1.pa) * (1 + skill_attr2.pa / 100.0);

	//特攻
	ma = (20 * _int + upgrade_attr.ma + equip_attr.ma + skill_attr1.ma) * (1 + skill_attr2.ma / 100.0);

	//护甲
	pd = (upgrade_attr.pd + equip_attr.pd + skill_attr1.pd) * (1 + skill_attr2.pd / 100.0);

	//特防
	md = (upgrade_attr.md + equip_attr.md + skill_attr1.md) * (1 + skill_attr2.md / 100.0);

	//物暴
	pcr = (upgrade_attr.pcr + equip_attr.pcr + skill_attr1.pcr) * (1 + skill_attr2.pcr / 100.0);

	//特暴
	mcr = (upgrade_attr.mcr + equip_attr.mcr + skill_attr1.mcr) * (1 + skill_attr2.mcr / 100.0);

	//物韧
	pt = (upgrade_attr.pt + equip_attr.pt + skill_attr1.pt) * (1 + skill_attr2.pt / 100.0);

	//特韧
	mt = (upgrade_attr.mt + equip_attr.mt + skill_attr1.mt) * (1 + skill_attr2.mt / 100.0);

	//生命回复
	hp_recovery = (upgrade_attr.hp_recovery + equip_attr.hp_recovery + skill_attr1.hp_recovery) * (1 + skill_attr2.hp_recovery / 100.0);

	//能量回复 
	ep_recovery = (upgrade_attr.ep_recovery + equip_attr.ep_recovery + skill_attr1.ep_recovery) * (1 + skill_attr2.ep_recovery / 100.0);

	//破甲
	pda = (upgrade_attr.pda + equip_attr.pda + skill_attr1.pda) * (1 + skill_attr2.pda / 100.0);

	//破魔
	pmr = (upgrade_attr.pmr + equip_attr.pmr + skill_attr1.pmr) * (1 + skill_attr2.pmr / 100.0);

	return 0;
}

int
Spirit::init_spirit_base_info(uint32_t init_lv)
{
	if (!base_info) {
		return -1;
	}
	get_tm = get_now_tv()->tv_sec;
	star = 1;
	grade = 1;
	lv = init_lv ? init_lv : 1;
	lv_factor = calc_level_factor();
	exp = 0;
	btl_power = 0;
	state = 0;

	str_growth = base_info->str_growth[star - 1];
	int_growth = base_info->int_growth[star - 1];
	agi_growth = base_info->agi_growth[star - 1];

	_str = base_info->attr._str;
	_agi = base_info->attr._agi;
	_int = base_info->attr._int;
	_spd = base_info->attr._spd;

	hp = base_info->attr.max_hp;
	max_hp = base_info->attr.max_hp;
	hp_recovery = base_info->attr.hp_recovery;
	ep = 1000;
	max_ep = 1000;
	ep_recovery = base_info->attr.ep_recovery;

	pa = base_info->attr.pa;
	pd = base_info->attr.pd;
	pcr = base_info->attr.pcr;
	pt = base_info->attr.pt;
	ma = base_info->attr.ma;
	md = base_info->attr.md;
	mcr = base_info->attr.mcr;
	mt = base_info->attr.mt;
	hit = base_info->attr.hit;
	miss = base_info->attr.miss;
	pda = base_info->attr.pda;
	pmr = base_info->attr.pmr;

	for (uint32_t i = 0; i < 4; i++) {
		skills[i] = base_info->skills[i];
	}

	calc_all();

	return 0;
}

int
Spirit::init_spirit_info(const db_spirit_info_t* p_info)
{
	lv = p_info->lv;
	exp = p_info->exp;
	star = p_info->star;
	grade = p_info->grade;
	for (uint32_t i = 0; i < 4; i++) {
		talent[i][0] = p_info->talent[i].id;
		talent[i][1] = p_info->talent[i].lv;
	}

	for (uint32_t i = 0; i < 6; i++) {
		equips[i] = p_info->equips[i];
	}

	return 0;
}

int
Spirit::pack_spirit_simple_info(db_spirit_simple_info_t& info)
{
	info.lv = lv;
	info.exp = exp;

	return 0;
}

int
Spirit::pack_spirit_db_info(db_spirit_info_t& info)
{
	info.sp_id = id;
  	info.lv = lv;
	info.exp = exp;
	info.star = star;
	info.grade = grade;
	for (uint32_t i = 0; i < 6; i++) {
		info.equips[i] = equips[i];
	}
	for (uint32_t i = 0; i < 4; i++) {
		info.talent[i].id = talent[i][0];
		info.talent[i].lv = talent[i][1];
	}

	return 0;
}


/********************************************************************************/
/*							SpiritManager Class									*/
/********************************************************************************/

SpiritManager::SpiritManager(Player* p) : owner(p) 
{

}

SpiritManager::~SpiritManager()
{
	SpiritsMap::iterator it = spirits.begin();
	for (; it != spirits.end(); it++) {
		Spirit* p_spirit = it->second;
		SAFE_DELETE(p_spirit);
	}
}

Spirit*
SpiritManager::get_spirit(uint32_t sp_id)
{
	map<uint32_t, Spirit*>::iterator it = spirits.find(sp_id);
	if (it == spirits.end()) {
		return 0;
	}

	return it->second;
}

Spirit* 
SpiritManager::add_spirit(uint32_t sp_id)
{
	const spirit_xml_info_t* p_info = spirit_xml_mgr->get_spirit_xml_info(sp_id);
	if (!p_info) {
		KERROR_LOG(owner->user_id, "invalid spirit id, sp_id=%u", sp_id);
		return 0;
	}

	//是否拥有该精灵
	if (get_spirit(sp_id)) {
		KERROR_LOG(owner->user_id, "already owner the spirit, sp_id=%u", sp_id);
		return 0;
	}

	Spirit* sp = new Spirit(owner, sp_id);
	if (!sp) {
		return 0;
	}

	//初始化精灵信息
	sp->init_spirit_base_info();

	//插入map
	spirits.insert(make_pair(sp_id, sp));

	//更新DB TODO
	db_add_spirit_info_in db_in;
	sp->pack_spirit_db_info(db_in.sp_info);
	send_msg_to_dbroute(0, db_add_spirit_info_cmd, &db_in, owner->user_id);
	
	KDEBUG_LOG(owner->user_id, "ADD SPIRIT\t[sp_id=%u]", sp_id);
	
	return sp;
}

int
SpiritManager::init_all_spirits_info(db_get_player_spirits_info_out* p_in)
{
	vector<db_spirit_info_t>::iterator it = p_in->spirits.begin();
	for (; it != p_in->spirits.end(); it++) {
		db_spirit_info_t* p_info = &(*it);
		const spirit_xml_info_t* sp_info = spirit_xml_mgr->get_spirit_xml_info(p_info->sp_id);
		if (!sp_info) {
			KERROR_LOG(owner->user_id, "invalid spirit id, sp_id=%u", p_info->sp_id);
			return -1;
		}

		Spirit* sp = new Spirit(owner, p_info->sp_id);
		if (!sp) {
			return -1;
		}
		sp->init_spirit_base_info(p_info->lv);
		sp->init_spirit_info(p_info);
		sp->calc_all();

		spirits.insert(SpiritsMap::value_type(sp->id, sp));
	}

	return 0;
}

int
SpiritManager::pack_all_spirits_info(cli_get_spirits_info_out& out)
{
	SpiritsMap::iterator it = spirits.begin();
	for (; it != spirits.end(); it++) {
		Spirit* sp = it->second;
		cli_spirit_info_t info;
		info.sp_id = sp->id;
		info.lv = sp->lv;
		info.exp = sp->exp;
		info.star = sp->star;
		info.grade = sp->grade;
		for (uint32_t i = 0; i < 6; i++) {
			info.equips[i] = sp->equips[i];
		}
		for (uint32_t i = 0; i < 4; i++) {
			info.skills[i] = sp->skills[i];
			info.talent[i].id = sp->talent[i][0];
			info.talent[i].lv = sp->talent[i][1];
		}

		out.spirits.push_back(info);
	}

	return 0;
}


/********************************************************************************/
/*						SpiritXmlManager Class									*/
/********************************************************************************/
SpiritXmlManager::SpiritXmlManager()
{

}

SpiritXmlManager::~SpiritXmlManager()
{

}

int 
SpiritXmlManager::read_from_xml(const char* filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_spirit_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	return ret; 
}

int
SpiritXmlManager::load_spirit_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("spirit"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");

			SpiritXmlMap::iterator it = spirit_xml_map.find(id);
			if (it != spirit_xml_map.end()) {
				KERROR_LOG(0, "load spirit xml info err, spirit id exists\t[id=%u]", id);
				return -1;
			}

			spirit_xml_info_t info;
			info.id = id;
			get_xml_prop(info.attr._str, cur, "str");
			get_xml_prop(info.attr._int, cur, "int");
			get_xml_prop(info.attr._agi, cur, "agi");
			get_xml_prop(info.attr._spd, cur, "spd");
			uint32_t count1 = get_xml_prop_arr(info.str_growth, cur, "str_growth");
			uint32_t count2 = get_xml_prop_arr(info.int_growth, cur, "int_growth");
			uint32_t count3 = get_xml_prop_arr(info.agi_growth, cur, "agi_growth");

			if (count1 < MAX_SPIRIT_STAR || count2 < MAX_SPIRIT_STAR || count3 < MAX_SPIRIT_STAR) {
				KERROR_LOG(0, "load spirit xml info err, count not enough\t[count1=%u count2=%u count3=%u]", count1, count2, count3);
				return -1;
			}
			get_xml_prop(info.attr.max_hp, cur, "maxhp");
			get_xml_prop(info.attr.hp_recovery, cur, "hp_recovery");
			get_xml_prop(info.attr.max_ep, cur, "maxep");
			get_xml_prop(info.attr.ep_recovery, cur, "ep_recovery");
			get_xml_prop(info.attr.pa, cur, "pa");
			get_xml_prop(info.attr.pd, cur, "pd");
			get_xml_prop(info.attr.pcr, cur, "pcr");
			get_xml_prop(info.attr.pt, cur, "pt");
			get_xml_prop(info.attr.ma, cur, "ma");
			get_xml_prop(info.attr.md, cur, "md");
			get_xml_prop(info.attr.mcr, cur, "mcr");
			get_xml_prop(info.attr.mt, cur, "mt");
			get_xml_prop(info.attr.hit, cur, "hit");
			get_xml_prop(info.attr.miss, cur, "miss");
			get_xml_prop(info.attr.pda, cur, "pda");
			get_xml_prop(info.attr.pmr, cur, "pmr");

			uint32_t count4 = get_xml_prop_arr(info.skills, cur, "skills");
			if (count4 < 4) {
				KERROR_LOG(0, "load spirit skill xml info err, count=%u", count4);
				return -1;
			}

			if (load_spirit_talent_xml_info(cur, info.talents) == -1) {
				KERROR_LOG(0, "load spirit talent xml info err");
				return -1;
			}

			KTRACE_LOG(0, "load spirit xml info\t[%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u]", 
					info.id, info.attr._str, info.attr._int, info.attr._agi, info.attr._spd, info.attr.max_hp, info.attr.hp_recovery, 
					info.attr.max_ep, info.attr.ep_recovery, info.attr.pa, info.attr.pd, info.attr.pcr, info.attr.pt, 
					info.attr.ma, info.attr.md, info.attr.mcr, info.attr.mt, info.attr.hit, info.attr.miss, info.attr.pda, info.attr.pmr);

			spirit_xml_map.insert(SpiritXmlMap::value_type(id, info));
		}

		cur = cur->next;
	}

	return 0;
}

int
SpiritXmlManager::load_spirit_talent_xml_info(xmlNodePtr cur, SpiritTalentXmlMap& talent_map)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("talent"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			SpiritTalentXmlMap::iterator it = talent_map.find(id);
			if (it != talent_map.end()) {
				KERROR_LOG(0, "load spirit talent xml info err, id exists, id=%u", id);
				return -1;
			}

			spirit_talent_xml_info_t info;
			info.id = id;
			get_xml_prop(info.talent_id, cur, "talentID");
			get_xml_prop(info.unlock_grade, cur, "unlockGrade");

			KTRACE_LOG(0, "load spirit talent xml info\t[%u %u %u]", info.id, info.talent_id, info.unlock_grade);

			talent_map.insert(SpiritTalentXmlMap::value_type(id, info));
		}

		cur = cur->next;
	}

	return 0;
}

const spirit_xml_info_t*
SpiritXmlManager::get_spirit_xml_info(uint32_t sp_id)
{
	SpiritXmlMap::iterator it = spirit_xml_map.find(sp_id);
	if (it == spirit_xml_map.end()) {
		return 0;
	}

	return &(it->second);
}

const spirit_talent_xml_info_t*
SpiritXmlManager::get_talent_xml_info(uint32_t sp_id, uint32_t talent_id)
{
	const spirit_xml_info_t* p_info = get_spirit_xml_info(sp_id);
	if (p_info) {
		SpiritTalentXmlMap::const_iterator it = p_info->talents.find(talent_id);
		if (it != p_info->talents.end()) {
			return &(it->second);
		}	
	}

	return 0;
}

/********************************************************************************/
/*						SpiritGrowthXmlManager Class							*/
/********************************************************************************/
SpiritGrowthXmlManager::SpiritGrowthXmlManager()
{

}

SpiritGrowthXmlManager::~SpiritGrowthXmlManager()
{
	
}

int
SpiritGrowthXmlManager::read_from_xml(const char* filename)
{
	xmlDocPtr doc = xmlParseFile(filename);
	if (!doc) {
		throw XmlParseError(string("failed to parse '") + filename + "'");
		ERROR_LOG("failed to parse spirit growth file!");
	}    

	xmlNodePtr cur = xmlDocGetRootElement(doc);
	if (!cur) {
		xmlFreeDoc(doc);
		throw XmlParseError(string("xmlDocGetRootElement error when loading '") + filename + "'");
		ERROR_LOG("xmlDocGetRootElement error when loading spirit growth file!");
	}    

	int ret = load_spirit_growth_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	return ret; 
}

int
SpiritGrowthXmlManager::load_spirit_growth_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("spirit"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			SpiritGrowthXmlMap::iterator it = spirit_growth_xml_map.find(id);
			if (it != spirit_growth_xml_map.end()) {
				KERROR_LOG(0, "load spirit growth xml info err, spirit id exists\t[id=%u]", id);
				return -1;
			}
			spirit_growth_xml_info_t info;
			info.id = id;
			if (load_spirit_growth_equip_xml_info(cur, info.equip_map) == -1) {
				KERROR_LOG(0, "load spirit growth equip xml info err\t[id=%u]", info.id);
				return -1;
			}

			KTRACE_LOG(0, "load spirit growth xml info\t[id=%u]", info.id);

			spirit_growth_xml_map.insert(SpiritGrowthXmlMap::value_type(id, info));

			load_grade_attr(id, info);
		}

		cur = cur->next;
	}

	return 0;
}

int
SpiritGrowthXmlManager::load_spirit_growth_equip_xml_info(xmlNodePtr cur, SpiritGrowthEquipXmlMap& p_map)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("equipGrowth"))) {
			uint32_t grade = 0;
			get_xml_prop(grade, cur, "grade");
			SpiritGrowthEquipXmlMap::iterator it = p_map.find(grade);
			if (it != p_map.end()) {
				KERROR_LOG(0, "load spirit growth equip xml info err, grade exists\t[grade=%u]", grade);
				return -1;
			}

			spirit_growth_equip_xml_info_t info;
			info.grade = grade;
			uint32_t count = get_xml_prop_arr(info.equips, cur, "equips");
			if (count != 6) {
				KERROR_LOG(0, "load spirit growth equip xml info err, count err\t[grade=%u, count=%u]", grade, count);
				return -1;
			}

			KTRACE_LOG(0, "load spirit growth equip xml info\t[%u %u %u %u %u %u %u]", 
					grade, info.equips[0], info.equips[1], info.equips[2], info.equips[3], info.equips[4], info.equips[5]);

			p_map.insert(SpiritGrowthEquipXmlMap::value_type(grade, info));
		}

		cur = cur->next;
	}

	return 0;
}

/* @brief 根据精灵进阶装备计算精灵进阶提升属性
 */
int
SpiritGrowthXmlManager::load_grade_attr(uint32_t sp_id, spirit_growth_xml_info_t& growth_info)
{
	spirit_grade_attr_info_t info;
	info.sp_id = sp_id;

	SpiritGrowthEquipXmlMap* p_map = &(growth_info.equip_map);
	SpiritGrowthEquipXmlMap::iterator it = p_map->begin();

	for (; it != p_map->end(); it++) {
		uint32_t grade = it->first;
		if (grade > MAX_SPIRIT_GRADE) {
			continue;
		}
		for (uint32_t i = 0; i < 6; i++) {
			uint32_t equip_id = it->second.equips[i];
			const equip_xml_info_t* p_equip = equip_xml_mgr->get_equip_xml_info(equip_id);
			if (p_equip) {
				spirit_attr_info_t attr_info;
				equip_xml_mgr->load_equip_attr_info(equip_id, attr_info);
				info.grade_attr[grade - 1]._str += attr_info._str;
				info.grade_attr[grade - 1]._int += attr_info._int;
				info.grade_attr[grade - 1]._agi += attr_info._agi;
				info.grade_attr[grade - 1]._spd += attr_info._spd;
				info.grade_attr[grade - 1].max_hp += attr_info.max_hp;
				info.grade_attr[grade - 1].max_ep += attr_info.max_ep;
				info.grade_attr[grade - 1].hp_recovery += attr_info.hp_recovery;
				info.grade_attr[grade - 1].ep_recovery += attr_info.ep_recovery;
				info.grade_attr[grade - 1].pa += attr_info.pa;
				info.grade_attr[grade - 1].pd += attr_info.pd;
				info.grade_attr[grade - 1].pcr += attr_info.pcr;
				info.grade_attr[grade - 1].pt += attr_info.pt;
				info.grade_attr[grade - 1].ma += attr_info.ma;
				info.grade_attr[grade - 1].md += attr_info.md;
				info.grade_attr[grade - 1].mcr += attr_info.mcr;
				info.grade_attr[grade - 1].mt += attr_info.mt;
				info.grade_attr[grade - 1].hit += attr_info.hit;
				info.grade_attr[grade - 1].miss += attr_info.miss;
				info.grade_attr[grade - 1].pda += attr_info.pda;
				info.grade_attr[grade - 1].pmr += attr_info.pmr;
			}
		}
	}

	SpiritGradeAttrMap::iterator it_attr = spirit_grade_attr_map.find(sp_id);
	if (it_attr != spirit_grade_attr_map.end()) {
		KERROR_LOG(0, "calc grade attr err, sp_id exists\t[sp_id=%u]", sp_id);
		return -1;
	}	

	spirit_grade_attr_map.insert(SpiritGradeAttrMap::value_type(sp_id, info));

	return 0;
}

/* @brief 获取精灵当前品阶所提升总属性
 */
int 
SpiritGrowthXmlManager::calc_grade_attr(uint32_t sp_id, uint32_t grade, spirit_attr_info_t& upgrade_attr)
{
	SpiritGradeAttrMap::iterator it = spirit_grade_attr_map.find(sp_id);
	if (it == spirit_grade_attr_map.end()) {
		return 0;
	}
	for (uint32_t i = 1; i < grade; i++) {
		upgrade_attr._str += it->second.grade_attr[i - 1]._str;
		upgrade_attr._int += it->second.grade_attr[i - 1]._int;
		upgrade_attr._agi += it->second.grade_attr[i - 1]._agi;
		upgrade_attr._spd += it->second.grade_attr[i - 1]._spd;

		upgrade_attr.max_hp += it->second.grade_attr[i - 1].max_hp;
		upgrade_attr.max_ep += it->second.grade_attr[i - 1].max_ep;
		upgrade_attr.hp_recovery += it->second.grade_attr[i - 1].hp_recovery;
		upgrade_attr.ep_recovery += it->second.grade_attr[i - 1].ep_recovery;
		upgrade_attr.pa += it->second.grade_attr[i - 1].pa;
		upgrade_attr.pd += it->second.grade_attr[i - 1].pd;
		upgrade_attr.pcr += it->second.grade_attr[i - 1].pcr;
		upgrade_attr.pt += it->second.grade_attr[i - 1].pt;
		upgrade_attr.ma += it->second.grade_attr[i - 1].ma;
		upgrade_attr.md += it->second.grade_attr[i - 1].md;
		upgrade_attr.mcr += it->second.grade_attr[i - 1].mcr;
		upgrade_attr.mt += it->second.grade_attr[i - 1].mt;
		upgrade_attr.hit += it->second.grade_attr[i - 1].hit;
		upgrade_attr.miss += it->second.grade_attr[i - 1].miss;
		upgrade_attr.pda += it->second.grade_attr[i - 1].pda;
		upgrade_attr.pmr += it->second.grade_attr[i - 1].pmr;
	}

	return 0;
}

/* @brief 检查当前装备是否可穿戴 返回装备pos
 */
bool
SpiritGrowthXmlManager::check_equip_is_can_wear(uint32_t sp_id, uint32_t grade, uint32_t equip_id, uint32_t pos)
{
	if (!pos || pos > 6) {
		return false;
	}
	SpiritGrowthXmlMap::iterator it = spirit_growth_xml_map.find(sp_id);
	if (it == spirit_growth_xml_map.end()) {
		return false;
	}

	SpiritGrowthEquipXmlMap::iterator it_grade = it->second.equip_map.find(grade);
	if (it_grade == it->second.equip_map.end()) {
		return false;
	}

	if (equip_id == it_grade->second.equips[pos - 1]) {
		return true;
	}

	return false;;
}



/********************************************************************************/
/*							SpiritExpXmlManager Class							*/
/********************************************************************************/
SpiritExpXmlManager::SpiritExpXmlManager()
{

}

SpiritExpXmlManager::~SpiritExpXmlManager()
{

}

int
SpiritExpXmlManager::read_from_xml(const char* filename)
{
	xmlDocPtr doc = xmlParseFile(filename);
	if (!doc) {
		throw XmlParseError(string("failed to parse '") + filename + "'");
		ERROR_LOG("failed to parse spirit exp file!");
	}    

	xmlNodePtr cur = xmlDocGetRootElement(doc);
	if (!cur) {
		xmlFreeDoc(doc);
		throw XmlParseError(string("xmlDocGetRootElement error when loading '") + filename + "'");
		ERROR_LOG("xmlDocGetRootElement error when loading spirit exp file!");
	}    

	int ret = load_spirit_exp_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	return ret; 
}

int
SpiritExpXmlManager::load_spirit_exp_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("spiritExp"))) {
			uint32_t level = 0;

			get_xml_prop(level, cur, "level");
			SpiritExpXmlMap::iterator it = spirit_exp_xml_map.find(level);
			if (it != spirit_exp_xml_map.end()) {
				KERROR_LOG(0, "load spirit exp xml info err, level exists\t[level=%u]", level);
				return -1;
			}

			spirit_exp_xml_info_t info;
			info.lv = level;
			get_xml_prop(info.need_exp, cur, "needExp");

			KTRACE_LOG(0, "load spirit exp xml info[%u %u]", info.lv, info.need_exp);

			spirit_exp_xml_map.insert(SpiritExpXmlMap::value_type(level, info));
		}

		cur = cur->next;
	}

	return 0;
}

/* @brief 获取精灵升级所需经验
 */
uint32_t
SpiritExpXmlManager::get_level_up_exp(uint32_t lv)
{
	SpiritExpXmlMap::iterator it = spirit_exp_xml_map.find(lv);
	if (it != spirit_exp_xml_map.end()) {
		return it->second.need_exp;
	}

	return 0;
}

/********************************************************************************/
/*							SpiritGradeAttrXmlManager Class						*/
/********************************************************************************/
SpiritGradeAttrXmlManager::SpiritGradeAttrXmlManager()
{

}

SpiritGradeAttrXmlManager::~SpiritGradeAttrXmlManager()
{

}

int
SpiritGradeAttrXmlManager::read_from_xml(const char* filename)
{
	xmlDocPtr doc = xmlParseFile(filename);
	if (!doc) {
		throw XmlParseError(string("failed to parse '") + filename + "'");
		ERROR_LOG("failed to parse spirit grade exp file!");
	}    

	xmlNodePtr cur = xmlDocGetRootElement(doc);
	if (!cur) {
		xmlFreeDoc(doc);
		throw XmlParseError(string("xmlDocGetRootElement error when loading '") + filename + "'");
		ERROR_LOG("xmlDocGetRootElement error when loading spirit grade exp file!");
	}    

	int ret = load_spirit_grade_attr_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	return ret; 
}

int
SpiritGradeAttrXmlManager::load_spirit_grade_attr_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("spirit"))) {
			uint32_t sp_id = 0;
			get_xml_prop(sp_id, cur, "id");
			SpiritGradeAttrXmlMap::iterator it = spirit_grade_map.find(sp_id);
			if (it != spirit_grade_map.end()) {
				ERROR_LOG("load spirit grade attr xml info err, sp_id exists, sp_id=%u", sp_id);
				return -1; 
			}

			spirit_grade_attr_xml_info_t info;
			info.sp_id = sp_id;
			if (load_spirit_grade_attr_detail_xml_info(cur, info.grade_map) == -1) {
				ERROR_LOG("load spirit grade attr xml info err, sp_id=%u", sp_id);
				return -1; 
			}

			TRACE_LOG("load spirit grade attr xml info\t[sp_id=%u]", sp_id);

			spirit_grade_map.insert(SpiritGradeAttrXmlMap::value_type(sp_id, info));
		}
		cur = cur->next;
	}

	return 0;
}

int
SpiritGradeAttrXmlManager::load_spirit_grade_attr_detail_xml_info(xmlNodePtr cur, SpiritGradeAttrDetailXmlMap& grade_map)
{
 	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("attr"))) {
			uint32_t grade = 0;
			get_xml_prop(grade, cur, "grade");
			SpiritGradeAttrDetailXmlMap::iterator it = grade_map.find(grade);
			if (it != grade_map.end()) {
				ERROR_LOG("load spirit grade attr detail xml info err, grade exists, grade=%u", grade);
				return -1;
			}

			spirit_grade_attr_detail_xml_info_t info;
			info.grade = grade;
			get_xml_prop(info.attr._str, cur, "str");
			get_xml_prop(info.attr._agi, cur, "agi");
			get_xml_prop(info.attr._int, cur, "int");
			get_xml_prop(info.attr._spd, cur, "spd");
			get_xml_prop(info.attr.max_hp, cur, "maxhp");
			get_xml_prop(info.attr.pa, cur, "pa");
			get_xml_prop(info.attr.pd, cur, "pd");
			get_xml_prop(info.attr.pcr, cur, "pcr");
			get_xml_prop(info.attr.pt, cur, "pt");
			get_xml_prop(info.attr.ma, cur, "ma");
			get_xml_prop(info.attr.md, cur, "md");
			get_xml_prop(info.attr.mcr, cur, "mcr");
			get_xml_prop(info.attr.mt, cur, "mt");
			get_xml_prop(info.attr.pda, cur, "pda");
			get_xml_prop(info.attr.pmr, cur, "pmr");
			get_xml_prop(info.attr.hp_recovery, cur, "hp_recovery");
			get_xml_prop(info.attr.ep_recovery, cur, "ep_recovery");


			TRACE_LOG("load spirit grade attr detail info\t[%u]", grade);

			grade_map.insert(SpiritGradeAttrDetailXmlMap::value_type(grade, info));
		}
		cur = cur->next;
	}

	return 0;
}

/*
int
SpoiritGradeAttrXmlManager::calc_grade_attr(uint32_t sp_id, uint32_t grade, spi)
*/

/********************************************************************************/
/*								client request									*/
/********************************************************************************/
/* @brief 拉取精灵信息
 */
int cli_get_spirits_info(Player* p, Cmessage* c_in)
{
	return send_msg_to_dbroute(p, db_get_player_spirits_info_cmd, 0, p->user_id);
}

/* @brief 精灵穿戴装备
 */
int cli_spirit_wear_equipment(Player* p, Cmessage* c_in)
{
	cli_spirit_wear_equipment_in* p_in = P_IN;
	
	Spirit* sp = p->spirit_mgr->get_spirit(p_in->sp_id);
   	if (!sp) {
		KERROR_LOG(p->user_id, "spirit not exists, sp_id=%u", p_in->sp_id);
		return -1;
	}

	if (sp->wear_equipment(p_in->equip_id, p_in->pos) == -1) {
		KERROR_LOG(p->user_id, "spirit wear equipment err, sp_id=%u, equip_id=%u, pos=%u", 
				p_in->sp_id, p_in->equip_id, p_in->pos);
		return -1;
	}

	KDEBUG_LOG(p->user_id, "SPIRIT WEAR EQUIPMENT\t[sp_id=%u equip_id=%u pos=%u]", p_in->sp_id, p_in->equip_id, p_in->pos);

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 精灵升星
 */
int cli_spirit_rising_star(Player* p, Cmessage* c_in)
{
	cli_spirit_rising_star_in* p_in = P_IN;
	Spirit* sp = p->spirit_mgr->get_spirit(p_in->sp_id);
   	if (!sp) {
		KERROR_LOG(p->user_id, "spirit not exists, sp_id=%u", p_in->sp_id);
		return -1;
	}

	
	int ret = sp->rising_star();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 精灵升阶
 */
int cli_spirit_rising_grade(Player* p, Cmessage* c_in)
{
	cli_spirit_rising_grade_in* p_in = P_IN;
	Spirit* sp = p->spirit_mgr->get_spirit(p_in->sp_id);
   	if (!sp) {
		KERROR_LOG(p->user_id, "spirit not exists, sp_id=%u", p_in->sp_id);
		return -1;
	}

	int ret = sp->rising_grade();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 精灵天赋升级
 */
int cli_spirit_talent_level_up(Player* p, Cmessage* c_in)
{
	cli_spirit_talent_level_up_in* p_in = P_IN;
	Spirit* sp = p->spirit_mgr->get_spirit(p_in->sp_id);
   	if (!sp) {
		KERROR_LOG(p->user_id, "spirit not exists, sp_id=%u", p_in->sp_id);
		return -1;
	}
	if (!p_in->talent_pos || p_in->talent_pos > 4) {
		KERROR_LOG(p->user_id, "spirit talent pos err, sp_id=%u, pos=%u", p_in->sp_id, p_in->talent_pos);
		return -1;
	}
	const spirit_xml_info_t* p_info = spirit_xml_mgr->get_spirit_xml_info(sp->id);
	if (!p_info) {
		KERROR_LOG(p->user_id, "invalid spirit id, sp_id=%u", sp->id);
		return -1;
	}

	int ret = sp->talent_level_up(p_in->talent_pos);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	KDEBUG_LOG(p->user_id, "SPIRIT TALENT LEVEL UP!\t[sp_id=%u, talent_pos=%u]", p_in->sp_id, p_in->talent_pos);

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 精灵吃经验道具
 */
int cli_spirit_eat_exp_items(Player* p, Cmessage* c_in)
{
	cli_spirit_eat_exp_items_in* p_in = P_IN;
	Spirit* sp = p->spirit_mgr->get_spirit(p_in->sp_id);
	if (!sp) {
		KERROR_LOG(p->user_id, "spirit not exists, sp_id=%u", p_in->sp_id);
		return -1;
	}

	int ret = sp->eat_exp_items(p_in->item_id, p_in->item_cnt);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_spirit_eat_exp_items_out cli_out;
	cli_out.lv = sp->lv;
	cli_out.exp = sp->exp;

	KDEBUG_LOG(p->user_id, "SPIRIT EAT EXP ITEMS\t[sp_id=%u item_id=%u item_cnt=%u]", p_in->sp_id, p_in->item_id, p_in->item_cnt);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/********************************************************************************/
/*									dbsvr return 								*/
/********************************************************************************/
/* @brief 拉取精灵信息返回
 */
int db_get_player_spirits_info(Player* p, Cmessage* c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);

	db_get_player_spirits_info_out* p_in = P_IN;

	p->spirit_mgr->init_all_spirits_info(p_in);

	if (p->wait_cmd == cli_proto_login_cmd) {
		p->login_step++;

		KDEBUG_LOG(p->user_id, "LOGIN STEP %u SPIRIRS INFO", p->login_step);

		cli_get_spirits_info_out cli_out;
		p->spirit_mgr->pack_all_spirits_info(cli_out);
		
		//TODO:
		//p->wait_cmd = 0;
		//return 0;
		return p->send_to_self(cli_get_spirits_info_cmd, &cli_out, 1);
	} else if (p->wait_cmd == cli_get_spirits_info_cmd) {
		cli_get_spirits_info_out cli_out;
		p->spirit_mgr->pack_all_spirits_info(cli_out);

		return p->send_to_self(p->wait_cmd, &cli_out, 1);
		
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}
