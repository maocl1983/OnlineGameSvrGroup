/*
 * =====================================================================================
 *
 *  @file  btl_soul.cpp 
 *
 *  @brief  战魂系统
 *
 *  compiler  gcc4.4.7 
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

#include "global_data.hpp"
#include "btl_soul.hpp"
#include "player.hpp"
#include "dbroute.hpp"
#include "log_thread.hpp"

//BtlSoulXmlManager btl_soul_xml_mgr;
//BtlSoulLevelXmlManager btl_soul_level_xml_mgr;
//DivineItemXmlManager divine_item_xml_mgr;

using namespace std;
using namespace project;
/********************************************************************************/
/*							BtlSoulManager Class								*/
/********************************************************************************/
BtlSoul::BtlSoul(Player *p, uint32_t btl_soul_id) : owner(p), id(btl_soul_id)
{
	get_tm = 0;
	lv= 0;
	exp = 0;
	hero_id = 0;
	tmp = 0;
	base_info = btl_soul_xml_mgr->get_btl_soul_xml_info(id);
}

BtlSoul::~BtlSoul()
{

}


/********************************************************************************/
/*							BtlSoulManager Class								*/
/********************************************************************************/
BtlSoulManager::BtlSoulManager(Player *p) : owner(p)
{
	btl_soul_map.clear();
	hero_btl_soul_map.clear();
	divine_btl_soul_map.clear();
}

BtlSoulManager::~BtlSoulManager()
{
	hero_btl_soul_map.clear();
	divine_btl_soul_map.clear();
	BtlSoulMap::iterator it = btl_soul_map.begin();
	while (it != btl_soul_map.end()) {
		BtlSoul *btl_soul = it->second;
		btl_soul_map.erase(it++);
		SAFE_DELETE(btl_soul);
	}
}

int 
BtlSoulManager::init_btl_soul_list(db_get_player_btl_soul_list_out *p_out)
{
	for (uint32_t i = 0; i < p_out->btl_soul_list.size(); i++) {
		db_btl_soul_info_t *p_info = &(p_out->btl_soul_list[i]);
		const btl_soul_xml_info_t *base_info = btl_soul_xml_mgr->get_btl_soul_xml_info(p_info->id);
		if (base_info) {
			BtlSoul *p_btl_soul = new BtlSoul(owner, base_info->id);
			p_btl_soul->get_tm = p_info->get_tm;
			p_btl_soul->hero_id = p_info->hero_id;
			p_btl_soul->lv = p_info->lv;
			p_btl_soul->exp = p_info->exp;
			p_btl_soul->tmp = p_info->tmp;
			p_btl_soul->base_info = btl_soul_xml_mgr->get_btl_soul_xml_info(p_info->id);
			
			btl_soul_map.insert(BtlSoulMap::value_type(p_btl_soul->get_tm, p_btl_soul));

			if (p_btl_soul->hero_id) {
				HeroBtlSoulMap::iterator it = hero_btl_soul_map.find(p_btl_soul->hero_id);
				if (it == hero_btl_soul_map.end()) {
					vector<BtlSoul*> btl_soul_list;
					btl_soul_list.push_back(p_btl_soul);
					hero_btl_soul_map.insert(HeroBtlSoulMap::value_type(p_btl_soul->hero_id, btl_soul_list));
				} else {
					it->second.push_back(p_btl_soul);
				}
			}

			//如果在占星背包，则同时插入占星map
			if (p_btl_soul->tmp) {
				divine_btl_soul_map.insert(BtlSoulMap::value_type(p_btl_soul->get_tm, p_btl_soul));
			}

			T_KTRACE_LOG(owner->user_id, "init btl soul info\t[%u %u %u %u %u %u]", 
					p_btl_soul->id, p_btl_soul->get_tm, p_btl_soul->hero_id, p_btl_soul->lv, p_btl_soul->exp, p_btl_soul->tmp);
		}
	}

	return 0;
}

