/*
 * ============================================================
 *
 *  @file      redis.hpp
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

#ifndef REDIS_HPP
#define REDIS_HPP

#include "common_def.hpp"
#include "dbroute.hpp"

class Player;
class Cmessage;

#define CHECK_REDIS_ERR(p_, err_) \
	do { \
		if ((err_)) { \
			return p_->send_to_self_error(p_->wait_cmd, err_, 1); \
		} \
	} while(0)



void init_redis_handle_funs();
void handle_redis_return(db_proto_head_t *rs_pkg, Player* p, uint32_t wait_cmd, int conn_fd, uint32_t len);
int send_msg_to_redis(Player *p, int cmd, Cmessage* msg,  uint32_t user_id);

/* @brief Redis管理类
 */
class Redis {
public:
    Redis();
    ~Redis();

	int set_user_nick(Player* p);
	int get_user_nick_list(Player* p, std::vector<uint32_t>& user_ids);

	int set_user_level(Player *p);
	int get_user_level_list(Player* p, std::vector<uint32_t>& user_ids);

	int set_treasure_protection_time(Player *p, uint32_t protcetion_tm);

	int set_treasure_piece_user(Player *p, uint32_t piece_id);
	int del_treasure_piece_user(Player *p, uint32_t piece_id);
	int get_treasure_piece_user_list(Player *p, uint32_t piece_id);
};

extern Redis redis_mgr;

#endif
