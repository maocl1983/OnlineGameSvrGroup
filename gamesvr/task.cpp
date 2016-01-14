/*
 * =====================================================================================
 *
 *  @file  task.cpp 
 *
 *  @brief  任务系统
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
#include "task.hpp"
#include "player.hpp"
#include "dbroute.hpp"

using namespace std;
using namespace project;

//TaskXmlManager task_xml_mgr;

/********************************************************************************/
/*									TaskManager									*/
/********************************************************************************/
TaskManager::TaskManager(Player *p) : owner(p)
{
	task_map.clear();
}

TaskManager::~TaskManager()
{

}

void
TaskManager::init_task_list(db_get_task_list_out *p_in)
{
	for (uint32_t i = 0; i < p_in->task_list.size(); i++) {
		db_task_info_t *p_info = &(p_in->task_list[i]);
		const task_xml_info_t *base_info = task_xml_mgr->get_task_xml_info(p_info->task_id);
		if (!base_info) {
			continue;
		}
		task_info_t info;
		info.id = p_info->task_id;
		info.completed = p_info->completed;
		info.reach_tms = p_info->reach_tms;
		info.time = p_info->time;
		info.reward_stat = p_info->reward_stat;
		info.base_info = base_info;

		T_KTRACE_LOG(owner->user_id, "init task list\t[%u %u %u %u %u]", info.id, info.completed, info.reach_tms, info.time, info.reward_stat);

		task_map.insert(TaskMap::value_type(info.id, info));
	}
}

const task_info_t*
TaskManager::get_task_info(uint32_t task_id)
{
	TaskMap::iterator it = task_map.find(task_id);
	if (it != task_map.end()) {
		return &(it->second);
	}

	return 0;
}

bool
TaskManager::check_pre_task_is_completed(uint32_t task_id)
{
	const task_xml_info_t *p_xml_info = task_xml_mgr->get_task_xml_info(task_id);
	if (!p_xml_info) {
		return false;
	}
	if (p_xml_info->pre_task) {
		const task_info_t *p_info = get_task_info(p_xml_info->pre_task);
		if (!p_info || !p_info->completed) {
			return false;
		}
	}

	return true;
}

/* @brief 检查任务
 * @parm1和parm2都可缺省, parm1缺省表示按type累计次数的任务, parm2缺省表示按type和parm1累计次数的任务
 */
int
TaskManager::check_task(uint32_t task_type, uint32_t parm1, uint32_t parm2)
{
	const task_type_xml_info_t *p_type_info = task_xml_mgr->get_task_type_xml_info(task_type);
	if (!p_type_info) {
		return 0;
	}

	for (uint32_t i = 0; i < p_type_info->task_list.size(); i++) {
		uint32_t task_id = p_type_info->task_list[i];
		const task_xml_info_t *p_xml_info = task_xml_mgr->get_task_xml_info(task_id);
		const task_info_t *p_info = get_task_info(task_id);
		if (!p_xml_info) {
			continue;
		}

		//是否解锁
		if (owner->lv < p_xml_info->unlock_lv) {
			continue;
		}
		
		//是否完成前置任务
		if (!check_pre_task_is_completed(task_id)) {
			continue;
		}

		//是否改变任务进度
		uint32_t progress_flag = 0;
		switch (task_type) {
		case em_task_type_story_instance: 
		case em_task_type_common_fight:
			if (parm1 == p_xml_info->parm1) {
				progress_flag = 1;
			}
			break;
		case em_task_type_skill_strength:
		case em_task_type_soldier_train:
		case em_task_type_equip_strength:
		case em_task_type_adventure:
		case em_task_type_arena:
		case em_task_type_inlaid_gem:
		case em_task_type_internal_affairs:
		case em_task_type_grant_title:
			progress_flag = 1;
			break;
		case em_task_type_btl_power:
		case em_task_type_role_lv:
			if (parm1 >= p_xml_info->parm1) {
				progress_flag = 1;
			}
		case em_task_type_soldier_rank:
		case em_task_type_soldier_star:
		case em_task_type_hero_lv:
		case em_task_type_hero_rank:
		case em_task_type_hero_honor_lv:
			//第一次达成或达成次数超过上次
			if (parm1 >= p_xml_info->parm1 && (!p_info || parm2 > p_info->reach_tms)) {
				progress_flag = 1;
			}
			break;
		default:
			break;
		}
		if (progress_flag) {
			//如果已完成，则此处会返回
			if (parm2) {
				progress_task(task_id, parm2);
			} else {
				progress_task(task_id);
			}
		}
	}


	return 0;
}

