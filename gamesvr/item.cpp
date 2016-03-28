/*
 * =====================================================================================
 *
 *  @file  item.cpp
 *
 *  @brief  处理物品相关的信息
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 * copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
/*
#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"
#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"

#include "global_data.hpp"
#include "item.hpp"
#include "player.hpp"
#include "dbroute.hpp"
#include "redis.hpp"
*/

#include "stdafx.hpp"
//ItemsXmlManager items_xml_mgr;
//HeroRankItemXmlManager hero_rank_item_xml_mgr;
//ItemPieceXmlManager item_piece_xml_mgr;
//RandomItemXmlManager random_item_xml_mgr;

using namespace std;
using namespace project;

ItemsManager::ItemsManager(Player *p) : owner(p)
{
	
}

ItemsManager::~ItemsManager()
{

}

int 
ItemsManager::init_items_info(db_get_player_items_info_out *p_out)
{
	vector<db_item_info_t>::iterator it = p_out->items.begin();
	for (; it != p_out->items.end(); it++) {
		item_info_t info;
		info.id = it->item_id;
		info.cnt = it->item_cnt;
		items.insert(ItemsMap::value_type(info.id, info));
		T_KTRACE_LOG(owner->user_id, "init items info\t[%u %u]", info.id, info.cnt);

		const item_piece_xml_info_t *p_piece_xml_info = item_piece_xml_mgr->get_item_piece_xml_info(info.id);
		if (p_piece_xml_info && p_piece_xml_info->type == 2) {//装备碎片
			redis_mgr->set_treasure_piece_user(owner, info.id);
		}
	}

	return 0;
}

uint32_t
ItemsManager::get_item_cnt(uint32_t id)
{
	ItemsMap::iterator it = items.find(id);
	if (it == items.end()) {
		return 0;
	}

	return it->second.cnt;
}

bool
ItemsManager::check_is_valid_item(uint32_t item_id)
{
	const item_xml_info_t *p_info = items_xml_mgr->get_item_xml_info(item_id);
	if (p_info) {
		return true;
	}

	const hero_rank_item_xml_info_t *p_info_2 = hero_rank_item_xml_mgr->get_hero_rank_item_xml_info(item_id);
	if (p_info_2) {
		return true;
	}

	const item_piece_xml_info_t *p_info_3 = item_piece_xml_mgr->get_item_piece_xml_info(item_id);
	if (p_info_3) {
		return true;
	}

	return false;
}

int
ItemsManager::get_item_max_cnt(uint32_t item_id)
{
	const item_xml_info_t *p_info = items_xml_mgr->get_item_xml_info(item_id);
	if (p_info) {
		return p_info->max;
	}

	return 999;
}

int
ItemsManager::add_reward(uint32_t item_id, uint32_t item_cnt)
{
	uint32_t cat = item_id / 100000;
	if (cat == 8) {//装备
		for (uint32_t i = 0; i < item_cnt; i++) {
			owner->equip_mgr->add_one_equip(item_id);
		}
	} else if (cat == 2) {//战魂
		for (uint32_t i = 0; i < item_cnt; i++) {
			owner->btl_soul_mgr->add_btl_soul(item_id);
		}
	} else {//道具
		this->add_item_without_callback(item_id, item_cnt);
	}

	return 0;
}

int
ItemsManager::add_items(uint32_t id, uint32_t add_cnt)
{
	if (!add_cnt) {
		return 0;
	}
	if (!check_is_valid_item(id)) {
		return -1;
	}
	
	uint32_t cur_cnt = get_item_cnt(id);
	uint32_t max_cnt = get_item_max_cnt(id);
	if (cur_cnt + add_cnt > max_cnt) {
		add_cnt = cur_cnt < max_cnt ? max_cnt - cur_cnt : 0;
	}

	ItemsMap::iterator it = items.find(id);
	if (it == items.end()) {
		item_info_t info;
		info.id = id;
		info.cnt = add_cnt;
		items.insert(ItemsMap::value_type(id, info));
	} else {
		it->second.cnt += add_cnt;
	}

	cur_cnt += add_cnt;

	return add_cnt;
}

