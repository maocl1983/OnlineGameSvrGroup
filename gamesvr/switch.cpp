/**
 * =====================================================================================
 *
 * @file  switch.cpp
 *
 * @brief 处理和switch服务器相关的命令
 *
 * compiler  GCC4.1.2
 * platform  Linux
 *
 * =====================================================================================
 */


extern "C"{
#include <string.h>
#include <libcommon/conf_parser/config.h>
#include <libcommon/log.h>
#include <libcommon/time/timer.h> 
}
#include <asyn_serv/net_if.hpp> 

#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"
#include "./proto/xseer_switch_enum.hpp"
#include "./proto/xseer_db.hpp"
#include "./proto/xseer_db_enum.hpp"

#include "switch.hpp"
#include "utils.hpp"
#include "common_def.hpp"



//-------------------------------------------------------
//函数定义
#undef  PROTO_FUNC_DEF
#define PROTO_FUNC_DEF(proto_name) \
	int proto_name( Player *p, Cmessage *p_in , uint32_t ret) ;
//#include "./proto/xseer_switch_func_def.hpp"
	PROTO_FUNC_DEF(sw_kick_player_off_line)
//-------------------------------------------------------------
//对应的结构体
#include "./proto/xseer_switch.hpp"
//-------------------------------------------------------------
//命令绑定
typedef int(*P_DEALFUN_T)(Player *p, Cmessage *c_in, uint32_t ret);

#undef  BIND_PROTO_CMD
#define BIND_PROTO_CMD(cmdid,proto_name,c_in,c_out,null)\
	{cmdid, new (c_out), proto_name},

Cproto< P_DEALFUN_T> g_switch_proto_list[]={
//#include "./proto/xseer_switch_bind_func.hpp"
	BIND_PROTO_CMD(60004,sw_kick_player_off_line,Cmessage,Cmessage,0)
};

//命令map
Cproto_map< Cproto< P_DEALFUN_T> >  g_switch_proto_map;

int switch_fd = -1;

/* @brief 处理switch返回的包，分发到相应的处理函数中
 * @param data 返回包
 * @param len switch返回包的长度 
*/
void handle_switch_return(sw_proto_head_t *data, uint32_t len)
{
	Player *p = NULL;
	sw_proto_head_t *pkg = data;

	if (data->seq) {
		uint32_t waitcmd = data->seq & 0xFFFF; //低16位命令
		int	  connfd  = data->seq >> 16; //高16位fd

		//p = g_player_mgr.get_player_by_fd(connfd);
		p = g_player_mgr.get_player_by_fd(connfd);
		if (!p || p->wait_cmd != waitcmd) {
			KERROR_LOG(pkg->uid, "sw re err:[p = %p][uid = %u][cmd = %u][seq = %u]",
				p, pkg->uid, pkg->cmd, pkg->seq);
			return;
		}
	} else {
		p = g_player_mgr.get_player_by_uid(pkg->uid);
		if (!p && (pkg->uid)) {
			//一些sw命令是不管用户是否在线的
			/*
			if (pkg->cmd != sw_player_recharge_noti_cmd
					&& pkg->cmd != sw_player_certificate_noti_cmd 
					&& pkg->cmd != sw_player_family_info_noti_cmd
					&& pkg->cmd != sw_battle_team_info_noti_cmd
					&& pkg->cmd != sw_get_btl_team_top_score_users_noti_cmd) {
				KERROR_LOG(pkg->uid, "sw re err:[p = %p][uid = %u][cmd = %u][seq = %u]",
					p, pkg->uid, pkg->cmd, pkg->seq);
				return;
			}
			*/
		}
	}
	
	KDEBUG_LOG(pkg->uid, "switch return\t[user_id = %u] [cmd = %u] [ret =%u]",
		pkg->uid, pkg->cmd, pkg->ret);

	//----------------------------------------------------
	Cproto< P_DEALFUN_T> * p_proto_item =g_switch_proto_map.getitem( pkg->cmd );
	if (p_proto_item == NULL) {
		DEBUG_LOG("sw re cmd id not find: %u", pkg->cmd);
		return;
	}

	int sw_ret = pkg->ret;
	Cmessage * msg;

	if (sw_ret == 0){//成功
		//还原对象
		p_proto_item->proto_msg->init();

		Cbuff_array in_ba ( (char *)data+sizeof(sw_proto_head_t), pkg->len - sizeof(sw_proto_head_t));
		//失败
		if (!p_proto_item->proto_msg->read_from_buf(in_ba)) {
			KERROR_LOG(pkg->uid, "sw 还原对象失败, cmd=%u", pkg->cmd);
			return;	
		}

		//客户端多上传报文
		if (!in_ba.is_end()) {
			KERROR_LOG(pkg->uid, "sw re 过多报文, cmd=%u", pkg->cmd);
			return;
		}
		msg = p_proto_item->proto_msg;
	} else {
		msg = NULL;
	}
	p_proto_item->func(p,msg,sw_ret);	
}


