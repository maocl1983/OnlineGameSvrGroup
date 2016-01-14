/*
 * =====================================================================================
 *
 *  @file  dbroute.hpp 
 *
 *  @brief  跟dbroute相关的函数 
 *
 *  compiler  gcc4.4.7 
 *	
 *  platform  Linux
 *
 *
 * =====================================================================================
 */

#ifndef DBPROXY_HPP_
#define DBPROXY_HPP_

#include "common_def.hpp"

class Player;


enum udp_socket{
	udp_socket_cli_chat	= 0, /* udp类型：客户端聊天 */
	cnc_udp_record_online_id = 1, /* 记录网通用户登入的online的ID号 */
	tel_udp_record_online_id = 2, /* 记录电信用户登入的online的ID号 */
	udp_submit_contribution = 3, /* 投稿 */
	udp_report_cmd = 4, /* 汇报命令频率 */
	udp_report_chat_content = 5, /* 上报聊天的内容 */
	max_udp_socket,  /* udp最大个数 */
};

#define CHECK_DB_ERR(p_, err_) \
	do { \
		if ((err_)) { \
			return p_->send_to_self_error(p_->wait_cmd, err_, 1); \
		} \
	} while(0)


enum tcp_socket_index_t {
	gf_dbroute_wt_fd_index = 0,
	gf_dbroute_cnc_fd_index = 1,
};

/* @brief 表示TCP连接的参数
 */
struct fd_ip_port_t {
	int fd;
	char *ip;
	int port;
};

#pragma pack(1)

/* 跟DB交互的协议格式 */
struct db_proto_head_t {
	uint32_t len; /* 协议的长度 */
    uint32_t seq_num; /* 序列号 */
    uint16_t cmd; /* 命令号 */
    uint16_t sid; /* 区服id */
    uint32_t ret; /* DB返回的错误码 */
    uint32_t user_id; /* 路由ID */
    uint8_t body[]; /* 包体信息 */
};

#pragma pack()

class Cmessage;

int send_msg_to_dbroute(Player *p, int cmd, Cmessage *msg,  uint32_t user_id, bool is_global=false);

void init_db_handle_funs();

int	init_all_udp_sockets();

int send_udp_msg_to_dbroute(int cmd, Cmessage *msg,  uint32_t user_id, uint8_t index);

void handle_db_return(db_proto_head_t *db_pkg, uint32_t len);

int db_return_expired(void *owner, void *data);

int init_udp_socket(uint8_t flag);

int send_msg_by_tcp(Player *p, tcp_socket_index_t index, const void *buf, int total);

void init_tcp_socket();

int is_global_db_cmd(uint32_t db_cmd);

//extern int dbroute_fd;

#endif