int
ItemsManager::del_items(uint32_t id, uint32_t del_cnt)
{
	if (!del_cnt) {
		return 0;
	}
	if (!check_is_valid_item(id)) {
		return -1;
	}
	
	uint32_t cur_cnt = get_item_cnt(id);
	if (cur_cnt < del_cnt) {
		del_cnt = cur_cnt;
	}

	ItemsMap::iterator it = items.find(id);
	if (it == items.end()) {
		return -1;
	} else {
		it->second.cnt -= del_cnt;
	}

	return del_cnt;
}

int
ItemsManager::add_item_without_callback(uint32_t id, uint32_t add_cnt)
{
	if (!add_cnt) {
		return 0;
	}
	if (id == 1) {//金币
		owner->chg_golds(add_cnt);
		return 0;
	}
	const item_xml_info_t *p_info = items_xml_mgr->get_item_xml_info(id);
	if (p_info) {
		if (p_info->type == em_item_type_for_hero_card) {//英雄卡牌
			Hero *p_hero = owner->hero_mgr->get_hero(p_info->relation_id);
			if (!p_hero) {//如果没有该英雄，则卡牌直接转化成英雄
				owner->hero_mgr->add_hero(p_info->relation_id);
				return 0;
			}
		} else if (p_info->type == em_item_type_for_soldier_card) {//小兵卡牌
			uint32_t soldier_id = p_info->relation_id;
			Soldier *p_soldier = owner->soldier_mgr->get_soldier(soldier_id); 
			if (!p_soldier){
				owner->soldier_mgr->add_soldier(soldier_id);
				return 0;
			} else {//有小兵转换成20碎片
				id = 54 * 10000 + soldier_id;
				add_cnt = 20 * add_cnt; 
			}
		}
	}

	int32_t real_add_cnt = add_items(id, add_cnt);

	if (real_add_cnt == -1) {
		T_KWARN_LOG(owner->user_id, "add invalid item id\t[item_id=%u]", id);
		return owner->send_to_self_error(owner->wait_cmd, cli_invalid_item_err, 1);
	}

	//db更新
	db_change_items_in db_in;
	db_in.flag = 1;
	db_in.item_id = id;
	db_in.chg_cnt = real_add_cnt;

	send_msg_to_dbroute(0, db_change_items_cmd, &db_in, owner->user_id);

	//如果是装备碎片，加入redis
	const item_piece_xml_info_t *p_piece_xml_info = item_piece_xml_mgr->get_item_piece_xml_info(id);
	if (p_piece_xml_info && p_piece_xml_info->type == 2) {//装备碎片
		redis_mgr->set_treasure_piece_user(owner, id);
	}

	//通知前端
	cli_item_change_noti_out noti_out;
	noti_out.item_id = id;
	noti_out.item_cnt = this->get_item_cnt(id);

	T_KDEBUG_LOG(owner->user_id, "ADD ITEMS\t[item_id=%u, add_cnt=%u, cur_cnt=%u]", id, add_cnt, noti_out.item_cnt);

	owner->send_to_self(cli_item_change_noti_cmd, &noti_out, 0);

	return real_add_cnt;
}

int
ItemsManager::del_item_without_callback(uint32_t id, uint32_t del_cnt)
{
	if (!del_cnt) {
		return 0;
	}
	int32_t real_del_cnt = del_items(id, del_cnt);
	if (real_del_cnt == -1) {
		T_KWARN_LOG(owner->user_id, "del invalid item id\t[item_id=%u]", id);
		return owner->send_to_self_error(owner->wait_cmd, cli_invalid_item_err, 1);
	}

	//db更新
	db_change_items_in db_in;
	db_in.flag = 0;
	db_in.item_id = id;
	db_in.chg_cnt = real_del_cnt;

	send_msg_to_dbroute(0, db_change_items_cmd, &db_in, owner->user_id);

	uint32_t item_cnt = this->get_item_cnt(id);
	if (item_cnt == 0) {
		//如果是装备碎片，更新redis
		const item_piece_xml_info_t *p_piece_xml_info = item_piece_xml_mgr->get_item_piece_xml_info(id);
		if (p_piece_xml_info && p_piece_xml_info->type == 2) {//装备碎片
			redis_mgr->del_treasure_piece_user(owner, id);
		}
	}

	//通知前端
	cli_item_change_noti_out noti_out;
	noti_out.item_id = id;
	noti_out.item_cnt = item_cnt;

	T_KDEBUG_LOG(owner->user_id, "DEL ITEMS\t[item_id=%u, del_cnt=%u, cur_cnt=%u]", id, del_cnt, item_cnt);

	return owner->send_to_self(cli_item_change_noti_cmd, &noti_out, 0);
}

