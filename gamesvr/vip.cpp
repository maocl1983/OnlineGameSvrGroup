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
/*
#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"
#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"

#include "global_data.hpp"
#include "vip.hpp"
#include "dbroute.hpp"
#include "player.hpp"
*/

#include "stdafx.hpp"
using namespace std;
using namespace project;

//VipXmlManager vip_xml_mgr;


/********************************************************************************/
/*									VipManager									*/
/********************************************************************************/
VipManager::VipManager(Player *p) : owner(p)
{

}

VipManager::~VipManager()
{

}

int
VipManager::buy_daily_gift(uint32_t gift_id)
{
	if (gift_id > 7) {
		return cli_invalid_input_arg_err;
	}
	if (gift_id > owner->vip_lv) {
		T_KWARN_LOG(owner->user_id, "vip lv not enough\t[gift_id=%u, vip_lv=%u]", gift_id, owner->vip_lv);
		return cli_vip_lv_not_enough_err;
	}

	uint32_t gift_stat = owner->res_mgr->get_res_value(daily_vip_gift_stat);
	if (test_bit_on(gift_stat, gift_id + 1)) {
		T_KWARN_LOG(owner->user_id, "vip daily gift already gotted\t[gift_id=%u]", gift_id);
		return cli_vip_daily_gift_already_bought_err;
	}

	uint32_t price[] = {15, 30, 50, 70, 100, 120, 150, 200};
	uint32_t cost_diamond = price[gift_id];
	if (owner->diamond < cost_diamond) {
		return cli_not_enough_diamond_err;
	}

	//扣除元宝
	owner->chg_diamond(-cost_diamond);

	//设置限制
	gift_stat = set_bit_on(gift_stat, gift_id + 1);
	owner->res_mgr->set_res_value(daily_vip_gift_stat, gift_stat);

	//发送奖励
	cli_send_get_common_bonus_noti_out noti_out;
	if (gift_id == 0) {//3级宝石礼包
		owner->items_mgr->add_item_without_callback(163003, 1);
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 163003, 1);
	} else if (gift_id == 1) {//4级宝石礼包
	  	owner->items_mgr->add_item_without_callback(163004, 1);	
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 163004, 1);
	} else if (gift_id == 2) {//陨铁石3颗
	  	owner->items_mgr->add_item_without_callback(120002, 3);	
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 120002, 3);
	} else if (gift_id == 3) {//精炼石3颗
	  	owner->items_mgr->add_item_without_callback(120000, 3);	
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 120000, 3);
	} else if (gift_id == 4) {//耐力丹4个
	  	owner->items_mgr->add_item_without_callback(120008, 4);	
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 120008, 4);
	} else if (gift_id == 5) {//战功道具10个
	  	owner->items_mgr->add_item_without_callback(140000, 10);	
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 140000, 10);
	} else if (gift_id == 6) {//紫色装备碎片礼包*6,铜宝箱*10,铜钥匙*10
	  	owner->items_mgr->add_item_without_callback(162000, 6);	
	  	owner->items_mgr->add_item_without_callback(161000, 10);	
	  	owner->items_mgr->add_item_without_callback(160000, 10);	
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 162000, 6);
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 161000, 10);
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 160000, 10);
	} else if (gift_id == 7) {//紫色装备碎片礼包*10,铜宝箱*20,铜钥匙*20
	  	owner->items_mgr->add_item_without_callback(162000, 10);	
	  	owner->items_mgr->add_item_without_callback(161000, 20);	
	  	owner->items_mgr->add_item_without_callback(160000, 20);	
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 162000, 10);
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 161000, 20);
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, 160000, 20);
	}

	owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	return 0;
}


/********************************************************************************/
/*								VipXmlManager									*/
/********************************************************************************/
VipXmlManager::VipXmlManager()
{
	vip_xml_map.clear();
}

VipXmlManager::~VipXmlManager()
{

}

const vip_xml_info_t *
VipXmlManager::get_vip_xml_info(uint32_t type)
{
	VipXmlMap::iterator it = vip_xml_map.find(type);
	if (it != vip_xml_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
VipXmlManager::get_vip_max_limit(uint32_t type, uint32_t vip_lv)
{
	if (vip_lv > 14) {
		return 0;
	}
	const vip_xml_info_t *p_xml_info = get_vip_xml_info(type);
	if (!p_xml_info) {
		return 0;
	}

	return p_xml_info->vip[vip_lv];
}

int
VipXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_vip_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);

	xmlFreeDoc(doc);

	return ret;
}

int
VipXmlManager::load_vip_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("vip"))) {
			uint32_t type = 0;
			get_xml_prop(type, cur, "type");
			VipXmlMap::iterator it = vip_xml_map.find(type);
			if (it != vip_xml_map.end()) {
				ERROR_LOG("load vip xml info err, type=%u", type);
				return -1;
			}
			vip_xml_info_t info = {};
			info.type = type;
			get_xml_prop_def(info.vip[0], cur, "v0", 0);
			get_xml_prop_def(info.vip[1], cur, "v1", 0);
			get_xml_prop_def(info.vip[2], cur, "v2", 0);
			get_xml_prop_def(info.vip[3], cur, "v3", 0);
			get_xml_prop_def(info.vip[4], cur, "v4", 0);
			get_xml_prop_def(info.vip[5], cur, "v5", 0);
			get_xml_prop_def(info.vip[6], cur, "v6", 0);
			get_xml_prop_def(info.vip[7], cur, "v7", 0);
			get_xml_prop_def(info.vip[8], cur, "v8", 0);
			get_xml_prop_def(info.vip[9], cur, "v9", 0);
			get_xml_prop_def(info.vip[10], cur, "v10", 0);
			get_xml_prop_def(info.vip[11], cur, "v11", 0);
			get_xml_prop_def(info.vip[12], cur, "v12", 0);
			get_xml_prop_def(info.vip[13], cur, "v13", 0);
			get_xml_prop_def(info.vip[14], cur, "v14", 0);
	
			TRACE_LOG("load vip xml info\t[%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u]", info.type, 
					info.vip[0], info.vip[1], info.vip[2], info.vip[3], info.vip[4], info.vip[5], info.vip[6], info.vip[7], 
					info.vip[8], info.vip[9], info.vip[10], info.vip[11], info.vip[12], info.vip[13], info.vip[14]);

			vip_xml_map.insert(VipXmlMap::value_type(type, info));
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 领取vip每日礼包
 */
int cli_buy_vip_daily_gift(Player *p, Cmessage *c_in)
{
	cli_buy_vip_daily_gift_in *p_in = P_IN;

	int ret = p->vip_mgr->buy_daily_gift(p_in->gift_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "BUY VIP DAILY GIFT\t[gift_id=%u]", p_in->gift_id);

	cli_buy_vip_daily_gift_out cli_out;
	cli_out.gift_id = p_in->gift_id;
	cli_out.gift_stat = p->res_mgr->get_res_value(daily_vip_gift_stat);

	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}
