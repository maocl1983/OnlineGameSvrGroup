/*
 * =====================================================================================
 *
 *  @file  horse.cpp 
 *
 *  @brief  战马系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
/*
#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"
#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"

#include "global_data.hpp"
#include "player.hpp"
#include "hero.hpp"
#include "item.hpp"
#include "dbroute.hpp"
#include "horse.hpp"
*/

#include "stdafx.hpp"
using namespace std;
using namespace project;


//HorseAttrXmlManager horse_attr_xml_mgr;
//HorseExpXmlManager horse_exp_xml_mgr;
//HorseEquipXmlManager horse_equip_xml_mgr;

/********************************************************************************/
/*							Horse Class											*/
/********************************************************************************/
Horse::Horse(Player *p) : owner(p)
{
	lv = 0;
	exp = 0;

	max_hp = 0;
	ad = 0;
	armor = 0;
	resist = 0;
	ad_cri = 0;
	ad_ren = 0;
	cri_damage = 0;
	cri_avoid = 0;
	hit = 0;
	miss = 0;
	out_max_hp = 0;
	out_ad = 0;
	out_armor = 0;
	out_resist = 0;
	soldier_max_hp = 0;
	soldier_ad = 0;

	base_info = 0;

	horse_equips.clear();

}

Horse::~Horse()
{
	horse_equips.clear();
}

int
Horse::init_horse_info_from_db(uint32_t horse_lv, uint32_t horse_exp)
{
	lv = horse_lv;
	exp = horse_exp;

	base_info = horse_attr_xml_mgr->get_horse_attr_xml_info(lv);

	calc_all();

	T_KTRACE_LOG(owner->user_id, "init horse info from db\t[%u %u]", lv, exp);
	return 0;
}

int
Horse::calc_base_attr()
{
	const horse_attr_xml_info_t *p_attr_info = horse_attr_xml_mgr->get_horse_attr_xml_info(lv);
	if (!p_attr_info) {
		return 0;
	}
	max_hp = p_attr_info->max_hp;
	ad = p_attr_info->ad;
	armor = p_attr_info->armor;
	resist = p_attr_info->resist;
	out_max_hp = p_attr_info->out_max_hp;
	out_ad = p_attr_info->out_ad;
	out_armor = p_attr_info->out_armor;
	out_resist = p_attr_info->out_resist;
	soldier_max_hp = p_attr_info->soldier_max_hp;
	soldier_ad = p_attr_info->soldier_ad;

	return 0;
}

int
Horse::calc_equip_attr()
{
	HorseEquipMap::iterator it = horse_equips.begin();
	for (; it != horse_equips.end(); ++it) {
		const horse_equip_xml_info_t *p_info = it->second->base_info;
		if (p_info) {
			max_hp += p_info->max_hp;
			ad += p_info->ad;
			armor += p_info->armor;
			resist += p_info->resist;
			ad_cri += p_info->ad_cri;
			cri_damage += p_info->cri_damage;
			cri_avoid += p_info->cri_avoid;
			hit += p_info->hit;
			miss += p_info->miss;
		}	
	}

	return 0;
}

int 
Horse::calc_all()
{
	base_info = horse_attr_xml_mgr->get_horse_attr_xml_info(lv);
	calc_base_attr();
	calc_equip_attr();

	return 0;
}


HorseEquip*
Horse::get_horse_equip(uint32_t get_tm)
{
	HorseEquipMap::iterator it = horse_equips.find(get_tm);
	if (it != horse_equips.end()) {
		return it->second;
	}

	return 0;
}

int
Horse::get_levelup_exp(uint32_t lv)
{
	if (lv > MAX_HORSE_LEVEL) {
		return -1;
	}
	const horse_exp_xml_info_t *p_xml_info = horse_exp_xml_mgr->get_horse_exp_xml_info(lv);
	if (!p_xml_info) {
		return -1;
	}

	return p_xml_info->exp;
}