int
TaskManager::progress_task(uint32_t task_id, uint32_t reach_tms)
{
	const task_xml_info_t *base_info = task_xml_mgr->get_task_xml_info(task_id);
	if (!base_info) {
		return -1;
	}

	uint32_t now_sec = get_now_tv()->tv_sec;
	TaskMap::iterator it = task_map.find(task_id);
	if (it != task_map.end()) {
		task_info_t *p_info = &(it->second);
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
		task_info_t info ={};
		info.id = task_id;
		info.reach_tms = reach_tms ? reach_tms : 1;
		info.time = now_sec;
		info.reward_stat = 0;
		info.base_info = base_info;
		if (info.reach_tms >= base_info->parm2) {
			info.completed = 1;
		}
		task_map.insert(TaskMap::value_type(task_id, info));
	}
	
	const task_info_t *p_info = get_task_info(task_id);
	if (!p_info) {
		return -1;
	}

	//更新DB
	db_set_task_info_in db_in;
	db_in.task_info.task_id = task_id;
	db_in.task_info.completed = p_info->completed;
	db_in.task_info.reach_tms = p_info->reach_tms;
	db_in.task_info.time = p_info->time;
	send_msg_to_dbroute(0, db_set_task_info_cmd, &db_in, owner->user_id);

	//通知前端
	cli_update_task_info_noti_out noti_out;
	noti_out.task_info.task_id = task_id;
	noti_out.task_info.completed = p_info->completed;
	noti_out.task_info.reach_tms = p_info->reach_tms;
	noti_out.task_info.time = p_info->time;
	owner->send_to_self(cli_update_task_info_noti_cmd, &noti_out, 0);

	T_KDEBUG_LOG(owner->user_id, "update task progress\t[task_id=%u]", task_id);

	return 0;
}

int
TaskManager::get_task_completed_reward(uint32_t task_id)
{
	TaskMap::iterator it = task_map.find(task_id);
	if (it == task_map.end()) {
		T_KWARN_LOG(owner->user_id, "task not completed 1\t[task_id=%u]", task_id);
		return cli_task_not_completed_err;
	}

	task_info_t *p_info = &(it->second);
	if (!p_info->completed) {
		T_KWARN_LOG(owner->user_id, "task not completed 2\t[task_id=%u]", task_id);
		return cli_task_not_completed_err;
	}

	if (p_info->reward_stat) {
		T_KWARN_LOG(owner->user_id, "task reward already gotted\t[task_id=%u]", task_id);
		return cli_task_reward_already_gotted_err;
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
	db_set_task_reward_stat_in db_in;
	db_in.task_id = task_id;
	db_in.reward_stat = 1;
	send_msg_to_dbroute(0, db_set_task_reward_stat_cmd, &db_in, owner->user_id);

	//通知前端
	cli_update_task_info_noti_out task_noti_out;
	task_noti_out.task_info.task_id = task_id;
	task_noti_out.task_info.completed = p_info->completed;
	task_noti_out.task_info.reach_tms = p_info->reach_tms;
	task_noti_out.task_info.time = p_info->time;
	task_noti_out.task_info.reward_stat = p_info->reward_stat;
	owner->send_to_self(cli_update_task_info_noti_cmd, &task_noti_out, 0);

	//奖励通知
	owner->send_to_self(cli_send_get_common_bonus_noti_cmd, &noti_out, 0);

	T_KDEBUG_LOG(owner->user_id, "GET TASK COMPLETED REWARD\t[%u]", task_id);

	return 0;
}

void 
TaskManager::pack_client_task_list(cli_get_task_list_out &out)
{
	TaskMap::iterator it = task_map.begin();
	for (; it != task_map.end(); ++it) {
		task_info_t *p_info = &(it->second);
		cli_task_info_t info;
		info.task_id = p_info->id;
		info.completed = p_info->completed;
		info.reach_tms = p_info->reach_tms;
		info.time = p_info->time;
		info.reward_stat = p_info->reward_stat;

		T_KTRACE_LOG(owner->user_id, "pack client task list\t[%u %u %u %u %u]", info.task_id, info.completed, info.reach_tms, info.time, info.reward_stat);

		out.task_list.push_back(info);
	}
}


/********************************************************************************/
/*								TaskXmlManager									*/
/********************************************************************************/

TaskXmlManager::TaskXmlManager()
{

}

TaskXmlManager::~TaskXmlManager()
{

}

const task_xml_info_t*
TaskXmlManager::get_task_xml_info(uint32_t task_id)
{
	TaskXmlMap::iterator it = task_map.find(task_id);
	if (it != task_map.end()) {
		return &(it->second);
	}

	return 0;
}

const task_type_xml_info_t*
TaskXmlManager::get_task_type_xml_info(uint32_t task_type)
{
	TaskTypeXmlMap::iterator it = task_type_map.find(task_type);
	if (it != task_type_map.end()) {
		return &(it->second);
	}

	return 0;
}

int
TaskXmlManager::read_from_xml(const char *filename)
{
	XML_PARSE_FILE(filename);
	int ret = load_task_xml_info(cur);
	BOOT_LOG(ret, "Load File %s", filename);
	xmlFreeDoc(doc);

	return ret;
}

int
TaskXmlManager::load_task_xml_info(xmlNodePtr cur)
{
	cur = cur->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, XMLCHAR_CAST("task"))) {
			uint32_t task_id = 0;
			get_xml_prop(task_id, cur, "task_id");
			TaskXmlMap::iterator it = task_map.find(task_id);
			if (it != task_map.end()) {
				ERROR_LOG("load task xml info err, task_id=%u", task_id);
				return -1;
			}
			task_xml_info_t info;
			info.id = task_id;
			get_xml_prop(info.type, cur, "task_type");
			get_xml_prop(info.parm1, cur, "prm1");
			get_xml_prop_def(info.parm2, cur, "prm2", 0);
			get_xml_prop_def(info.unlock_lv, cur, "unlock_lv", 0);
			get_xml_prop_def(info.item_id1, cur, "item_id1", 0);
			get_xml_prop_def(info.item_cnt1, cur, "item_cnt1", 0);
			get_xml_prop_def(info.item_id2, cur, "item_id2", 0);
			get_xml_prop_def(info.item_cnt2, cur, "item_cnt2", 0);
			get_xml_prop_def(info.golds, cur, "golds", 0);
			get_xml_prop_def(info.diamond, cur, "diamond", 0);

			TRACE_LOG("load task xml info\t[%u %u %u %u %u %u %u %u %u %u %u]", 
					info.id, info.type, info.parm1, info.parm2, info.unlock_lv, info.item_id1, info.item_cnt1, 
					info.item_id2, info.item_cnt2, info.golds, info.diamond);

			task_map.insert(TaskXmlMap::value_type(task_id, info));

			TaskTypeXmlMap::iterator it2 = task_type_map.find(info.type);
			if (it2 != task_type_map.end()) {
				it2->second.task_list.push_back(info.id);
			} else {
				task_type_xml_info_t info2;
				info2.type = info.type;
				info2.task_list.push_back(info.id);
				task_type_map.insert(TaskTypeXmlMap::value_type(info2.type, info2));
			}
		}
		cur = cur->next;
	}

	return 0;
}

