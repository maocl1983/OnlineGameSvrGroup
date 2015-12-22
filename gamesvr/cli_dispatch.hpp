/*
 * =====================================================================================
 *
 *  @file  cli_dispatch.hpp 
 *
 *  @brief  初始化处理函数，及分发处理命令 
 *
 *  compiler  gcc4.4.7 
 *	
 *  platform  Linux
 *
 * =====================================================================================
 */

#ifndef CLI_DISPATCH_HPP_
#define CLI_DISPATCH_HPP_

extern "C" {
#include <inttypes.h>
}

enum {
	/* 版本号 */
	ver = 99,
	/*SERVER和CLIENT协议包的最大长度 */
	cli_proto_max_len = 32 * 1024,
};

#pragma pack(1)

/* SERVER和CLIENT的协议包头格式 */
struct cli_proto_head_t {
	uint32_t len; /* 协议的长度 */
	uint32_t cmd; /* 协议的命令号 */
	uint32_t user_id; /* 用户的米米号 */
	uint32_t seq_num;/* 序列号 */
	uint32_t ret; /* S->C, 错误码 */
	uint8_t body[]; /* 包体信息 */
};

#pragma pack()

/* @brief 根据不同的命令号调用相应的处理函数
 */
int dispatch(void *data, fdsession_t *fdsess, bool first_tm = true);

/* @brief 初始化处理客户端数据的函数数组
 */
void init_cli_handle_funs();

/* @brief 向client发送包
 */
//int send_msg_to_cli(Player *p, uint16_t cmd, Cmessage* msg);

#endif
