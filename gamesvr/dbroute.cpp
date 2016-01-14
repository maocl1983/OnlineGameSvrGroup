/*
 * =====================================================================================
 *
 *  @file  dbroute.cpp 
 *
 *  @brief  处理DB相关的命令 
 *
 *  compiler  gcc4.4.7 
 *	
 *  platform  Linux
 *
 * copyright:  kings, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */
#include <libproject/protobuf/Cproto_map.h>
#include <libproject/protobuf/Cproto_msg.h>
#include <libproject/protobuf/Cproto_util.h>

#include "./proto/xseer_online.hpp"
#include "./proto/xseer_db.hpp"
#include "./proto/account_db.hpp"

#include "global_data.hpp"
#include "dbroute.hpp"
#include "player.hpp"
#include "redis.hpp"

//-------------------------------------------------------
//函数定义
#undef  PROTO_FUNC_DEF
#define PROTO_FUNC_DEF(proto_name)\
	int proto_name( Player *p, Cmessage *p_in , uint32_t ret) ;
#include "./proto/xseer_db_func_def.hpp" 

//命令绑定
typedef   int(*P_DEALFUN_T)( Player *p, Cmessage *c_in, uint32_t ret);

#undef  BIND_PROTO_CMD
#define BIND_PROTO_CMD(cmdid,proto_name,c_in,c_out,null)\
	{cmdid, new (c_out), proto_name },

Cproto< P_DEALFUN_T> g_route_proto_list[]={
	//#include "./proto/xseer_db_bind_func.hpp" 
#include "./proto/xseer_db_for_gs_bind_func.hpp" 
};
 
#include "./proto/xseer_db_enum.hpp" 
#include "./proto/xseer_online_enum.hpp" 
//命令map
Cproto_map< Cproto< P_DEALFUN_T> >  g_route_proto_map;

//int dbroute_fd = -1;

/* @brief 初始化DB的返回的处理函数
 */
void init_db_handle_funs()
{
  	g_route_proto_map.init_list(g_route_proto_list,sizeof(g_route_proto_list)/sizeof(g_route_proto_list[0]));
}

/* @brief db命令超时
 */
int db_return_expired(void *owner, void *data)
{
	/*Player *p = (Player *)owner;
	KERROR_LOG(p->user_id, "db timeout\t[wait_cmd = %u, db_cmd = %u]", p->db_timer_wait_cmd, p->db_timer_db_cmd);
	p->db_timer = NULL;*/
	
	//send_warning("db_timeout", p->user_id, p->db_timer_wait_cmd, p->db_timer_db_cmd, 0);
	return 0;
}



/* @brief 向DB发送包
 * @param msg 发送DB的包体的内容，不包括包头
 */
int send_msg_to_dbroute(Player *p, int cmd, Cmessage *msg, uint32_t user_id, bool is_global)
{
	if (dbroute_fd == -1) {
		dbroute_fd = connect_to_service(config_get_strval("dbroute"), 0, 65535, 1);
	}
	if (dbroute_fd == -1) {
		KERROR_LOG(user_id, "send to dbroute failed [fd = %d] [wait_cmd = %u] [db_cmd = %X]",
				dbroute_fd, p ? p->wait_cmd :0,  cmd);
		if (p) {
			return p->send_to_self_error(p->wait_cmd, cli_connect_db_err, 1);		
		} else {
			Player* usr = g_player_mgr->get_player_by_uid(user_id);
			if (usr){
				usr->wait_cmd = 0;
				return 0;
			}
		}
		return -1;
	}

	db_proto_head_t  pkg;
	pkg.len = sizeof(db_proto_head_t);
	pkg.seq_num = (p ? ((p->fdsess->fd << 16) | p->wait_cmd) : 0);
	pkg.sid = is_global ? 0: g_server_id;
	pkg.cmd = cmd;
	pkg.ret = 0;
	pkg.user_id = user_id;

	//KTRACE_LOG(p->user_id, "send db buf[%u %x][%u %u]",pkg.cmd, pkg.seq_num, p->fdsess->fd, p->wait_cmd);
	return net_send_msg(dbroute_fd, reinterpret_cast<char *>(&pkg), msg);
}