int
BtlSoulManager::init_hero_btl_soul_list()
{
	HeroBtlSoulMap::iterator it = hero_btl_soul_map.begin();
	for (; it != hero_btl_soul_map.end(); ++it) {
		Hero *p_hero = owner->hero_mgr->get_hero(it->first);
		if (p_hero) {
			vector<BtlSoul*>::iterator it2 = it->second.begin();
			for (; it2 != it->second.end(); ++it2) {
				BtlSoul *btl_soul = *it2;
				p_hero->btl_souls.insert(BtlSoulMap::value_type(btl_soul->get_tm, btl_soul));
			}
			p_hero->calc_hero_btl_power();
		}
	}

	//更新完重新计算战斗力
	owner->calc_btl_power();

	return 0;
}

BtlSoul *
BtlSoulManager::get_btl_soul(uint32_t get_tm)
{
	BtlSoulMap::iterator it = btl_soul_map.find(get_tm);
	if (it != btl_soul_map.end()) {
		return it->second;
	}

	return 0;
}

int
BtlSoulManager::btl_soul_level_up(uint32_t get_tm)
{
	BtlSoul *p_btl_soul = get_btl_soul(get_tm);
	if (!p_btl_soul) {
		T_KWARN_LOG(owner->user_id, "btl soul not exists\t[get_tm=%u]", get_tm);
		return cli_btl_soul_not_exist_err;
	}

	if (p_btl_soul->lv >= MAX_BTL_SOUL_LEVEL) {
		T_KWARN_LOG(owner->user_id, "btl soul lv over max\t[lv=%u]", p_btl_soul->lv);
		return cli_btl_soul_already_reach_max_lv_err;
	}

	//检查经验是否足够
	uint32_t need_exp = btl_soul_level_xml_mgr->get_btl_soul_levelup_exp(p_btl_soul->base_info->rank, p_btl_soul->lv);
	if (owner->btl_soul_exp < need_exp) {
		T_KWARN_LOG(owner->user_id, "btl soul level up need exp not enough\t[cur_exp=%u, need_exp=%u]", owner->btl_soul_exp, need_exp);
		return cli_btl_soul_exp_not_enough_err;
	}

	//扣除经验
	owner->chg_btl_soul_exp(-need_exp);

	//升级
	p_btl_soul->lv++;

	//更新DB
	db_update_btl_soul_level_in db_in;
	db_in.get_tm = p_btl_soul->get_tm;
	db_in.lv = p_btl_soul->lv;
	send_msg_to_dbroute(0, db_update_btl_soul_level_cmd, &db_in, owner->user_id);

	return 0;
}

int
BtlSoulManager::get_divine_btl_soul_cnt()
{
	return divine_btl_soul_map.size();
}

int
BtlSoulManager::divine_exec(uint32_t divine_id, uint32_t &btl_soul_id)
{
	if (!divine_id || divine_id > 5) {
		return 0;
	}

	btl_soul_id = divine_item_xml_mgr->random_divine_item(divine_id);

	uint32_t next_divine_id = 1;
	uint32_t next_prob[] = {50, 40, 30, 10, 0};
  	uint32_t r = rand() % 100;
	if (r < next_prob[divine_id - 1]) {
		next_divine_id = divine_id + 1;
	}

	return next_divine_id;	
}

