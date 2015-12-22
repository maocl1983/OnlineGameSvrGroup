/**
 * =====================================================================================
 *
 * @file  switch.hpp
 *
 * @brief 处理和switch服务器相关的命令
 *
 * compiler  GCC4.1.2
 * 
 * platform  Linux
 *
 * =====================================================================================
 */

#ifndef  SWITCH_HPP_
#define  SWITCH_HPP_

#include <libproject/protobuf/Cproto_map.h>
#include <libproject/protobuf/Cproto_msg.h>
#include <libproject/protobuf/Cproto_util.h>

#include "player.hpp"



//-------------------------------------------------------
//函数定义
#undef  BIND_PROTO_CMD
#define BIND_PROTO_CMD(cmdid,proto_name,c_in,c_out,NULL)\
	int proto_name(Player *p,  Cmessage *p_in);
//#include "./proto/xseer_switch_bind_for_online.h" 

#pragma pack(1)
/** @brief 服务器间通信协议头
  */
struct sw_proto_head_t {
	uint32_t len; /*协议总长度*/
	uint32_t seq; /*序列号*/
	uint16_t cmd; /*命令号*/
	uint32_t ret; /*返回值*/
	uint32_t uid; /*米米号*/
};
#pragma pack()
extern int switch_fd;

class Cmessage;

int connect_to_switch();
int connect_to_switch_timely(void *owner, void *data);
void init_switch_handle_funs();
void handle_switch_return(sw_proto_head_t * data, uint32_t len);
int send_msg_to_switch(Player *p, uint16_t cmd, Cmessage *msg,  uint32_t user_id);
int send_sw_keepalive_pkg(void *owner, void *data);
int sw_register_gamesvr_info();

int sw_player_login_line(Player *p, uint32_t flag);
int sw_player_logout_line(Player *p);
#endif   /* SWITCH_HPP_ */