bool
Horse::check_is_can_add_exp()
{
	uint32_t cur_levelup_exp = get_levelup_exp(lv);
	if (lv >= 10 && exp >= cur_levelup_exp && !check_horse_is_breakthrough()) {//未曾突破则不能升级
		return false;
	}

	return true;
}

int
Horse::add_exp(uint32_t add_value, bool breakthrough)
{
	if (!add_value && !breakthrough) {
		return 0;
	}

	if (lv > MAX_HORSE_LEVEL) {
		return 0;
	}

	if (!check_is_can_add_exp()) {
		return 0;
	}

	uint32_t old_lv = lv;

	exp += add_value;

	if (lv == MAX_HORSE_LEVEL) {
		uint32_t need_exp = get_levelup_exp(lv);
		if (exp > need_exp) {
			exp = need_exp;
		}
	} else {
		uint32_t levelup_exp = get_levelup_exp(lv);
		while (exp >= levelup_exp) {
			if (!check_is_can_add_exp()) {
				break;
			}
			lv++;
			exp -= levelup_exp;
			if (lv >= MAX_HORSE_LEVEL) {
				uint32_t max_exp = get_levelup_exp(MAX_HORSE_LEVEL);
				if (exp > max_exp) {
					exp = max_exp;
				}
				break;
			}
			levelup_exp = get_levelup_exp(lv);
		}
	}

	if (lv > old_lv) {
		calc_all();
		owner->horse_lv = lv;
		owner->horse_exp = exp;
		send_horse_attr_change_noti();
	}

	//更新DB
	db_update_horse_info_in db_in;
	db_in.horse_lv = lv;
	db_in.horse_exp = exp;
	send_msg_to_dbroute(0, db_update_horse_info_cmd, &db_in, owner->user_id);

	//战马经验变化通知
	cli_horse_exp_change_noti_out noti_out;
	noti_out.horse_lv = lv;
	noti_out.horse_exp = exp;
	noti_out.horse_levelup_exp = get_levelup_exp(lv);
	owner->send_to_self(cli_horse_exp_change_noti_cmd, &noti_out, 0);

	T_KTRACE_LOG(owner->user_id, "add horse exp\t[%u %u %u]", add_value, lv, exp);

	return 0;
}

int
Horse::send_horse_attr_change_noti()
{
	cli_horse_attr_change_noti_out noti_out;
	noti_out.horse_info.max_hp = max_hp;
	noti_out.horse_info.ad = ad;
	noti_out.horse_info.armor = armor;
	noti_out.horse_info.resist = resist;
	noti_out.horse_info.ad_cri = ad_cri;
	noti_out.horse_info.ad_ren = ad_ren;
	noti_out.horse_info.cri_damage = cri_damage;
	noti_out.horse_info.cri_avoid = cri_avoid;
	noti_out.horse_info.hit = hit;
	noti_out.horse_info.miss = miss;
	noti_out.horse_info.out_max_hp = out_max_hp;
	noti_out.horse_info.out_ad = out_ad;
	noti_out.horse_info.out_armor = out_armor;
	noti_out.horse_info.out_resist = out_resist;
	noti_out.horse_info.soldier_max_hp = soldier_max_hp;
	noti_out.horse_info.soldier_ad = soldier_ad;

	T_KDEBUG_LOG(owner->user_id, "HORSE ATTR CHANGED!");

	return owner->send_to_self(cli_horse_attr_change_noti_cmd, &noti_out, 0);
}

