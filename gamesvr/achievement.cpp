/*
 * =====================================================================================
 *
 *  @file  achievement.cpp 
 *
 *  @brief  成就系统
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

#include "achievement.hpp"
#include "player.hpp"
#include "dbroute.hpp"

using namespace std;
using namespace project;

AchievementXmlManager achievement_xml_mgr;

/********************************************************************************/
/*									AchievementManager									*/
/********************************************************************************/
AchievementManager::AchievementManager(Player *p) : owner(p)
{
	achievement_map.clear();
}

AchievementManager::~AchievementManager()
{

}

void
AchievementManager::init_achievement_list(db_get_achievement_list_out *p_in)
{
	for (uint32_t i = 0; i < p_in->achievement_list.size(); i++) {
		db_achievement_info_t *p_info = &(p_in->achievement_list[i]);
		const achievement_xml_info_t *base_info = achievement_xml_mgr.get_achievement_xml_info(p_info->achievement_id);
		if (!base_info) {
			continue;
		}
		achievement_info_t info;
		info.id = p_info->achievement_id;
		info.completed = p_info->completed;
		info.reach_tms = p_info->reach_tms;
		info.time = p_info->time;
		info.reward_stat = p_info->reward_stat;
		info.base_info = base_info;

		T_KTRACE_LOG(owner->user_id, "init achievement list\t[%u %u %u %u %u]", info.id, info.completed, info.reach_tms, info.time, info.reward_stat);

		achievement_map.insert(AchievementMap::value_type(info.id, info));
	}
}

const achievement_info_t*
AchievementManager::get_achievement_info(uint32_t achievement_id)
{
	AchievementMap::iterator it = achievement_map.find(achievement_id);
	if (it != achievement_map.end()) {
		return &(it->second);
	}

	return 0;
}

bool
AchievementManager::check_pre_achievement_is_completed(uint32_t achievement_id)
{
	const achievement_xml_info_t *p_xml_info = achievement_xml_mgr.get_achievement_xml_info(achievement_id);
	if (!p_xml_info) {
		return false;
	}
	if (p_xml_info->pre_achievement) {
		const achievement_info_t *p_info = get_achievement_info(p_xml_info->pre_achievement);
		if (!p_info || !p_info->completed) {
			return false;
		}
	}

	return true;
}

/* @brief 检查成就
 * @parm1和parm2都可缺省, parm1缺省表示按type累计次数的成就, parm2缺省表示按type和parm1累计次数的成就
 */
int
AchievementManager::check_achievement(uint32_t achievement_type, uint32_t parm1, uint32_t parm2)
{
	const achievement_type_xml_info_t *p_type_info = achievement_xml_mgr.get_achievement_type_xml_info(achievement_type);
	if (!p_type_info) {
		return 0;
	}

	for (uint32_t i = 0; i < p_type_info->achievement_list.size(); i++) {
		uint32_t achievement_id = p_type_info->achievement_list[i];
		const achievement_xml_info_t *p_xml_info = achievement_xml_mgr.get_achievement_xml_info(achievement_id);
		const achievement_info_t *p_info = get_achievement_info(achievement_id);
		if (!p_xml_info) {
			continue;
		}

		//是否解锁
		if (owner->lv < p_xml_info->unlock_lv) {
			continue;
		}
		
		//是否完成前置成就
		if (!check_pre_achievement_is_completed(achievement_id)) {
			continue;
		}

		//是否改变成就进度
		uint32_t progress_flag = 0;
		switch (achievement_type) {
		case em_achievement_type_story_instance: 
			if (parm1 == p_xml_info->parm1) {
				progress_flag = 1;
			}
			break;
		case em_achievement_type_skill_strength:
		case em_achievement_type_soldier_train:
		case em_achievement_type_equip_strength:
		case em_achievement_type_adventure:
		case em_achievement_type_arena:
			progress_flag = 1;
			break;
		case em_achievement_type_btl_power:
			if (parm1 >= p_xml_info->parm1) {
				progress_flag = 1;
			}
		case em_achievement_type_soldier_rank:
		case em_achievement_type_soldier_star:
		case em_achievement_type_hero_lv:
		case em_achievement_type_hero_rank:
		case em_achievement_type_hero_honor_lv:
			//第一次达成或达成次数超过上次
			if (parm1 >= p_xml_info->parm1 && (!p_info || parm2 > p_info->reach_tms)) {
				progress_flag = 1;
			}
			break;
		case em_achievement_type_btl_soul_cat:
		case en_achievement_type_btl_soul_lv:
		case em_achievement_type_btl_soul_rank:
			break;
		default:
			break;
		}
		if (progress_flag) {
			//如果已完成，则此处会返回
			if (parm2) {
				progress_achievement(achievement_id, parm2);
			} else {
				progress_achievement(achievement_id);
			}
		}
	}


	return 0;
}