int
ItemsManager::compound_item_piece(uint32_t piece_id, uint32_t &item_id)
{
	const item_piece_xml_info_t *p_piece_xml_info = item_piece_xml_mgr->get_item_piece_xml_info(piece_id);
	if (!p_piece_xml_info) {
		T_KWARN_LOG(owner->user_id, "compound item piece id err\t[piece_id=%u]", piece_id);
		return cli_invalid_item_err;
	}

	uint32_t piece_cnt = this->get_item_cnt(piece_id);
	if (piece_cnt < p_piece_xml_info->need_num) {
		T_KWARN_LOG(owner->user_id, "compound item piece need num not enough\t[num=%u, need_num=%u]", piece_cnt, p_piece_xml_info->need_num);
		return cli_not_enough_item_err;
	}

	//扣除碎片
	this->del_item_without_callback(piece_id, p_piece_xml_info->need_num);

	//添加新物品
	item_id = p_piece_xml_info->relation_id;
	this->add_reward(item_id, 1);

	return 0;
}

/* @brief 开启宝箱
 */
int
ItemsManager::open_treasure_box(uint32_t box_id, uint32_t cnt)
{
	uint32_t key_id = 0;
	if (box_id == 161000) {//铜宝箱
		key_id = 160000;
	} else if (box_id == 161001) {//银宝箱
		key_id = 160001;
	} else if (box_id == 161002) {//金宝箱
		key_id = 160002;
	} else {
		return cli_invalid_treasure_box_id_err;
	}

	uint32_t box_cnt = this->get_item_cnt(box_id);
	uint32_t key_cnt = this->get_item_cnt(key_id);

	if (box_cnt < cnt || key_cnt < cnt) {
		T_KWARN_LOG(owner->user_id, "open treasure box need cnt not enough\t[box_cnt=%u, key_cnt=%u, cnt=%u]", box_cnt, key_cnt, cnt);
		return cli_not_enough_item_err;
	}

	//扣除宝箱 钥匙
	this->del_item_without_callback(box_id, cnt);
	this->del_item_without_callback(key_id, cnt);

	//添加物品
	cli_send_get_common_bonus_noti_out noti_out;
	for (uint32_t i = 0; i < cnt; i++) {
		const random_item_item_xml_info_t *p_xml_info = random_item_xml_mgr->random_one_item(box_id);
		if (p_xml_info) {
			this->add_reward(p_xml_info->item_id, p_xml_info->item_cnt);
			this->pack_give_items_info(noti_out.give_items_info, p_xml_info->item_id, p_xml_info->item_cnt);
		}
	}

	//发送通知
	if (noti_out.give_items_info.size() > 0) {
		owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);
	}

	return 0;
}

/* @brief 开启随机礼包
 */
int
ItemsManager::open_random_gift(uint32_t item_id, uint32_t cnt)
{
	const item_xml_info_t *p_xml_info = items_xml_mgr->get_item_xml_info(item_id);
	if (!p_xml_info || p_xml_info->type != em_item_type_for_random_gift) {
		return cli_invalid_item_err;
	}

	uint32_t item_cnt = this->get_item_cnt(item_id);
	if (item_cnt < cnt) {
		return cli_not_enough_item_err;
	}

	//扣除礼包
	this->del_item_without_callback(item_id, cnt);

	//添加物品
	for (uint32_t i = 0; i < cnt; i++) {
		const random_item_item_xml_info_t *p_xml_info = random_item_xml_mgr->random_one_item(item_id);
		if (p_xml_info) {
			this->add_reward(p_xml_info->item_id, p_xml_info->item_cnt);
		}
	}

	return 0;
}