int
Horse::horse_train(uint32_t type, uint32_t &cri_flag)
{
	if (!type || type > 2) {
		T_KWARN_LOG(owner->user_id, "horse train type err, type=%u", type);
		return cli_invalid_input_arg_err;
	}

	const horse_exp_xml_info_t *p_xml_info = horse_exp_xml_mgr->get_horse_exp_xml_info(this->lv);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "horse exp xml info not found err, type=%u, lv=%u", type, lv);
		return cli_invalid_horse_lv_err;
	}

	if (!check_is_can_add_exp()) {//未曾突破则不能升级
		return cli_horse_not_breakthrough_err;
	}

	cri_flag = 0;
	uint32_t add_exp = 0;
	if (type == 1) {//金币培养
		uint32_t golds_tms = owner->res_mgr->get_res_value(daily_horse_train_golds_tms);
		if (golds_tms >= 20) {
			T_KWARN_LOG(owner->user_id, "horse train daily tms is over, type=%u, golds_tms=%u", type, golds_tms);
			return cli_horse_train_tms_not_enough_err;
		}
		//检查金币是否足够
		if (owner->golds < p_xml_info->golds_cost) {
			T_KWARN_LOG(owner->user_id, "horse train golds not enough, cur_golds=%u, need_golds=%u", owner->golds, p_xml_info->golds_cost);
			return cli_not_enough_golds_err;
		}
		//扣除金币
		owner->chg_golds(-p_xml_info->golds_cost);

		add_exp = p_xml_info->golds_exp;
		uint32_t r = rand() % 100;
		if (r < p_xml_info->golds_crit2_prob) {
			cri_flag = 1;
			add_exp *= 2;
		}
	} else {
		uint32_t diamond_tms = owner->res_mgr->get_res_value(daily_horse_train_diamond_tms);
		if (diamond_tms >= 20) {
			T_KWARN_LOG(owner->user_id, "horse train daily tms is over, type=%u, diamond_tms=%u", type, diamond_tms);
			return cli_horse_train_tms_not_enough_err;
		}
		//检查钻石是否足够
		if (owner->diamond < p_xml_info->diamond_cost) {
			T_KWARN_LOG(owner->user_id, "horse train diamond not enough, cur_diamond=%u, need_diamond=%u", owner->diamond, p_xml_info->diamond_cost);
			return cli_not_enough_diamond_err;
		}

		//扣除钻石
		owner->chg_diamond(-p_xml_info->diamond_cost);

		add_exp = p_xml_info->diamond_exp;
		uint32_t r = rand() % 100;
		if (r < p_xml_info->diamond_crit10_prob) {
			cri_flag = 2;
			add_exp *= 10;
		} else if (r < p_xml_info->diamond_crit2_prob) {
			cri_flag = 1;
			add_exp *= 2;
		}
	}

	if (add_exp > 0) {
		this->add_exp(add_exp);
	}

	T_KTRACE_LOG(owner->user_id, "HORSE TRAIN\t[type=%u, add_exp=%u]", type, add_exp);

	return 0;
}

int
Horse::horse_use_exp_items(std::vector<cli_item_info_t> &items)
{
	if (!check_is_can_add_exp()) {//未曾突破则不能升级
		return cli_horse_not_breakthrough_err;
	}

	for (uint32_t i = 0; i < items.size(); i++) {
		cli_item_info_t *p_info = &(items[i]);
		const item_xml_info_t *p_xml_info = items_xml_mgr->get_item_xml_info(p_info->item_id);
		if (!p_xml_info) {
			return cli_invalid_item_err;
		}
		if (p_xml_info->type != em_item_type_for_horse_exp) {
			T_KWARN_LOG(owner->user_id, "horse use items err, type err, type=%u", p_xml_info->type);
			return cli_invalid_item_type_err;
		}
		uint32_t cur_cnt = owner->items_mgr->get_item_cnt(p_info->item_id);
		if (cur_cnt < p_info->item_cnt) {
			T_KWARN_LOG(owner->user_id, "horse use items err, items not enough, cur_cnt=%u, cnt=%u", cur_cnt, p_info->item_cnt);
			return cli_not_enough_item_err;
		}
	}

	uint32_t add_exp = 0;
	for (uint32_t i = 0; i < items.size(); i++) {
		cli_item_info_t *p_info = &(items[i]);
		const item_xml_info_t *p_xml_info = items_xml_mgr->get_item_xml_info(p_info->item_id);
		if (p_xml_info) {
			owner->items_mgr->del_item_without_callback(p_info->item_id, p_info->item_cnt);
			add_exp += (p_xml_info->effect * p_info->item_cnt);
		}
	}

	if (add_exp > 0) {
		this->add_exp(add_exp);
	}

	T_KTRACE_LOG(owner->user_id, "HORSE USE EXP ITEMS\t[add_exp=%u]", add_exp);

	return 0;
}