int
BtlSoulManager::divine_request(uint32_t divine_id, cli_divine_request_out &out)
{
	if (!divine_id || divine_id > 5) {
		T_KWARN_LOG(owner->user_id, "divine request id err\t[id=%u]", divine_id);
		return cli_invalid_input_arg_err;
	}
	uint32_t cur_divine_id = owner->res_mgr->get_res_value(forever_divine_id);
	if (cur_divine_id != divine_id) {
		T_KWARN_LOG(owner->user_id, "divine request id not match\t[cur_id=%u, id=%u]", cur_divine_id, divine_id);
		return cli_divine_id_not_match_err;
	}

	uint32_t need_golds[] = {0, 4000, 8000, 12000, 20000, 30000};
	if (owner->golds < need_golds[divine_id]) {
		T_KWARN_LOG(owner->user_id, "divine request golds not enough\t[cur_golds=%u, need_golds=%u]", owner->golds, need_golds[divine_id]);
		return cli_not_enough_golds_err;
	}

	//检查占星背包是否已满
	if (this->get_divine_btl_soul_cnt() >= 10) {
		T_KWARN_LOG(owner->user_id, "divine request bag is full");
		return cli_divine_btl_soul_bag_alreay_full_err;
	}
	
	uint32_t btl_soul_id = 0;
	uint32_t next_divine_id = divine_exec(divine_id, btl_soul_id);

	BtlSoul *p_btl_soul = this->add_btl_soul(btl_soul_id, 1);
	if (!p_btl_soul) {
		T_KWARN_LOG(owner->user_id, "divine request btl soul id err\t[btl_soul_id=%u]", btl_soul_id);
		return cli_invalid_input_arg_err;
	}

	out.divine_id = next_divine_id;
	out.btl_soul.id = btl_soul_id;
	out.btl_soul.get_tm = p_btl_soul->get_tm;

	//设置占星ID
	owner->res_mgr->set_res_value(forever_divine_id, next_divine_id);

	T_KTRACE_LOG(owner->user_id, "divine request\t[%u %u %u]", next_divine_id, p_btl_soul->id, p_btl_soul->get_tm);

	return 0;
}

int
BtlSoulManager::divine_request_one_key(uint32_t divine_id, cli_divine_request_one_key_out &out)
{
	if (!divine_id || divine_id > 5) {
		T_KWARN_LOG(owner->user_id, "divine request id err\t[id=%u]", divine_id);
		return cli_invalid_input_arg_err;
	}
	uint32_t cur_divine_id = owner->res_mgr->get_res_value(forever_divine_id);
	if (cur_divine_id != divine_id) {
		T_KWARN_LOG(owner->user_id, "divine request id not match\t[cur_id=%u, id=%u]", cur_divine_id, divine_id);
		return cli_divine_id_not_match_err;
	}

	//检测金币是否足够
	uint32_t need_golds[] = {0, 4000, 8000, 12000, 20000, 30000};
	uint32_t max_golds = 0;
	for (int i = 0; i < 6; i++) {
		max_golds += need_golds[i];
	}
	max_golds *= 2;
	//按最好情况算
	if (owner->golds < max_golds) {
		T_KWARN_LOG(owner->user_id, "divine request golds not enough\t[cur_golds=%u, need_golds=%u]", owner->golds, max_golds);
		return cli_not_enough_golds_err;
	}

	//检测临时背包是否还有残留
	if (this->get_divine_btl_soul_cnt() > 0) {
		return cli_divine_btl_soul_bag_not_empty_err;
	}

	//db_add_btl_soul_list_in db_in;
	uint32_t final_divine_id = 0;
	for (int i = 0; i < 10; i++) {
		uint32_t btl_soul_id = 0;
		uint32_t next_divine_id = divine_exec(divine_id, btl_soul_id);

		BtlSoul *p_btl_soul = this->add_btl_soul(btl_soul_id, 1);
		if (!p_btl_soul) {
			T_KWARN_LOG(owner->user_id, "btl soul not exist\t[id=%u]", btl_soul_id);
			return cli_btl_soul_not_exist_err;
		}

		out.divine_list.push_back(next_divine_id);
		cli_btl_soul_info_t out_info;
		out_info.id = btl_soul_id;
		out_info.get_tm = p_btl_soul->get_tm;
		out.soul_list.push_back(out_info);
		if (i == 9) {
			final_divine_id = next_divine_id;
		}
		T_KTRACE_LOG(owner->user_id, "divine request one key\t[%u %u %u]", next_divine_id, p_btl_soul->id, p_btl_soul->get_tm);
	}

	//更新DB TODO
	//send_msg_to_dbroute(0, db_add_btl_soul_list_cmd, &db_in, owner->user_id);

	//设置占星ID
	owner->res_mgr->set_res_value(forever_divine_id, final_divine_id);

	return 0;
}

