/*
 * ============================================================
 *
 *  @file      redis.cpp
 *
 *  @brief     处理连接redis相关逻辑 
 * 
 *  compiler   gcc4.4.7
 *
 *  platform   Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved.
 *
 * ============================================================
 */

#include <libproject/protobuf/Cproto_map.h>
#include <libproject/protobuf/Cproto_msg.h>
#include <libproject/protobuf/Cproto_util.h>

#include "./proto/xseer_redis.hpp"
#include "./proto/xseer_online.hpp"

#include "global_data.hpp"
#include "redis.hpp"
#include "player.hpp"
#include "treasure_risk.hpp"

//Redis redis_mgr;

//-------------------------------------------------------
//函数定义
#undef  PROTO_FUNC_DEF
#define PROTO_FUNC_DEF(proto_name)\
	int proto_name( Player *p, Cmessage* p_in , uint32_t ret) ;
#include "./proto/xseer_redis_func_def.hpp" 

//命令绑定
typedef   int(*P_DEALFUN_T)( Player *p, Cmessage* c_in, uint32_t ret);

#undef  BIND_PROTO_CMD
#define BIND_PROTO_CMD(cmdid,proto_name,c_in,c_out,null)\
	{cmdid, new (c_out), proto_name },

Cproto< P_DEALFUN_T> g_redis_proto_list[]={
	#include "./proto/xseer_redis_bind_func.hpp" 
};
 
#include "./proto/xseer_redis_enum.hpp" 
#include "./proto/xseer_online_enum.hpp" 

//命令map
Cproto_map< Cproto< P_DEALFUN_T> >  g_redis_proto_map;


/* @brief 初始化DB的返回的处理函数
 */
void init_redis_handle_funs()
{
  	g_redis_proto_map.init_list(g_redis_proto_list,sizeof(g_redis_proto_list)/sizeof(g_redis_proto_list[0]));
}

/* @brief 向redis发送包
 * @param msg 发送redis的包体的内容，不包括包头
 */
int send_msg_to_redis(Player *p, int cmd, Cmessage *msg, uint32_t user_id)
{
	return send_msg_to_dbroute(p, cmd, msg, user_id);
}

/* @brief 处理redis返回的包，分发到相应的处理函数中
 * @param rs_pkg redis的返回包
 * @param len redis返回包的长度 
 */
//void handle_redis_return(db_proto_head_t *rs_pkg, uint32_t len)
void handle_redis_return(db_proto_head_t *rs_pkg, Player* p, uint32_t wait_cmd, int conn_fd, uint32_t len)
{
	/* 如无需等redis返回的命令处理到此结束*/
	if (!rs_pkg->seq_num || !rs_pkg->cmd) {
		return;
	}

	if (!p || p->wait_cmd != wait_cmd) {
		//KERROR_LOG(0, "CONNECTION HAS BEEN CLOSED [P = %p] [UID = %u] [CMD = 0x%X %u] [seq=%x] [fd = %u]", 
			//p, rs_pkg->user_id, rs_pkg->cmd, wait_cmd, rs_pkg->seq_num, conn_fd);
		return;
	}
	KDEBUG_LOG(0, "redis return\t[user id = %u %u cmd = %u %u ret =%u]",
	         p->user_id, rs_pkg->user_id, wait_cmd, rs_pkg->cmd, rs_pkg->ret);

	//----------------------------------------------------
	Cproto< P_DEALFUN_T> * p_proto_item =g_redis_proto_map.getitem( rs_pkg->cmd );
	if (p_proto_item == NULL) {
		KDEBUG_LOG(0, "redis cmd id not find: %u", rs_pkg->cmd);
		return;
	}

	Cmessage * msg;

	if (rs_pkg->ret == 0) {//成功
		//还原对象
		p_proto_item->proto_msg->init( );

		Cbuff_array in_ba((char *)rs_pkg->body, rs_pkg->len - sizeof(db_proto_head_t));

		//失败
		if (!p_proto_item->proto_msg->read_from_buf(in_ba)) {
			KERROR_LOG(rs_pkg->user_id, "db 还原对象失败, cmd=%u", rs_pkg->cmd);
			return;	
		}

		//客户端多上传报文
		if (!in_ba.is_end()) {
			KERROR_LOG(rs_pkg->user_id, "db re 过多报文, cmd=%u", rs_pkg->cmd);
			return;
		}
		msg=p_proto_item->proto_msg;
	} else {
		msg = NULL;
	}

	int ret = p_proto_item->func(p, msg, rs_pkg->ret);	
	if (ret) {
		close_client_conn(p->fdsess->fd);
	}
}

/************************************************************************/
/*                           Redis Class                                */
/************************************************************************/
Redis::Redis()
{

}

Redis::~Redis()
{

}