int
Horse::put_on_equip(uint32_t get_tm)
{
	HorseEquip* p_equip = owner->horse_equip_mgr->get_horse_equip(get_tm);
	if (!p_equip) {
		T_KWARN_LOG(owner->user_id, "horse equip not exists\t[get_tm=%u]", get_tm);
		return cli_equip_not_exist_err;
	}

	const horse_equip_xml_info_t *p_xml_info = horse_equip_xml_mgr->get_horse_equip_xml_info(p_equip->id);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "horse equip invalid\t[equip_id=%u]", p_equip->id);
		return cli_invalid_equip_err;
	}

	HorseEquip *p_horse_equip = get_horse_equip(get_tm);
	if (p_horse_equip) {
		T_KWARN_LOG(owner->user_id, "horse equip already put on\t[get_tm=%u]", get_tm);
		return cli_equip_already_put_on_err;
	}

	//更新缓存
	p_equip->state = 1;
	horse_equips.insert(HorseEquipMap::value_type(get_tm, p_equip));

	//更新DB
	db_hero_put_on_equipment_in db_in;
	db_in.hero_id = 1;
	db_in.get_tm = get_tm;
	send_msg_to_dbroute(0, db_hero_put_on_equipment_cmd, &db_in, owner->user_id);

	//重新计算属性 
	calc_all();

	//通知前端
	send_horse_attr_change_noti();

	return 0;
}

int
Horse::put_off_equip(uint32_t get_tm)
{
	HorseEquip* p_equip = owner->horse_equip_mgr->get_horse_equip(get_tm);
	if (!p_equip) {
		T_KWARN_LOG(owner->user_id, "horse equip not exists\t[get_tm=%u]", get_tm);
		return cli_equip_not_exist_err;
	}

	const horse_equip_xml_info_t *p_xml_info = horse_equip_xml_mgr->get_horse_equip_xml_info(p_equip->id);
	if (!p_xml_info) {
		T_KWARN_LOG(owner->user_id, "horse equip invalid\t[equip_id=%u]", p_equip->id);
		return cli_invalid_equip_err;
	}

	HorseEquip *p_horse_equip = get_horse_equip(get_tm);
	if (!p_horse_equip) {
		T_KWARN_LOG(owner->user_id, "horse equip already put off\t[get_tm=%u]", get_tm);
		return cli_equip_already_put_off_err;
	}

	//更新缓存
	p_equip->state = 0;
	horse_equips.erase(get_tm);

	//更新DB
	db_hero_put_off_equipment_in db_in;
	db_in.get_tm = get_tm;
	send_msg_to_dbroute(0, db_hero_put_off_equipment_cmd, &db_in, owner->user_id);

	//重新计算属性 
	calc_all();

	//通知前端
	send_horse_attr_change_noti();
	

	return 0;
}

bool
Horse::check_horse_is_breakthrough()
{
	if (lv < 10 || lv >= MAX_HORSE_LEVEL || lv % 10 != 0) {
		return true;
	}

	uint32_t rank = lv / 10;
	int out_flag = owner->res_mgr->get_res_value(forever_horse_train_out_flag);
	if (test_bit_on(out_flag, rank)) {
		return true;
	}

	return false;
}