int
BtlSoulManager::add_btl_soul_from_tmp_to_bag(uint32_t get_tm, cli_add_btl_soul_to_bag_out &out)
{
	BtlSoul *btl_soul = this->get_btl_soul(get_tm);
	if (!btl_soul) {
		T_KWARN_LOG(owner->user_id, "btl soul not exist\t[get_tm=%u]", get_tm);
		return cli_btl_soul_not_exist_err;
	}

	divine_btl_soul_map.erase(btl_soul->get_tm);
	btl_soul->tmp = 0;

	//更新DB
	db_remove_btl_soul_from_tmp_bag_in db_in;
	db_in.get_tm = get_tm;
	send_msg_to_dbroute(0, db_remove_btl_soul_from_tmp_bag_cmd, &db_in, owner->user_id);

	//重新打包占星背包信息
	BtlSoulMap::iterator it = divine_btl_soul_map.begin();
	for (; it != divine_btl_soul_map.end(); ++it) {
		cli_btl_soul_info_t info;
		info.id = it->second->id;
		info.get_tm = it->second->get_tm;
		out.soul_list.push_back(info);
	}

	return 0;
}

int
BtlSoulManager::add_btl_soul_from_tmp_to_bag_one_key() 
{
	db_remove_btl_soul_from_tmp_bag_list_in db_in;
	BtlSoulMap::iterator it = divine_btl_soul_map.begin();
	while (it != divine_btl_soul_map.end()) {
		BtlSoul *btl_soul = it->second;
		btl_soul->tmp = 0;
		divine_btl_soul_map.erase(it++);
		db_in.remove_list.push_back(btl_soul->get_tm);
	}

	send_msg_to_dbroute(0, db_remove_btl_soul_from_tmp_bag_list_cmd, &db_in, owner->user_id);

	return 0;
}

BtlSoul*
BtlSoulManager::add_btl_soul(uint32_t id, uint32_t tmp)
{
	const btl_soul_xml_info_t *p_xml_info = btl_soul_xml_mgr->get_btl_soul_xml_info(id);
	if (!p_xml_info) {
		return 0;
	}

	uint32_t now_sec = time(0);
	BtlSoulMap::iterator it;
	while ((it = btl_soul_map.find(now_sec)) != btl_soul_map.end()) {
		now_sec++;
	}

	BtlSoul *btl_soul = new BtlSoul(owner, id);
	btl_soul->get_tm = now_sec;
	btl_soul->hero_id = 0;
	btl_soul->lv = 1;
	btl_soul->exp = 0;
	btl_soul->base_info = p_xml_info;
	btl_soul_map.insert(BtlSoulMap::value_type(btl_soul->get_tm, btl_soul));

	if (tmp) {
		divine_btl_soul_map.insert(BtlSoulMap::value_type(btl_soul->get_tm, btl_soul));
	}

	//更新DB
	db_add_btl_soul_in db_in;
	db_in.id = btl_soul->id;
	db_in.get_tm = btl_soul->get_tm;
	db_in.tmp = tmp;
	send_msg_to_dbroute(0, db_add_btl_soul_cmd, &db_in, owner->user_id);

	T_KDEBUG_LOG(owner->user_id, "ADD BTL SOUL\t[id=%u, get_tm=%u, tmp=%u]", btl_soul->id, btl_soul->get_tm, tmp);

	return btl_soul;
}