int
AchievementManager::progress_achievement(uint32_t achievement_id, uint32_t reach_tms)
{
	const achievement_xml_info_t *base_info = achievement_xml_mgr.get_achievement_xml_info(achievement_id);
	if (!base_info) {
		return -1;
	}

	uint32_t now_sec = get_now_tv()->tv_sec;
	AchievementMap::iterator it = achievement_map.find(achievement_id);
	if (it != achievement_map.end()) {
		achievement_info_t *p_info = &(it->second);
		if (p_info->completed) {
			return -1;
		}
		if (reach_tms) {
			p_info->reach_tms = reach_tms;
		} else {
			p_info->reach_tms++;
		}
		if (p_info->reach_tms >= p_info->base_info->parm2) {
			p_info->completed = 1;
		}
		p_info->time = now_sec;
	} else {
		achievement_info_t info;
		info.id = achievement_id;
		info.reach_tms = reach_tms ? reach_tms : 1;
		info.time = now_sec;
		info.reward_stat = 0;
		info.base_info = base_info;
		if (info.reach_tms >= base_info->parm2) {
			info.completed = 1;
		}
		achievement_map.insert(AchievementMap::value_type(achievement_id, info));
	}
	
	const achievement_info_t *p_info = get_achievement_info(achievement_id);
	if (!p_info) {
		return -1;
	}

	//更新DB
	db_set_achievement_info_in db_in;
	db_in.achievement_info.achievement_id = achievement_id;
	db_in.achievement_info.completed = p_info->completed;
	db_in.achievement_info.reach_tms = p_info->reach_tms;
	db_in.achievement_info.time = p_info->time;
	send_msg_to_dbroute(0, db_set_achievement_info_cmd, &db_in, owner->user_id);

	//通知前端
	cli_update_achievement_info_noti_out noti_out;
	noti_out.achievement_info.achievement_id = achievement_id;
	noti_out.achievement_info.completed = p_info->completed;
	noti_out.achievement_info.reach_tms = p_info->reach_tms;
	noti_out.achievement_info.time = p_info->time;
	owner->send_to_self(cli_update_achievement_info_noti_cmd, &noti_out, 0);

	T_KDEBUG_LOG(owner->user_id, "update achievement progress\t[achievement_id=%u]", achievement_id);

	return 0;
}

int
AchievementManager::get_achievement_completed_reward(uint32_t achievement_id)
{
	AchievementMap::iterator it = achievement_map.find(achievement_id);
	if (it == achievement_map.end()) {
		T_KWARN_LOG(owner->user_id, "achievement not completed 1\t[achievement_id=%u]", achievement_id);
		return cli_achievement_not_completed_err;
	}

	achievement_info_t *p_info = &(it->second);
	if (!p_info->completed) {
		T_KWARN_LOG(owner->user_id, "achievement not completed 2\t[achievement_id=%u]", achievement_id);
		return cli_achievement_not_completed_err;
	}

	if (p_info->reward_stat) {
		T_KWARN_LOG(owner->user_id, "achievement reward already gotted\t[achievement_id=%u]", achievement_id);
		return cli_achievement_reward_already_gotted_err;
	}

	cli_send_get_common_bonus_noti_out noti_out;

	//发送奖励
	if (p_info->base_info->item_id1 && p_info->base_info->item_cnt1) {
		owner->items_mgr->add_reward(p_info->base_info->item_id1, p_info->base_info->item_cnt1);
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, p_info->base_info->item_id1, p_info->base_info->item_cnt1);
	}
	if (p_info->base_info->item_id2 && p_info->base_info->item_cnt2) {
		owner->items_mgr->add_reward(p_info->base_info->item_id2, p_info->base_info->item_cnt2);
		owner->items_mgr->pack_give_items_info(noti_out.give_items_info, p_info->base_info->item_id2, p_info->base_info->item_cnt2);
	}

	if (p_info->base_info->golds) {
		owner->chg_golds(p_info->base_info->golds);
		noti_out.golds = p_info->base_info->golds;
	}
	if (p_info->base_info->diamond) {
		owner->chg_diamond(p_info->base_info->diamond);
		noti_out.diamond = p_info->base_info->diamond;
	}

	//更新缓存
	p_info->reward_stat = 1;

	//更新DB
	db_set_achievement_reward_stat_in db_in;
	db_in.achievement_id = achievement_id;
	db_in.reward_stat = 1;
	send_msg_to_dbroute(0, db_set_achievement_reward_stat_cmd, &db_in, owner->user_id);

	//通知前端
	cli_update_achievement_info_noti_out achievement_noti_out;
	achievement_noti_out.achievement_info.achievement_id = achievement_id;
	achievement_noti_out.achievement_info.completed = p_info->completed;
	achievement_noti_out.achievement_info.reach_tms = p_info->reach_tms;
	achievement_noti_out.achievement_info.time = p_info->time;
	achievement_noti_out.achievement_info.reward_stat = p_info->reward_stat;
	owner->send_to_self(cli_update_achievement_info_noti_cmd, &achievement_noti_out, 0);

	//奖励通知
	owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	T_KDEBUG_LOG(owner->user_id, "GET ACHIEVEMENT COMPLETED REWARD\t[%u]", achievement_id);

	return 0;
}

