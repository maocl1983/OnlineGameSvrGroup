/**
 * =====================================================================================
 *
 * @file  alarm.hpp
 *
 * @brief 处理和alarm服务器相关的命令
 *
 * compiler  GCC4.1.2
 * 
 * platform  Linux
 *
 * =====================================================================================
 */

#ifndef  ALARM_HPP_
#define  ALARM_HPP_

#include <libproject/protobuf/Cproto_map.h>
#include <libproject/protobuf/Cproto_msg.h>
#include <libproject/protobuf/Cproto_util.h>

#pragma pack(1)
/** @brief 服务器间通信协议头
  */
struct alarm_proto_head_t {
	uint16_t len; 		/* 协议总长度 */
	uint16_t game_id; 	/* 游戏ID */
	uint8_t body[];		/* 包体 */
};
#pragma pack()
//extern int alarm_fd;

int connect_to_alarm();
int connect_to_alarm_timely(void *owner, void *data);
int send_msg_to_alarm(const char *msg);
#endif   /* ALARM_HPP_ */