/* @brief 售卖物品
 */
int
ItemsManager::sell_item(uint32_t item_id, uint32_t cnt)
{
	const item_xml_info_t *p_xml_info = items_xml_mgr->get_item_xml_info(item_id);
	if (!p_xml_info) {
		return cli_invalid_item_err;
	}

	uint32_t item_cnt = this->get_item_cnt(item_id);
	if (item_cnt < cnt) {
		return cli_not_enough_item_err;
	}

	//扣除物品
	this->del_item_without_callback(item_id, cnt);

	//增加金币
	uint32_t add_golds = p_xml_info->price * cnt;
	owner->chg_golds(add_golds);

	return 0;
}

int
ItemsManager::pack_all_items_info(cli_get_items_info_out &out)
{
	ItemsMap::iterator it = items.begin();
	for (; it != items.end(); it++) {
		cli_item_info_t info;
		info.item_id = it->second.id;
		info.item_cnt = it->second.cnt;
		out.items.push_back(info);
	}

	return 0;
}

void 
ItemsManager::pack_give_items_info(std::vector<cli_item_info_t> &item_vec, uint32_t item_id, uint32_t item_cnt)
{
	cli_item_info_t info;
	info.item_id = item_id;
	info.item_cnt = item_cnt;

	item_vec.push_back(info);
}

/********************************************************************************/
/*							ItemXmlManager class								*/
/********************************************************************************/
ItemsXmlManager::ItemsXmlManager()
{

}

ItemsXmlManager::~ItemsXmlManager()
{

}

const item_xml_info_t*
ItemsXmlManager::get_item_xml_info(uint32_t item_id)
{
	ItemXmlMap::const_iterator it = items.find(item_id);
	if (it == items.end()) {
		return 0;
	}

	return &(it->second);
}

int
ItemsXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_items_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);
	return ret; 
	
}