int
Horse::horse_breakthrough()
{
	if (lv < 10 || lv >= MAX_HORSE_LEVEL || lv % 10 != 0) {
		return cli_horse_breakthrough_lv_err;
	}
	uint32_t rank = lv / 10;
	int out_flag = owner->res_mgr->get_res_value(forever_horse_train_out_flag);
	if (test_bit_on(out_flag, rank)) {
		return cli_horse_already_breakthrough_err;
	}

	//检查金币是否足够
	uint32_t cost_golds[] = {10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000};
	if (owner->golds < cost_golds[rank-1]) {
		return cli_not_enough_golds_err;
	}
	//扣除金币
	owner->chg_golds(0 - cost_golds[rank-1]);

	//设置out_flag
	out_flag = set_bit_on(out_flag, rank);
	owner->res_mgr->set_res_value(forever_horse_train_out_flag, out_flag);

	//触发升级
	this->add_exp(0, true);

	return 0;
}

void 
Horse::pack_horse_attr_info(cli_horse_attr_info_t &attr_info)
{
	attr_info.max_hp = max_hp;
	attr_info.ad = ad;
	attr_info.armor = armor;
	attr_info.resist = resist;
	attr_info.ad_cri = ad_cri;
	attr_info.ad_ren = ad_ren;
	attr_info.cri_damage = cri_damage;
	attr_info.cri_avoid = cri_avoid;
	attr_info.hit = hit;
	attr_info.miss = miss;
	attr_info.out_max_hp = out_max_hp;
	attr_info.out_ad = out_ad;
	attr_info.out_armor = out_armor;
	attr_info.out_resist = out_resist;
	attr_info.soldier_max_hp = soldier_max_hp;
	attr_info.soldier_ad = soldier_ad;

	T_KTRACE_LOG(owner->user_id, "pack horse attr info\t[%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u]", 
			max_hp, ad, armor, resist, ad_cri, ad_ren, cri_damage, cri_avoid, hit, miss, 
			out_max_hp, out_ad, out_armor, out_resist, soldier_max_hp, soldier_ad);
}

void 
Horse::pack_horse_equips_info(vector<cli_equip_info_t> &equip_vec)
{
	HorseEquipMap::iterator it = horse_equips.begin();
	for (; it != horse_equips.end(); ++it) {
		HorseEquip *p_equip = it->second;
		cli_equip_info_t info;
		memset(&info, 0, sizeof(info));
		info.equip_id = p_equip->id;
		info.get_tm = p_equip->get_tm;
		equip_vec.push_back(info);
	}
}

void 
Horse::pack_horse_info(cli_get_horse_info_out &out)
{
	uint32_t golds_tms = owner->res_mgr->get_res_value(daily_horse_train_golds_tms);
	uint32_t diamond_tms = owner->res_mgr->get_res_value(daily_horse_train_diamond_tms);
	out.horse_lv = lv;
	out.horse_exp = exp;
	out.horse_levelup_exp = get_levelup_exp(lv);
	out.golds_left_tms = golds_tms < 20 ? 20 - golds_tms : 0;
	out.diamond_left_tms = diamond_tms < 20 ? 20 - diamond_tms : 0;
	out.breakthrough_flag = check_horse_is_breakthrough() ? 1 : 0;
	pack_horse_attr_info(out.horse_attr);
	pack_horse_equips_info(out.horse_equips);

	T_KTRACE_LOG(owner->user_id, "pack horse info\t[%u %u %u %u]", 
			out.horse_lv, out.horse_exp, out.golds_left_tms, out.diamond_left_tms);
}

/********************************************************************************/
/*							HorseEquip Class									*/
/********************************************************************************/
HorseEquip::HorseEquip(Player *p, uint32_t equip_id) : owner(p), id(equip_id)
{
	get_tm = 0;
	state = 0;
	base_info = 0;
}

HorseEquip::~HorseEquip()
{

}

/********************************************************************************/
/*							HorseEquipManager Class								*/
/********************************************************************************/
HorseEquipManager::HorseEquipManager(Player *p) : owner(p)
{

}

HorseEquipManager::~HorseEquipManager()
{
	HorseEquipMap::iterator it = horse_equips.begin();
	while (it != horse_equips.end()) {
		HorseEquip *p_equip = it->second;
		horse_equips.erase(it++);
		SAFE_DELETE(p_equip);
	}
}