int
BtlSoulManager::add_btl_soul(std::vector<uint32_t> &btl_soul_vec)
{
	db_add_btl_soul_list_in db_in;
	for (uint32_t i = 0; i < btl_soul_vec.size(); i++) {
		uint32_t id = btl_soul_vec[i];
		const btl_soul_xml_info_t *base_info = btl_soul_xml_mgr->get_btl_soul_xml_info(id);
		if (!base_info) {
			continue;
		}

		uint32_t now_sec = time(0);

		BtlSoul *btl_soul = new BtlSoul(owner, id);
		btl_soul->get_tm = now_sec;
		btl_soul->hero_id = 0;
		btl_soul->lv = 1;
		btl_soul->exp = 0;
		btl_soul->base_info = base_info;
		BtlSoulMap::iterator it = btl_soul_map.find(btl_soul->get_tm);
		while (it != btl_soul_map.end()) {
			btl_soul->get_tm++;
			it = btl_soul_map.find(btl_soul->get_tm);
		}
		btl_soul_map.insert(BtlSoulMap::value_type(btl_soul->get_tm, btl_soul));

		db_btl_soul_info_t db_info;
		db_info.id = btl_soul->id;
		db_info.get_tm = btl_soul->get_tm;
		db_in.btl_soul_list.push_back(db_info);
	
		T_KDEBUG_LOG(owner->user_id, "ADD BTL SOUL\t[id=%u, get_tm=%u]", btl_soul->id, btl_soul->get_tm);
	}

	if (db_in.btl_soul_list.size() > 0) {
		send_msg_to_dbroute(0, db_add_btl_soul_list_cmd, &db_in, owner->user_id);
	}

	return 0;
}

int
BtlSoulManager::del_btl_soul(uint32_t get_tm)
{
	BtlSoul *btl_soul = get_btl_soul(get_tm);
	if (!btl_soul) {
		return -1;
	}

	//更新缓存
	btl_soul_map.erase(get_tm);

	BtlSoulMap::iterator it = divine_btl_soul_map.find(get_tm);
	if (it != divine_btl_soul_map.end()) {
		divine_btl_soul_map.erase(get_tm);
	}

	//更新DB
	db_del_btl_soul_in db_in;
	db_in.get_tm = get_tm;
	send_msg_to_dbroute(0, db_del_btl_soul_cmd, &db_in, owner->user_id);

	T_KDEBUG_LOG(owner->user_id, "SWALLOW BTL SOUL\t[id=%u, get_tm=%u]", btl_soul->id, get_tm);

	//删除战魂
	SAFE_DELETE(btl_soul);

	return 0;
}

int
BtlSoulManager::swallow_btl_soul(vector<uint32_t> &swallow_list)
{
	for (uint32_t i = 0; i < swallow_list.size(); i++) {
		uint32_t get_tm = swallow_list[i];
		BtlSoul *btl_soul = get_btl_soul(get_tm);
		if (!btl_soul) { 
			T_KWARN_LOG(owner->user_id, "btl soul not exist\t[get_tm=%u]", get_tm);
			return cli_btl_soul_not_exist_err;
		}
	}

	//更新缓存
	uint32_t add_exp = 0;
	for (uint32_t i = 0; i < swallow_list.size(); i++) {
		uint32_t get_tm = swallow_list[i];
		BtlSoul *btl_soul = get_btl_soul(get_tm);
		this->del_btl_soul(get_tm);
		add_exp += btl_soul->base_info->exp;
	}

	//增加人物战魂经验 
	owner->chg_btl_soul_exp(add_exp);

	return 0;
}

int
BtlSoulManager::swallow_btl_soul_from_divine_bag(vector<uint32_t> &swallow_list, cli_swallow_btl_soul_from_divine_bag_out &out)
{
	for (uint32_t i = 0; i < swallow_list.size(); i++) {
		uint32_t get_tm = swallow_list[i];
		BtlSoul *btl_soul = get_btl_soul(get_tm);
		if (!btl_soul) {
			T_KWARN_LOG(owner->user_id, "btl soul not exist\t[get_tm=%u]", get_tm);
			return cli_btl_soul_not_exist_err;
		}
	}

	uint32_t add_exp = 0;
	db_del_btl_soul_list_in db_in;
	for (uint32_t i = 0; i < swallow_list.size(); i++) {
		uint32_t get_tm = swallow_list[i];
		BtlSoul *btl_soul = get_btl_soul(get_tm);
		if (btl_soul) {
			add_exp += btl_soul->base_info->exp;
			btl_soul_map.erase(get_tm);
			db_in.del_list.push_back(get_tm);
			BtlSoulMap::iterator it = divine_btl_soul_map.find(get_tm);
			if (it != divine_btl_soul_map.end()) {
				divine_btl_soul_map.erase(get_tm);
			}
			SAFE_DELETE(btl_soul);
		}
	}

	owner->chg_btl_soul_exp(add_exp);

	//更新DB
	send_msg_to_dbroute(0, db_del_btl_soul_list_cmd, &db_in, owner->user_id);

	T_KDEBUG_LOG(owner->user_id, "SWALLOW BTL SOUL FROM DIVINE BAG\t[add_exp=%u]", add_exp);

	return 0;
}