/* @brief 向switch发送包
 * @param cmd 发送DB的命令号
 * @param msg 发送DB的包体的内容，不包括包头
 * @param user_id 用户米米号
 */
int send_msg_to_switch(Player *p, uint16_t cmd, Cmessage *msg, uint32_t user_id)
{
	if (switch_fd == -1) {
		connect_to_switch();
	}

	if (switch_fd == -1) {
		ERROR_LOG("send to SWITCH failed: [FD = %d]", switch_fd );
		if (p) {
			//return send_to_self_error(p, p->wait_cmd, cli_critical_err, 1);
		}
		return 0;
	}

	sw_proto_head_t pkg;
	pkg.len = sizeof(sw_proto_head_t);
	pkg.seq = (p ? ((p->fdsess->fd << 16) | p->wait_cmd) : 0);
	pkg.cmd = cmd;
	pkg.ret = 0;
	pkg.uid  = user_id;

	if (p) {
		KDEBUG_LOG(p->user_id, "send to sw: [sw_cmd=%u] [wait_cmd=%u] [seq=%u]", cmd, p->wait_cmd, pkg.seq);
	}

	return net_send_msg(switch_fd, reinterpret_cast<char *>(&pkg), msg);
}


/* @brief 初始化命令处理函数列表
 */
void init_switch_handle_funs()
{
	g_switch_proto_map.init_list( g_switch_proto_list,sizeof(g_switch_proto_list)/sizeof(g_switch_proto_list[0]) );
}

/* @brief 首次连接switch，并向其发送gamesvr信息
 */
int connect_to_switch()
{
	switch_fd = connect_to_service(config_get_strval("switch"), 1, 65535, 1);
	if (switch_fd != -1) {
		sw_register_gamesvr_info();
	} else {
		const char *switch_ip = config_get_strval("switch_ip");
		if (switch_ip) {
			switch_fd = connect_to_svr(config_get_strval("switch_ip"), config_get_intval("switch_port", 0), 65536, 1);
			if (switch_fd != -1) {
				sw_register_gamesvr_info();
			}
		}
	}
	KDEBUG_LOG(0,"CONNECT SWITCH\t[fd=%d]",switch_fd);
	return 0;
}

/* @brief 添加定时器，3秒重连switch
 */
int connect_to_switch_timely(void *owner, void *data)
{
	if (switch_fd == -1) {
		connect_to_switch();
	}
	add_timer_event(0, connect_to_switch_timely, NULL, NULL, 2000);
	/*KERROR_LOG(0, "[owner:%p, data:%p, switch_fd:%d]", owner, data, switch_fd);
	if (!data) {
		ADD_TIMER_EVENT_EX(&timer_events, timer_connect_to_switch_timely, reinterpret_cast<void*>(1), get_now_tv()->tv_sec + 3);
	} else if (switch_fd == -1) {
		ADD_TIMER_EVENT_EX(&timer_events, timer_connect_to_switch, reinterpret_cast<void*>(0), get_now_tv()->tv_sec + 5);
	}*/
	return 0;
}


/************************************************************************/
/*                       Client  Request                                */
/************************************************************************/

/* @brief 添加定时器，30秒向switch发送心跳包
 */
