/*
 * =====================================================================================
 *
 *  @file  cli_dispatch.cpp 
 *
 *  @brief  初始化协议处理函数和分发协议处理函数
 *
 *  compiler  gcc4.4.7 
 *	
 *  platform  Linux
 *
 * copyright:  kings, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

extern "C"{
#include <sys/types.h>
#include <sys/socket.h>
//#include <async_serv/net_if.h>
#include <libcommon/log.h>
}
#include <asyn_serv/net_if.hpp>

#include <libproject/protobuf/Cproto_map.h>
#include <libproject/protobuf/Cproto_msg.h>
#include <libproject/protobuf/Cproto_util.h>
#include <libproject/utils/strings.hpp>
#include <libproject/utils/tcpip.h>

#include "cli_dispatch.hpp"
#include "player.hpp"


//函数定义
#define PROTO_FUNC_DEF(proto_name)\
	int proto_name(Player *p, Cmessage *p_in ) ;
#include "./proto/xseer_online_func_def.hpp" 

//对应的结构体
#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"

#undef  BIND_PROTO_CMD
#define BIND_PROTO_CMD(cmdid,proto_name,c_in,c_out,null)\
	{cmdid, new (c_in), proto_name },

//命令绑定
typedef   int(*P_DEALFUN_T)( Player *p, Cmessage *p_in );

Cproto< P_DEALFUN_T> g_proto_list[]={
#include "./proto/xseer_online_bind_func.hpp"
};
//命令map
Cproto_map< Cproto< P_DEALFUN_T> >  g_proto_map;

std::string g_version_str("11101322");

/* @brief 根据不同的命令号调用相应的处理函数
 */
int dispatch(void *data, fdsession_t *fdsess, bool first_tm)
{
	cli_proto_head_t *head = (cli_proto_head_t *)data;

	// protocol for testing if this server is OK
	if (head->cmd == 101) {
		return send_pkg_to_client(fdsess, head, head->len);    
	}

	/*
	Player *p = g_player_mgr.get_player_by_uid(head->user_id);
	if (p) {
		p->fdsess = fdsess;
		p->wait_cmd = head->cmd;
	}*/
	Player *p = g_player_mgr.get_player_by_fd(fdsess->fd);
	if ((head->cmd != cli_proto_login_cmd && !p) || (head->cmd == cli_proto_login_cmd && p)|| (p && (p->user_id != head->user_id))) {
		KERROR_LOG(head->user_id, "pkg err [cmd = %u] [p addr %p]", head->cmd, p);
		return -1;
	}
	if (first_tm && p) {
		p->seqno = head->seq_num;
	}
	
	if (head->cmd == cli_proto_login_cmd) {
		Player player(head->user_id, fdsess);

		p = &player;
		p->wait_cmd = head->cmd;
		p->seqno = head->seq_num;
	} else {
		if (p && !p->login_completed) {
			KERROR_LOG(p->user_id, "login not completed, please wait");
			return p->send_to_self_error(head->cmd, cli_player_login_not_completed_err, 1);
		}
	}

	Cproto< P_DEALFUN_T> * p_proto_item =g_proto_map.getitem(head->cmd);
	if (p_proto_item == NULL) {
		KERROR_LOG(head->user_id, "cli cmd id not find: %u", head->cmd);
		return -1;
	}


	//还原对象
	p_proto_item->proto_msg->init( );

	Cbuff_array in_ba ( ((char*)data)+sizeof(cli_proto_head_t),
		head->len-sizeof(cli_proto_head_t));
	//失败,报文长度不符
	if (!p_proto_item->proto_msg->read_from_buf(in_ba)) {
		KERROR_LOG(head->user_id, "client 报文长度不够 cmd:%u", head->cmd);
		return -1;	
	}

	//客户端多上传报文
	if (!in_ba.is_end()) {
		KERROR_LOG(head->user_id, "client 过多报文 cmd:%u len=%u", head->cmd, head->len);
		return  -1;
	}
	
	p->wait_cmd = head->cmd;

	KTRACE_LOG(p->user_id, "cmd=%u", p->wait_cmd);

	return p_proto_item->func(p, p_proto_item->proto_msg);	
}


/* @brief 初始化处理客户端数据的函数数组
 */
void init_cli_handle_funs()
{
  	g_proto_map.init_list(g_proto_list,sizeof(g_proto_list)/sizeof(g_proto_list[0]));
}