int
ItemsXmlManager::load_items_xml_info(xmlNodePtr cur) 
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, reinterpret_cast<const xmlChar*>("item"))) {
			uint32_t id = 0;
			get_xml_prop(id, cur, "id");
			ItemXmlMap::iterator it = items.find(id);
			if (it != items.end()) {
				ERROR_LOG("load item xml info err, id exists, id=%u", id);
				return -1;
			}

			item_xml_info_t info;
			info.id = id;

			get_xml_prop_def(info.type, cur, "type", 0);
			get_xml_prop_def(info.rank, cur, "rank", 0);
			get_xml_prop_def(info.relation_id, cur, "relation_id", 0);
			get_xml_prop_def(info.max, cur, "max", 9999);
			get_xml_prop_def(info.effect, cur, "effect", 0);
			get_xml_prop_def(info.price, cur, "price", 0);

			items.insert(ItemXmlMap::value_type(id, info));

			TRACE_LOG("load item xml info\t[%u %u %u %u %u %u]", info.id, info.type, info.rank, info.max, info.effect, info.price);
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							HeroRankItemXmlManager Class						*/
/********************************************************************************/
HeroRankItemXmlManager::HeroRankItemXmlManager()
{

}

HeroRankItemXmlManager::~HeroRankItemXmlManager()
{

}

const hero_rank_item_xml_info_t*
HeroRankItemXmlManager::get_hero_rank_item_xml_info(uint32_t item_id)
{
	HeroRankItemXmlMap::iterator it = hero_rank_item_xml_map.find(item_id);
	if (it == hero_rank_item_xml_map.end()) {
		return 0;
	}

	return &(it->second);
}

int
HeroRankItemXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_hero_rank_item_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
HeroRankItemXmlManager::load_hero_rank_item_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("item"))) {
			uint32_t item_id = 0;
			get_xml_prop(item_id, cur, "item_id");
			HeroRankItemXmlMap::iterator it = hero_rank_item_xml_map.find(item_id);
			if (it != hero_rank_item_xml_map.end()) {
				ERROR_LOG("load hero rank item xml info err, item id exists, item_id=%u", item_id);
				return -1;
			}

			hero_rank_item_xml_info_t info;
			info.item_id = item_id;
			get_xml_prop(info.item_rank, cur, "item_rank");
			get_xml_prop(info.price, cur, "price");
			get_xml_prop_def(info.raw[0][0], cur, "raw_1", 0);
			get_xml_prop_def(info.raw[0][1], cur, "raw_1_num", 0);
			get_xml_prop_def(info.raw[1][0], cur, "raw_2", 0);
			get_xml_prop_def(info.raw[1][1], cur, "raw_2_num", 0);
			get_xml_prop_def(info.raw[2][0], cur, "raw_3", 0);
			get_xml_prop_def(info.raw[2][1], cur, "raw_3_num", 0);
			get_xml_prop_def(info.raw[3][0], cur, "raw_4", 0);
			get_xml_prop_def(info.raw[3][1], cur, "raw_4_num", 0);

			TRACE_LOG("load hero rank item xml info\t[%u %u %u %u %u %u %u %u %u %u %u]", 
					info.item_id, info.item_rank, info.price, info.raw[0][0], info.raw[0][1], info.raw[1][0], info.raw[1][1], 
					info.raw[2][0], info.raw[2][1], info.raw[3][0], info.raw[3][1]);

			hero_rank_item_xml_map.insert(HeroRankItemXmlMap::value_type(item_id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							ItempieceXmlManager Class						*/
/********************************************************************************/
ItemPieceXmlManager::ItemPieceXmlManager()
{

}

ItemPieceXmlManager::~ItemPieceXmlManager()
{

}

const item_piece_xml_info_t*
ItemPieceXmlManager::get_item_piece_xml_info(uint32_t item_id)
{
	ItemPieceXmlMap::iterator it = item_piece_xml_map.find(item_id);
	if (it == item_piece_xml_map.end()) {
		return 0;
	}

	return &(it->second);
}

int
ItemPieceXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);

	int ret = load_item_piece_xml_info(cur);

	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
ItemPieceXmlManager::load_item_piece_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("piece"))) {
			uint32_t item_id = 0;
			get_xml_prop(item_id, cur, "item_id");
			ItemPieceXmlMap::iterator it = item_piece_xml_map.find(item_id);
			if (it != item_piece_xml_map.end()) {
				ERROR_LOG("load hero rank piece xml info err, item id exists, item_id=%u", item_id);
				return -1;
			}

			item_piece_xml_info_t info;
			info.item_id = item_id;
			get_xml_prop(info.item_rank, cur, "item_rank");
			get_xml_prop_def(info.need_num, cur, "need_num", 0);
			get_xml_prop_def(info.comp_fee, cur, "comp_fee", 0);
			get_xml_prop(info.relation_id, cur, "relation_id");
			get_xml_prop(info.type, cur, "type");

			TRACE_LOG("load hero rank piece xml info\t[%u %u %u %u %u %u]", info.item_id, info.item_rank, info.need_num, info.comp_fee, info.relation_id, info.type);

			item_piece_xml_map.insert(ItemPieceXmlMap::value_type(item_id, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*							RandomItemXmlManager Class							*/
/********************************************************************************/
RandomItemXmlManager::RandomItemXmlManager()
{
	random_item_xml_map.clear();
}

RandomItemXmlManager::~RandomItemXmlManager()
{

}

int
RandomItemXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_random_item_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

const random_item_item_xml_info_t *
RandomItemXmlManager::random_one_item(uint32_t box_id)
{
	TreasureBoxXmlMap::iterator it = random_item_xml_map.find(box_id);
	if (it == random_item_xml_map.end()) {
		return 0;
	}

	if (!it->second.total_prob) {
		return 0;
	}

	uint32_t prob = 0;
	uint32_t r = rand() % (it->second.total_prob);
	for (uint32_t i = 0; i < it->second.items.size(); i++) {
		const random_item_item_xml_info_t *p_xml_info = &(it->second.items[i]);
		prob += p_xml_info->prob;
		if (r < prob) {
			return p_xml_info;
		}
	}

	return 0;
}

int
RandomItemXmlManager::load_random_item_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("box"))) {
			uint32_t box_id = 0;
			get_xml_prop(box_id, cur, "id");
			random_item_item_xml_info_t sub_info = {};
			get_xml_prop(sub_info.item_id, cur, "item_id");
			get_xml_prop(sub_info.item_cnt, cur, "num");
			get_xml_prop(sub_info.prob, cur, "prob");

			TreasureBoxXmlMap::iterator it = random_item_xml_map.find(box_id);
			if (it != random_item_xml_map.end()) {
				it->second.total_prob += sub_info.prob;
				it->second.items.push_back(sub_info);
			} else {
				random_item_xml_info_t info = {};
				info.box_id = box_id;
				info.total_prob = sub_info.prob;
				info.items.push_back(sub_info);

				random_item_xml_map.insert(TreasureBoxXmlMap::value_type(box_id, info));
			}

			TRACE_LOG("load random item xml info\t[%u %u %u %u]", box_id, sub_info.item_id, sub_info.item_cnt, sub_info.prob);
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								client request									*/
/********************************************************************************/
/* @brief 拉取物品信息
 */
int cli_get_items_info(Player *p, Cmessage *c_in)
{
	return send_msg_to_dbroute(p, db_get_player_items_info_cmd, 0, p->user_id);
}

/* @brief 合成物品碎片
 */
int cli_compound_item_piece(Player *p, Cmessage *c_in)
{
	cli_compound_item_piece_in *p_in = P_IN;

	uint32_t item_id = 0;
	int ret = p->items_mgr->compound_item_piece(p_in->piece_id, item_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	cli_compound_item_piece_out cli_out;
	cli_out.piece_id = p_in->piece_id;
	cli_out.item_id = item_id;

	T_KDEBUG_LOG(p->user_id, "COMPOUND ITEM PIECE\t[piece_id=%u, item_id=%u]", p_in->piece_id, item_id);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/* @brief 出售道具
 */
int cli_sell_item(Player *p, Cmessage *c_in)
{
	cli_sell_item_in *p_in = P_IN;

	int ret = p->items_mgr->sell_item(p_in->item_id, p_in->item_cnt);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "SELL ITEM\t[item_id=%u, item_cnt=%u]", p_in->item_id, p_in->item_cnt);

	cli_sell_item_out cli_out;
	cli_out.item_id = p_in->item_id;
	cli_out.item_cnt = p_in->item_cnt;

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}

/********************************************************************************/
/*								dbsvr return									*/
/********************************************************************************/
/* @brief 拉取物品返回
 */
int db_get_player_items_info(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);
	db_get_player_items_info_out *p_in = P_IN;
	p->items_mgr->init_items_info(p_in);
	p->equip_mgr->init_equips_info(p_in);
	p->horse_equip_mgr->init_horse_equips_info(p_in);

	if (p->wait_cmd == cli_proto_login_cmd) {
		p->login_step++;

		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u ITEMS INFO", p->login_step);

		//物品和装备信息
		cli_get_items_info_out cli_out;
		p->items_mgr->pack_all_items_info(cli_out);
		p->equip_mgr->pack_all_equips_info(cli_out.equips);
		p->horse_equip_mgr->pack_all_horse_equips_info((cli_out.equips));
		p->send_to_self(cli_get_items_info_cmd, &cli_out, 0);

		//战马信息
		cli_get_horse_info_out cli_out2;
		p->horse->pack_horse_info(cli_out2);
		p->send_to_self(cli_get_horse_info_cmd, &cli_out2, 0);

		return send_msg_to_dbroute(p, db_get_player_btl_soul_list_cmd, 0, p->user_id);
	} else if (p->wait_cmd == cli_get_items_info_cmd) {
		cli_get_items_info_out cli_out;
		p->items_mgr->pack_all_items_info(cli_out);
		
		return p->send_to_self(p->wait_cmd, &cli_out, 1);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}