int send_sw_keepalive_pkg(void *owner, void *data)
{
	//send_msg_to_switch(0, svr_proto_os_keepalive_cmd, 0, 0);   
	//ADD_TIMER_EVENT_EX(&timer_events, timer_send_sw_keepalive_pkg, 0, get_now_tv()->tv_sec + 30);
	return 0;
}

/* @brief gamesvr向switch注册信息
 */
int sw_register_gamesvr_info()
{
	sw_register_gamesvr_info_in out;
	out.id = static_cast<uint32_t>(get_server_id());
	snprintf(out.ip, sizeof(out.ip), "%s", get_server_ip());
	out.port = static_cast<uint32_t>(get_server_port());
	//TODO:
	//g_player_mgr.pack_player_uid_list(&out.players);

	DEBUG_LOG("register gamesvr info: [gamesvr_id=%u, ip=[%s], port=%u, usr_size=%lu] ",
		out.id, out.ip, out.port, out.players.size());

	return send_msg_to_switch(NULL, sw_register_gamesvr_info_cmd, &out, 0);
}

/* @brief 用户上线报告
 */
int sw_player_login_line(Player *p, uint32_t new_flag)
{
	sw_player_login_line_in out;
	out.gamesvr_id = static_cast<uint32_t>(get_server_id());
	out.new_flag = new_flag;
	//out.channle_id = p->channle_id;
	//out.domain = p->domain;
	return send_msg_to_switch(p, sw_player_login_line_cmd, &out, p->user_id);
}

/* @brief 用户下线报告
 */
int sw_player_logout_line(Player *p)
{
	/*if (!p->kick_off_flag) {
		sw_player_logout_line_in sw_in;
		sw_in.ip = get_client_ip(p->fdsess);
		sw_in.state = p->record_state;
		sw_in.domain = p->domain;
		return send_msg_to_switch(p, sw_player_logout_line_cmd, &sw_in, p->user_id);
	}*/
	return 0;
}

/* @brief 批量获取用户在线信息 
 */
/*int get_user_online_info_list(Player *p, std::vector<uint32_t>& user_list)
{
	sw_get_user_online_info_list_in sw_in;

	for (uint32_t i = 0; i < user_list.size(); i++) {
		sw_in.user_list.push_back(user_list[i]);
	}
	return send_msg_to_switch(p, sw_get_user_online_info_list_cmd, &sw_in, p->user_id);
}*/

/************************************************************************/
/*                       switch callback								*/
/************************************************************************/

/* @brief switch返回的被踢下线消息
 */
int sw_kick_player_off_line(Player *p, Cmessage *c_in, uint32_t ret)
{
	//p->kick_off_flag = true;
	//p->send_to_self(cli_kick_player_off_line_noti_cmd, NULL, 0);
	//KERROR_LOG(p->user_id, "kick usr offline");
	close_client_conn(p->fdsess->fd);
	return 0;
}

/* @brief switch返回的批量拉取用户在线信息
 */
/*int sw_get_user_online_info_list(Player *p, Cmessage *c_in, uint32_t ret)
{
	sw_get_user_online_info_list_out *p_in = P_IN;

	//托付系统查询是否在线
	if (p->wait_cmd == cli_love_ambassador_for_entrust_cmd ||
			p->wait_cmd == cli_upgrade_science_for_entrust_cmd) {
		if (p_in->online_info.size() != 1) {
        	p->send_to_self_error(p->wait_cmd, 10000000, 1);
		} else {
			set_player_entrust_action(p, &p_in->online_info[0]);
		}
		return 0;
	} else if (p->wait_cmd == cli_friend_one_key_entrust_cmd) {//一键帮助
		return p->friend_mgr.handle_friend_one_key_entrust_for_help(p, p_in->online_info);
	}

	cli_get_user_online_info_list_out cli_out;
	for (uint32_t i = 0; i < p_in->online_info.size(); i++) {
		cli_out.online_info.push_back(p_in->online_info[i]);
	}
	KDEBUG_LOG(p->user_id, "GET USER ONLINE INFO LIST\t[size=%u]", (uint32_t)p_in->online_info.size());
	return p->send_to_self(p->wait_cmd, &cli_out, 1);
}*/