/********************************************************************************/
/*								Client Request									*/
/********************************************************************************/
/* @brief 拉取任务列表
 */
int cli_get_task_list(Player *p, Cmessage *c_in)
{
	return send_msg_to_dbroute(p, db_get_task_list_cmd, 0, p->user_id);
}

/* @brief 领取任务完成奖励
 */
int cli_get_task_completed_reward(Player *p, Cmessage *c_in)
{
	cli_get_task_completed_reward_in *p_in = P_IN;

	int ret = p->task_mgr->get_task_completed_reward(p_in->task_id);
	if (ret) {
		return p->send_to_self_error(p->wait_cmd, ret, 1);
	}

	T_KDEBUG_LOG(p->user_id, "GET TASK COMPLETED REWARD\t[task_id=%u]", p_in->task_id);

	return p->send_to_self(p->wait_cmd, 0, 1);
}

/********************************************************************************/
/*								DB Return										*/
/********************************************************************************/
/* @brief 拉取任务列表
 */
int db_get_task_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_DB_ERR(p, ret);
	db_get_task_list_out *p_in = P_IN;

	p->task_mgr->init_task_list(p_in);

	if (p->wait_cmd == cli_proto_login_cmd) {
		p->login_step++;

		T_KDEBUG_LOG(p->user_id, "LOGIN STEP %u GET TASK LIST", p->login_step);

		cli_get_task_list_out cli_out;
		p->task_mgr->pack_client_task_list(cli_out);
		p->send_to_self(cli_get_task_list_cmd, &cli_out, 0);

		//拉取内政列表
		return send_msg_to_dbroute(p, db_get_affairs_list_cmd, 0, p->user_id);
	} else if (p->wait_cmd == cli_get_task_list_cmd) {
		cli_get_task_list_out cli_out;
		p->task_mgr->pack_client_task_list(cli_out);

		return p->send_to_self(p->wait_cmd, &cli_out, 1);
	}

	return p->send_to_self_error(p->wait_cmd, cli_invalid_cmd_err, 1);
}