int
HorseEquipManager::init_horse_equips_info(db_get_player_items_info_out *p_out)
{
	for (uint32_t i = 0; i < p_out->equips.size(); i++) {
		db_equip_info_t *p_info = &(p_out->equips[i]);
		const horse_equip_xml_info_t *base_info = horse_equip_xml_mgr->get_horse_equip_xml_info(p_info->equip_id);
		if (base_info) {
			HorseEquip *p_equip = new HorseEquip(owner, p_info->equip_id);
			p_equip->get_tm = p_info->get_tm;
			p_equip->state = p_info->hero_id ? 1 : 0;
			p_equip->base_info = base_info;
			horse_equips.insert(HorseEquipMap::value_type(p_equip->get_tm, p_equip));
			if (p_info->hero_id) {
				owner->horse->horse_equips.insert(HorseEquipMap::value_type(p_equip->get_tm, p_equip));
			}

			T_TRACE_LOG("init horse equips info\t[%u %u %u]", p_equip->id, p_equip->get_tm, p_equip->state);
		}
	}

	owner->horse->calc_all();
	owner->horse->send_horse_attr_change_noti();

	return 0;
}

HorseEquip*
HorseEquipManager::get_horse_equip(uint32_t get_tm)
{
	HorseEquipMap::iterator it = horse_equips.find(get_tm);
	if (it != horse_equips.end()) {
		return it->second;
	}

	return 0;
}

HorseEquip*
HorseEquipManager::add_one_horse_equip(uint32_t id)
{
	const horse_equip_xml_info_t *p_xml_info = horse_equip_xml_mgr->get_horse_equip_xml_info(id);
	if (!p_xml_info) {
		return 0;
	}

	uint32_t now_sec = get_now_tv()->tv_sec;
	HorseEquipMap::iterator it;
	while ((it = horse_equips.find(now_sec)) != horse_equips.end()) {
		now_sec++;
	}

	HorseEquip *p_equip = new HorseEquip(owner, p_xml_info->id);
	p_equip->get_tm = now_sec;
	p_equip->state = 0;
	p_equip->base_info = p_xml_info;
	horse_equips.insert(HorseEquipMap::value_type(p_equip->get_tm, p_equip));

	//更新DB
	db_add_equip_in db_in;
	db_in.equip_id = id;
	db_in.get_tm = now_sec;
	send_msg_to_dbroute(0, db_add_equip_cmd, &db_in, owner->user_id);

	return p_equip;
}

void
HorseEquipManager::pack_all_horse_equips_info(std::vector<cli_equip_info_t> &equip_vec)
{
	HorseEquipMap::iterator it = horse_equips.begin();
	for (; it != horse_equips.end(); ++it) {
		HorseEquip *p_equip = it->second;
		if (p_equip->state == 0) {
			cli_equip_info_t info;
			memset(&info, 0, sizeof(info));
			info.equip_id = p_equip->id;
			info.get_tm = p_equip->get_tm;
			equip_vec.push_back(info);
			T_KTRACE_LOG(owner->user_id, "PACK ALL HORSE EQUIPS INFO\t[%u %u]", info.equip_id, info.get_tm);
		}
	}
}


/********************************************************************************/
/*							HorseAttrXmlManager									*/
/********************************************************************************/
HorseAttrXmlManager::HorseAttrXmlManager()
{

}

HorseAttrXmlManager::~HorseAttrXmlManager()
{

}