int is_global_db_cmd(uint32_t db_cmd)
{
	if (db_cmd == db_get_arena_count_cmd
		|| db_cmd == db_get_arena_list_cmd
		|| db_cmd == db_get_guild_count_cmd
		|| db_cmd == db_get_guild_list_cmd
		|| db_cmd == db_get_guild_member_list_cmd
		) {
		return 1;
	}
	return 0;
}

/* @brief 处理DB返回的包，分发到相应的处理函数中
 * @param db_pkg DB的返回包
 * @param len DB返回包的长度 
 */
void handle_db_return(db_proto_head_t *db_pkg, uint32_t len)
{
	uint32_t wait_cmd = db_pkg->seq_num & 0xFFFF;
	int conn_fd = db_pkg->seq_num >> 16;
	Player *p = g_player_mgr->get_player_by_fd(conn_fd);
	/* DB返回之前客户端已关闭, 新的玩家进来, 此时拿到的p是新玩家 */
	/*if (p && p->user_id != db_pkg->user_id) {
		return;
	}*/

	/* 如无需等DB返回的命令处理到此结束 */
	if ((!db_pkg->seq_num || !db_pkg->cmd) && !is_global_db_cmd(db_pkg->cmd)) {
		return;
	}

	/* 判断是否是redis的命令 */
	if (db_pkg->cmd > 0x1E00 && db_pkg->cmd < 0x1FFF) {
		return handle_redis_return(db_pkg, p, wait_cmd, conn_fd, len);
	}

	/*if (p && p->db_timer) {
		REMOVE_TIMER(p->db_timer);
		p->db_timer = NULL;
	}*/
	if (!p || p->wait_cmd != wait_cmd) {
		if (!is_global_db_cmd(db_pkg->cmd)) {
			KERROR_LOG(0, "CONNECTION HAS BEEN CLOSED [P = %p] [UID = %u] [CMD = 0x%X %u] [seq=%x] [fd = %u]", 
				p, db_pkg->user_id, db_pkg->cmd, wait_cmd, db_pkg->seq_num, conn_fd);
			return;
		}
	}

	//防止登录错误交叉
#if 0
	if (p && p->wait_cmd == cli_proto_login_cmd && p->user_id != db_pkg->user_id) {
		KERROR_LOG(p->user_id, "db return error\t[user id = %u %u cmd = %u %#x]",
				p->user_id, db_pkg->user_id, wait_cmd, db_pkg->cmd);
		return;
	}
#endif


	KDEBUG_LOG(0, "db return\t[user id = %u %u cmd = %u %#x ret =%u]",
	         (p ? p->user_id : 0), db_pkg->user_id, wait_cmd, db_pkg->cmd, db_pkg->ret);

	//----------------------------------------------------
	Cproto< P_DEALFUN_T> * p_proto_item =g_route_proto_map.getitem( db_pkg->cmd );
	if (p_proto_item == NULL) {
		KDEBUG_LOG(0, "db cmd id not find: %#x", db_pkg->cmd);
		return;
	}

	Cmessage * msg;

	if (db_pkg->ret == 0) {//成功
		//还原对象
		p_proto_item->proto_msg->init( );

		Cbuff_array in_ba((char *)db_pkg->body, db_pkg->len - sizeof(db_proto_head_t));

		//失败
		if (!p_proto_item->proto_msg->read_from_buf(in_ba)) {
			KERROR_LOG(db_pkg->user_id, "db 还原对象失败, cmd=%u", db_pkg->cmd);
			return;	
		}

		//客户端多上传报文
		if (!in_ba.is_end()) {
			KERROR_LOG(db_pkg->user_id, "db re 过多报文, cmd=%u len=%u", db_pkg->cmd, db_pkg->len);
			return;
		}
		msg=p_proto_item->proto_msg;
	} else {
		msg = NULL;
		//deal_frd_no_found_err(p, db_pkg->cmd, db_pkg->user_id, db_pkg->ret);
	}
	int ret = p_proto_item->func(p, msg, db_pkg->ret);	
	if (ret && p) {
		close_client_conn(p->fdsess->fd);
	}
}


/************************************************************************/
/*                       db sever return                            */
/************************************************************************/