void
BtlSoulManager::pack_all_btl_soul_info(cli_get_btl_soul_list_out &out)
{
	BtlSoulMap::iterator it = btl_soul_map.begin();
	for (; it != btl_soul_map.end(); ++it) {
		BtlSoul *btl_soul = it->second;
		if (btl_soul->hero_id == 0) {
			cli_btl_soul_info_t info;
			info.id = btl_soul->id;
			info.get_tm = btl_soul->get_tm;
			info.lv = btl_soul->lv;
			info.exp = btl_soul->exp;

			if (!btl_soul->tmp) {
				out.btl_soul_list.push_back(info);
			} else {
				out.divine_btl_soul_list.push_back(info);
			}

			T_KTRACE_LOG(owner->user_id, "PACK ALL BTL SOUL INFO\t[%u %u %u %u]", info.id, info.get_tm, info.lv, info.exp);
		}
	}
}

/********************************************************************************/
/*							BtlSoulXmlManager Class								*/
/********************************************************************************/
BtlSoulXmlManager::BtlSoulXmlManager()
{

}

BtlSoulXmlManager::~BtlSoulXmlManager()
{

}

const btl_soul_xml_info_t *
BtlSoulXmlManager::get_btl_soul_xml_info(uint32_t id)
{
	BtlSoulXmlMap::const_iterator it = btl_soul_xml_map.find(id);
	if (it != btl_soul_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
BtlSoulXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_btl_soul_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
BtlSoulXmlManager::load_btl_soul_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("btlSoul"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			BtlSoulXmlMap::iterator it = btl_soul_xml_map.find(id);
			if (it != btl_soul_xml_map.end()) {
				ERROR_LOG("load btl soul xml info err, id exist, id=%u", id);
				return -1;
			}

			btl_soul_xml_info_t info;
			info.id = id;
			get_xml_prop(info.type, cur, "type");
			get_xml_prop(info.rank, cur, "rank");
			get_xml_prop(info.effect, cur, "effect");
			get_xml_prop(info.exp, cur, "exp");

			TRACE_LOG("load btl soul xml info\t[%u %u %u %u %u]", info.id, info.type, info.rank, info.effect, info.exp);

			btl_soul_xml_map.insert(BtlSoulXmlMap::value_type(id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							BtlSoulLevelXmlManager Class						*/
/********************************************************************************/
BtlSoulLevelXmlManager::BtlSoulLevelXmlManager()
{

}

BtlSoulLevelXmlManager::~BtlSoulLevelXmlManager()
{

}

int
BtlSoulLevelXmlManager::get_btl_soul_levelup_exp(uint32_t rank, uint32_t lv)
{
	BtlSoulLevelXmlMap::iterator it = rank_map.find(rank);
	if (it == rank_map.end()) {
		return -1;
	}
	BtlSoulLevelXmlLevelMap::iterator it2 = it->second.level_map.find(lv);	
	if (it2 == it->second.level_map.end()) {
		return -1;
	}

	return it2->second.exp;
}

int
BtlSoulLevelXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_btl_soul_level_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
BtlSoulLevelXmlManager::load_btl_soul_level_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("levelup"))) {
			uint32_t rank = 0;
			get_xml_prop(rank, cur, "rank");
			btl_soul_level_xml_level_info_t level_info;
			get_xml_prop(level_info.lv, cur, "lv");
			get_xml_prop(level_info.exp, cur, "exp");

			BtlSoulLevelXmlMap::iterator it = rank_map.find(rank);
			if (it != rank_map.end()) {
				BtlSoulLevelXmlLevelMap::iterator it2 = it->second.level_map.find(level_info.lv);
				if (it2 != it->second.level_map.end()) {
					ERROR_LOG("load btl soul level xml info err, rank=%u, lv=%u", rank, level_info.lv);
					return -1;
				}
				it->second.level_map.insert(BtlSoulLevelXmlLevelMap::value_type(level_info.lv, level_info));
			} else {
				btl_soul_level_xml_info_t info;
				info.rank = rank;
				info.level_map.insert(BtlSoulLevelXmlLevelMap::value_type(level_info.lv, level_info));
				rank_map.insert(BtlSoulLevelXmlMap::value_type(rank, info));
			}

			TRACE_LOG("load btl soul level xml info\t[%u %u %u]", rank, level_info.lv, level_info.exp);
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							DivineItemXmlManager Class							*/
/********************************************************************************/
DivineItemXmlManager::DivineItemXmlManager()
{

}

DivineItemXmlManager::~DivineItemXmlManager()
{

}

const divine_item_xml_info_t *
DivineItemXmlManager::get_divine_item_xml_info(uint32_t divine_id)
{
	DivineItemXmlMap::const_iterator it = divine_item_xml_map.find(divine_id);
	if (it != divine_item_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
DivineItemXmlManager::random_divine_item(uint32_t divine_id)
{
	const divine_item_xml_info_t *p_info = get_divine_item_xml_info(divine_id);
	if (p_info) {
		uint32_t r = rand() % p_info->total_prob;
		uint32_t cur_prob = 0;
		for (uint32_t i = 0; i < p_info->items.size(); i++) {
			const divine_item_detail_xml_info_t *p_item_info = &(p_info->items[i]);
			cur_prob += p_item_info->prob;
			if (r < cur_prob) {
				return p_item_info->item_id;
			}
		}
	}

	return 0;
}

int
DivineItemXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_divine_item_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
DivineItemXmlManager::load_divine_item_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("divine"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			divine_item_detail_xml_info_t sub_info = {};
			get_xml_prop(sub_info.item_id, cur, "item_id");
			get_xml_prop(sub_info.prob, cur, "item_prob");

			DivineItemXmlMap::iterator it = divine_item_xml_map.find(id);
			if (it != divine_item_xml_map.end()) {
				it->second.items.push_back(sub_info);
				it->second.total_prob += sub_info.prob;
			} else {
				divine_item_xml_info_t info;
				info.divine_id = id;
				info.total_prob = sub_info.prob;
				info.items.push_back(sub_info);

				divine_item_xml_map.insert(DivineItemXmlMap::value_type(id, info));
			}

			TRACE_LOG("load divine item xml info\t[%u %u %u]", id, sub_info.item_id, sub_info.prob);
		}
		cur = cur->next;
	}

	return 0;
}


/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 拉取战魂信息
 */
int cli_get_btl_soul_list(Player *p, Cmessage *c_in)
{
	return send_msg_to_dbroute(p, db_get_player_btl_soul_list_cmd, 0, p->user_id);
}

/* @brief 占星
 */
int cli_divine_request(Player *p, Cmessage *c_in)
{
	cli_divine_request_in *p_in = P_IN;

	cli_divine_request_out cli_out;
	int ret = p->btl_soul_mgr->divine_request(p_in->divine_id, cli_out);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "DIVINE REQUEST\t[divine_id=%u, btl_soul=%u]", p_in->divine_id, cli_out.btl_soul.id);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 一键占星
 */
int cli_divine_request_one_key(Player *p, Cmessage *c_in)
{
	cli_divine_request_one_key_in *p_in = P_IN;

	cli_divine_request_one_key_out cli_out;
	int ret = p->btl_soul_mgr->divine_request_one_key(p_in->divine_id, cli_out);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "DIVINE REQUEST ONE KEY\t[divine_id=%u]", p_in->divine_id);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 战魂放入背包
 */
int cli_add_btl_soul_to_bag(Player *p, Cmessage *c_in)
{
	cli_add_btl_soul_to_bag_in *p_in = P_IN;

	cli_add_btl_soul_to_bag_out cli_out;
	int ret = p->btl_soul_mgr->add_btl_soul_from_tmp_to_bag(p_in->get_tm, cli_out);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "ADD BTL SOUL TO BAG\t[get_tm=%u]", p_in->get_tm);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 战魂一键入包
 */
int cli_add_btl_soul_to_bag_one_key(Player *p, Cmessage *c_in)
{
	int ret = p->btl_soul_mgr->add_btl_soul_from_tmp_to_bag_one_key();
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "ADD BTL SOUL TO BAG ONE KEY");

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/* @brief 战魂吞噬
 */
int cli_swallow_btl_soul(Player *p, Cmessage *c_in)
{
	cli_swallow_btl_soul_in *p_in = P_IN;

	int ret = p->btl_soul_mgr->swallow_btl_soul(p_in->swallow_list);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_swallow_btl_soul_out cli_out;
	cli_out.btl_soul_exp = p->btl_soul_exp;
	for (uint32_t i = 0; i < p_in->swallow_list.size(); i++) {
		cli_out.swallow_list.push_back(p_in->swallow_list[i]);
	}

	T_KDEBUG_LOG(p->user_id, "SWALLOW BTL SOUL\t[btl_soul_exp=%u]", p->btl_soul_exp);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 占星背包战魂吞噬
 */
int cli_swallow_btl_soul_from_divine_bag(Player *p, Cmessage *c_in)
{
	cli_swallow_btl_soul_from_divine_bag_in *p_in = P_IN;

	cli_swallow_btl_soul_from_divine_bag_out cli_out;
	int ret = p->btl_soul_mgr->swallow_btl_soul_from_divine_bag(p_in->swallow_list, cli_out);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_out.btl_soul_exp = p->btl_soul_exp;

	T_KDEBUG_LOG(p->user_id, "SWALLOW BTL SOUL FROM DIVINE BAG\t[cnt=%u]", (uint32_t)p_in->swallow_list.size());

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 战魂升级
 */
int cli_btl_soul_level_up(Player *p, Cmessage *c_in)
{
	cli_btl_soul_level_up_in *p_in = P_IN;

	BtlSoul *btl_soul = p->btl_soul_mgr->get_btl_soul(p_in->get_tm);
	if (!btl_soul) {
		T_KWARN_LOG(p->user_id, "btl soul not exist\t[get_tm=%u]", p_in->get_tm);
		return p->send_to_self_error(p->wait_cmd, cli_btl_soul_not_exist_err, 1);
	}

	int ret = p->btl_soul_mgr->btl_soul_level_up(p_in->get_tm);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_btl_soul_level_up_out cli_out;
	cli_out.get_tm = p_in->get_tm;
	cli_out.lv = btl_soul->lv;
	cli_out.btl_soul_exp = p->btl_soul_exp;

	T_KDEBUG_LOG(p->user_id, "BTL SOUL LEVEL UP\t[get_tm=%u, lv=%u, btl_soul_exp=%u]", p_in->get_tm, btl_soul->lv, p->btl_soul_exp);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}


/********************************************************************************/
/*								DB return										*/
/********************************************************************************/
/* @brief 拉取战魂信息返回
 */
int db_get_player_btl_soul_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);
	db_get_player_btl_soul_list_out *p_in = P_IN;
	p->btl_soul_mgr->init_btl_soul_list(p_in);

	if (p->wait_cmd == cli_proto_login_cmd) {
		p->login_step++;
		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u BTL SOUL LIST", p->login_step);

		//战魂信息
		cli_get_btl_soul_list_out cli_out;
		p->btl_soul_mgr->pack_all_btl_soul_info(cli_out);
		p->send_to_self(cli_get_btl_soul_list_cmd, &cli_out, 0); 

		return send_msg_to_dbroute(p, db_get_player_heros_info_cmd, 0, p->user_id);
	} else if (p->wait_cmd == cli_get_btl_soul_list_cmd) {
		cli_get_btl_soul_list_out cli_out;
		p->btl_soul_mgr->pack_all_btl_soul_info(cli_out);

		return p->send_to_self(p->wait_cmd, &cli_out, 1);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}