const horse_attr_xml_info_t*
HorseAttrXmlManager::get_horse_attr_xml_info(uint32_t lv)
{
	HorseAttrXmlMap::iterator it = horse_map.find(lv);
	if (it != horse_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
HorseAttrXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_horse_attr_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
HorseAttrXmlManager::load_horse_attr_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("horse"))) {
			uint32_t lv = 0;
			get_xml_prop(lv, cur, "lv");
			HorseAttrXmlMap::iterator it = horse_map.find(lv);
			if (it != horse_map.end()) {
				ERROR_LOG("load horse attr xml info err, lv exist, lv=%u", lv);
				return -1;
			}
			horse_attr_xml_info_t info;
			info.lv = lv;
			get_xml_prop(info.max_hp, cur, "maxhp");
			get_xml_prop(info.ad, cur, "ad");
			get_xml_prop(info.armor, cur, "armor");
			get_xml_prop(info.resist, cur, "resist");
			get_xml_prop(info.out_max_hp, cur, "out_maxhp");
			get_xml_prop(info.out_ad, cur, "out_ad");
			get_xml_prop(info.out_armor, cur, "out_armor");
			get_xml_prop(info.out_resist, cur, "out_resist");
			get_xml_prop(info.soldier_max_hp, cur, "soldier_maxhp");
			get_xml_prop(info.soldier_ad, cur, "soldier_ad");

			TRACE_LOG("load horse attr xml info\t[%u %f %f %f %f %f %f %f %f %u %u]",
					info.lv, info.max_hp, info.ad, info.armor, info.resist, info.out_max_hp, info.out_ad, info.out_armor, info.out_resist, 
					info.soldier_max_hp, info.soldier_ad);

			horse_map.insert(HorseAttrXmlMap::value_type(lv, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							HorseExpXmlManager									*/
/********************************************************************************/
HorseExpXmlManager::HorseExpXmlManager()
{

}

HorseExpXmlManager::~HorseExpXmlManager()
{

}

const horse_exp_xml_info_t*
HorseExpXmlManager::get_horse_exp_xml_info(uint32_t lv)
{
	HorseExpXmlMap::iterator it = horse_exp_map.find(lv);
	if (it != horse_exp_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
HorseExpXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_horse_exp_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
HorseExpXmlManager::load_horse_exp_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("horse"))) {
			uint32_t lv = 0;
			get_xml_prop(lv, cur, "lv");
			HorseExpXmlMap::iterator it = horse_exp_map.find(lv);
			if (it != horse_exp_map.end()) {
				ERROR_LOG("load horse exp xml info, lv exist, lv=%u", lv);
				return -1;
			}
			horse_exp_xml_info_t info;
			info.lv = lv;
			get_xml_prop(info.golds_cost, cur, "golds_cost");
			get_xml_prop(info.golds_exp, cur, "golds_exp");
			get_xml_prop(info.golds_crit2_prob, cur, "golds_crit2_prob");
			get_xml_prop(info.diamond_cost, cur, "diamond_cost");
			get_xml_prop(info.diamond_exp, cur, "diamond_exp");
			get_xml_prop(info.diamond_crit2_prob, cur, "diamond_crit2_prob");
			get_xml_prop(info.diamond_crit10_prob, cur, "diamond_crit10_prob");
			get_xml_prop(info.exp, cur, "exp");

			TRACE_LOG("load horse exp xml info\t[%u %u %u %u %u %u %u %u %u]", 
					info.lv, info.golds_cost, info.golds_exp, info.golds_crit2_prob, info.diamond_cost, info.diamond_exp, 
					info.diamond_crit2_prob, info.diamond_crit10_prob, info.exp);

			horse_exp_map.insert(HorseExpXmlMap::value_type(lv, info));
		}
		cur = cur->next;
	}
	return 0;
}

/********************************************************************************/
/*							HorseEquipXmlManager								*/
/********************************************************************************/
HorseEquipXmlManager::HorseEquipXmlManager()
{

}

HorseEquipXmlManager::~HorseEquipXmlManager()
{

}

const horse_equip_xml_info_t*
HorseEquipXmlManager::get_horse_equip_xml_info(uint32_t id)
{
	HorseEquipXmlMap::iterator it = horse_equip_map.find(id);
	if (it != horse_equip_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
HorseEquipXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_horse_equip_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
HorseEquipXmlManager::load_horse_equip_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("horseEquip"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			HorseEquipXmlMap::iterator it = horse_equip_map.find(id);
			if (it != horse_equip_map.end()) {
				ERROR_LOG("load horse equip xml info err, id exist, id=%u", id);
				return -1;
			}

			horse_equip_xml_info_t info;
			info.id = id;
			get_xml_prop(info.max_hp, cur, "maxhp");
			get_xml_prop(info.ad, cur, "ad");
			get_xml_prop(info.armor, cur, "armor");
			get_xml_prop(info.resist, cur, "resist");
			get_xml_prop(info.ad_cri, cur, "ad_cri");
			get_xml_prop(info.ad_ren, cur, "ad_ren");
			get_xml_prop(info.cri_damage, cur, "cri_damage");
			get_xml_prop(info.cri_avoid, cur, "cri_avoid");
			get_xml_prop(info.hit, cur, "hit");
			get_xml_prop(info.miss, cur, "miss");

			TRACE_LOG("load horse equip xml info\t[%u %u %u %u %u %u %u %u %u %u %u]", 
					info.id, info.max_hp, info.ad, info.armor, info.resist, info.ad_cri, info.ad_ren, 
					info.cri_damage, info.cri_avoid, info.hit, info.miss);

			horse_equip_map.insert(HorseEquipXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							Client Request										*/
/********************************************************************************/
/* @brief 拉取战马信息
 */
int cli_get_horse_info(Player *p, Cmessage *c_in)
{
	cli_get_horse_info_out cli_out;
	p->horse->pack_horse_info(cli_out);

	T_KDEBUG_LOG(p->user_id, "GET HORSE INFO");

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 战马培养
 */
int cli_horse_train(Player *p, Cmessage *c_in)
{
	cli_horse_train_in *p_in = P_IN;
	
	uint32_t cri_flag = 0;
	int ret = p->horse->horse_train(p_in->type, cri_flag);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_horse_train_out cli_out;
	uint32_t golds_tms = p->res_mgr->get_res_value(daily_horse_train_golds_tms);
	uint32_t diamond_tms = p->res_mgr->get_res_value(daily_horse_train_diamond_tms);
	cli_out.golds_left_tms = golds_tms < 20 ? 20 - golds_tms : 0;
	cli_out.diamond_left_tms = diamond_tms < 20 ? 20 - diamond_tms : 0;
	cli_out.cri_flag = cri_flag;

	T_KDEBUG_LOG(p->user_id, "HORSE TRAIN\t[type=%u]", p_in->type);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 战马使用道具
 */
int cli_horse_use_exp_items(Player *p, Cmessage *c_in)
{
	cli_horse_use_exp_items_in *p_in = P_IN;

	int ret = p->horse->horse_use_exp_items(p_in->items);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "HORSE USE ITEMS!");

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 战马穿上装备
 */
int cli_horse_put_on_equip(Player *p, Cmessage *c_in)
{
	cli_horse_put_on_equip_in *p_in = P_IN;

	int ret = p->horse->put_on_equip(p_in->get_tm);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "HORSE PUT ON EQUIP\t[get_tm=%u]", p_in->get_tm);

	cli_horse_put_on_equip_out cli_out;
	cli_out.get_tm = p_in->get_tm;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 战马卸下装备
 */
int cli_horse_put_off_equip(Player *p, Cmessage *c_in)
{
	cli_horse_put_off_equip_in *p_in = P_IN;

	int ret = p->horse->put_off_equip(p_in->get_tm);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "HORSE PUT OFF EQUIP\t[get_tm=%u]", p_in->get_tm);

	cli_horse_put_off_equip_out cli_out;
	cli_out.get_tm = p_in->get_tm;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 战马突破
 */
int cli_horse_breakthrough(Player *p, Cmessage *c_in)
{
	int ret = p->horse->horse_breakthrough();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "HORSE TRAIN BREAKTHROUGH\t[lv=%u]", p->horse->lv);

	return p->send_to_self(p->wait_cmd, 0, 1);
}