void 
AchievementManager::pack_client_achievement_list(cli_get_achievement_list_out &out)
{
	AchievementMap::iterator it = achievement_map.begin();
	for (; it != achievement_map.end(); ++it) {
		achievement_info_t *p_info = &(it->second);
		cli_achievement_info_t info;
		info.achievement_id = p_info->id;
		info.completed = p_info->completed;
		info.reach_tms = p_info->reach_tms;
		info.time = p_info->time;
		info.reward_stat = p_info->reward_stat;

		T_KTRACE_LOG(owner->user_id, "pack client achievement list\t[%u %u %u %u %u]", info.achievement_id, info.completed, info.reach_tms, info.time, info.reward_stat);

		out.achievement_list.push_back(info);
	}
}


/********************************************************************************/
/*								AchievementXmlManager									*/
/********************************************************************************/

AchievementXmlManager::AchievementXmlManager()
{

}

AchievementXmlManager::~AchievementXmlManager()
{

}

const achievement_xml_info_t*
AchievementXmlManager::get_achievement_xml_info(uint32_t achievement_id)
{
	AchievementXmlMap::iterator it = achievement_map.find(achievement_id);
	if (it != achievement_map.end()) {
		return &(it->second);
	}

	return 0;
}

const achievement_type_xml_info_t*
AchievementXmlManager::get_achievement_type_xml_info(uint32_t achievement_type)
{
	AchievementTypeXmlMap::iterator it = achievement_type_map.find(achievement_type);
	if (it != achievement_type_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
AchievementXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_achievement_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
AchievementXmlManager::load_achievement_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("achievement"))) {
			uint32_t achievement_id = 0;
			get_xml_prop(achievement_id, cur, "achievement_id");
			AchievementXmlMap::iterator it = achievement_map.find(achievement_id);
			if (it != achievement_map.end()) {
				ERROR_LOG("load achievement xml info err, achievement_id=%u", achievement_id);
				return -1;
			}
			achievement_xml_info_t info;
			info.id = achievement_id;
			get_xml_prop(info.type, cur, "achievement_type");
			get_xml_prop(info.parm1, cur, "prm1");
			get_xml_prop_def(info.parm2, cur, "prm2", 0);
			get_xml_prop_def(info.unlock_lv, cur, "unlock_lv", 0);
			get_xml_prop_def(info.item_id1, cur, "item_id1", 0);
			get_xml_prop_def(info.item_cnt1, cur, "item_cnt1", 0);
			get_xml_prop_def(info.item_id2, cur, "item_id2", 0);
			get_xml_prop_def(info.item_cnt2, cur, "item_cnt2", 0);
			get_xml_prop_def(info.golds, cur, "golds", 0);
			get_xml_prop_def(info.diamond, cur, "diamond", 0);

			TRACE_LOG("load achievement xml info\t[%u %u %u %u %u %u %u %u %u %u %u]", 
					info.id, info.type, info.parm1, info.parm2, info.unlock_lv, info.item_id1, info.item_cnt1, 
					info.item_id2, info.item_cnt2, info.golds, info.diamond);

			achievement_map.insert(AchievementXmlMap::value_type(achievement_id, info));

			AchievementTypeXmlMap::iterator it2 = achievement_type_map.find(info.type);
			if (it2 != achievement_type_map.end()) {
				it2->second.achievement_list.push_back(info.id);
			} else {
				achievement_type_xml_info_t info2;
				info2.type = info.type;
				info2.achievement_list.push_back(info.id);
				achievement_type_map.insert(AchievementTypeXmlMap::value_type(info2.type, info2));
			}
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 拉取成就列表
 */
int cli_get_achievement_list(Player *p, Cmessage *c_in)
{
	return send_msg_to_dbroute(p, db_get_achievement_list_cmd, 0, p->user_id);
}

/* @brief 领取成就完成奖励
 */
int cli_get_achievement_completed_reward(Player *p, Cmessage *c_in)
{
	cli_get_achievement_completed_reward_in *p_in = P_IN;

	int ret = p->achievement_mgr->get_achievement_completed_reward(p_in->achievement_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "GET ACHIEVEMENT COMPLETED REWARD\t[achievement_id=%u]", p_in->achievement_id);

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/********************************************************************************/
/*								DB Return										*/
/********************************************************************************/
/* @brief 拉取成就列表
 */
int db_get_achievement_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);
	db_get_achievement_list_out *p_in = P_IN;

	p->achievement_mgr->init_achievement_list(p_in);

	if (p->wait_cmd == cli_proto_login_cmd) {
		p->login_step++;
		p->login_completed = 1;

		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u GET ACHIEVEMENT LIST", p->login_step);

		cli_get_achievement_list_out cli_out;
		p->achievement_mgr->pack_client_achievement_list(cli_out);
		return p->send_to_self(cli_get_achievement_list_cmd, &cli_out, 1);
	} else if (p->wait_cmd == cli_get_achievement_list_cmd) {
		cli_get_achievement_list_out cli_out;
		p->achievement_mgr->pack_client_achievement_list(cli_out);

		return p->send_to_self(p->wait_cmd, &cli_out, 1);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}