/* @brief 设置用户昵称 
 */
int
Redis::set_user_nick(Player* p)
{
	rs_set_user_nick_in rs_in;
	memcpy(rs_in.nick, p->nick, NICK_LEN);
	return send_msg_to_redis(0, rs_set_user_nick_cmd, &rs_in, p->user_id);
}

/* @brief 获取用户昵称列表 
 */
int
Redis::get_user_nick_list(Player* p, std::vector<uint32_t>& user_ids)
{
	rs_get_user_nick_list_in rs_in;

	for (uint32_t i = 0; i < user_ids.size(); i++) {
		rs_in.users.push_back(user_ids[i]);
	}

	return send_msg_to_redis(p, rs_get_user_nick_list_cmd, &rs_in, p->user_id);
}

/* @brief 设置玩家主角等级
 */
int
Redis::set_user_level(Player *p)
{
	rs_set_user_level_in rs_in;
	rs_in.lv = p->lv;
	return send_msg_to_redis(0, rs_set_user_level_cmd, &rs_in, p->user_id);
}

/* @brief 获取用户主角等级列表 
 */
int
Redis::get_user_level_list(Player* p, std::vector<uint32_t>& user_ids)
{
	rs_get_user_level_list_in rs_in;

	for (uint32_t i = 0; i < user_ids.size(); i++) {
		rs_in.users.push_back(user_ids[i]);
	}

	return send_msg_to_redis(p, rs_get_user_level_list_cmd, &rs_in, p->user_id);
}

/* @brief 设置碎片玩家
 */
int
Redis::set_treasure_piece_user(Player *p, uint32_t piece_id)
{
	rs_set_treasure_piece_user_in rs_in;
	rs_in.piece_id = piece_id;

	return send_msg_to_redis(0, rs_set_treasure_piece_user_cmd, &rs_in, p->user_id);
}

/* @brief 删除碎片玩家
 */
int
Redis::del_treasure_piece_user(Player *p, uint32_t piece_id)
{
	rs_del_treasure_piece_user_in rs_in;
	rs_in.piece_id = piece_id;

	return send_msg_to_redis(0, rs_del_treasure_piece_user_cmd, &rs_in, p->user_id);
}

/* @brief 获取碎片玩家列表
 */
int
Redis::get_treasure_piece_user_list(Player *p, uint32_t piece_id)
{
	rs_get_treasure_piece_user_list_in rs_in;
	rs_in.piece_id = piece_id;
	rs_in.lv = p->lv;

	return send_msg_to_redis(p, rs_get_treasure_piece_user_list_cmd, &rs_in, p->user_id);
}

/* @brief 设置夺宝保护时间
 */
int
Redis::set_treasure_protection_time(Player *p, uint32_t protection_tm)
{
	rs_set_treasure_protection_time_in rs_in;
	rs_in.protection_tm = protection_tm;

	return send_msg_to_redis(0, rs_set_treasure_protection_time_cmd, &rs_in, p->user_id);
}

/************************************************************************/
/*                       Client  Request                                */
/************************************************************************/

/************************************************************************/
/*                       redis sever return                             */
/************************************************************************/

int rs_set_user_nick(Player *p, Cmessage* c_in, uint32_t ret)
{
	CHECK_REDIS_ERR(p,ret);

	return 0;
}

int rs_get_user_nick_list(Player *p, Cmessage* c_in, uint32_t ret)
{
	CHECK_REDIS_ERR(p,ret);

	return 0;
}

int rs_set_user_level(Player *p, Cmessage* c_in, uint32_t ret)
{
	CHECK_REDIS_ERR(p,ret);

	return 0;
}

int rs_get_user_level_list(Player *p, Cmessage* c_in, uint32_t ret)
{
	CHECK_REDIS_ERR(p,ret);

	return 0;
}

int rs_set_treasure_piece_user(Player *p, Cmessage* c_in, uint32_t ret)
{
	CHECK_REDIS_ERR(p,ret);

	return 0;
}

int rs_del_treasure_piece_user(Player *p, Cmessage* c_in, uint32_t ret)
{
	CHECK_REDIS_ERR(p,ret);

	return 0;
}

int rs_get_treasure_piece_user_list(Player *p, Cmessage* c_in, uint32_t ret)
{
	CHECK_REDIS_ERR(p,ret);

	rs_get_treasure_piece_user_list_out* p_in = P_IN;

	if (p->wait_cmd == cli_get_treasure_risk_recommend_players_cmd) {
		return p->treasure_mgr->handle_treasure_piece_user_list_redis_return(p_in);
	}

	return 0;
}

int rs_set_treasure_protection_time(Player *p, Cmessage *c_in, uint32_t ret)
{
	CHECK_REDIS_ERR(p,ret);

	return 0;
}
