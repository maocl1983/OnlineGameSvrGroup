/**
 * =====================================================================================
 *
 * @file  stat_log.hpp
 *
 * @brief 统计信息
 *
 * compiler  gcc4.4.7
 *
 * platform  Linux
 * 		
 * =====================================================================================
 */

#ifndef  STAT_LOG_HPP_
#define  STAT_LOG_HPP_

extern "C"{
#include <inttypes.h>
#include <glib.h>
#include <libcommon/log.h>
#include <libcommon/statistic/msglog.h>
#include <libcommon/list.h>
}

/* 统计ID号 */
//--用户登录数据

/*统计×××人数,100000001~100099999*/
enum stat_id1_t {
	em_stat_id1_start     = 100000000, 

	em_stat_id1_end
};

/*统计×××人次,101000001~101999999*/
enum stat_id2_t {
	em_stat_id2_start = 101000000,
	em_stat_id2_1,
	em_stat_id2_2,
	em_stat_id2_3,
	em_stat_id2_4,
	em_stat_id2_5,

	em_stat_id2_end
};

/*统计×××人数/人次,102000001~102999999*/
enum stat_id3_t {
	em_stat_id3_start			 = 102000000,

	em_stat_id3_end
};

/*统计用户数据，如在线时长，103000001~103999999*/
enum stat_id4_t {
	em_stat_id4_start			 = 103000000,

	em_stat_id4_end
};


#pragma pack(1)
typedef struct stat_log_info {
	struct list_head log_list;
	uint32_t stat_id;
	uint32_t time;
	uint32_t len;
	uint8_t data[];
} stat_log_info_t;
#pragma pack()

typedef struct stat_log_list {
	list_head_t list;	//统计日志链表
	int count;			//当前链表元素个数
	int lock;			//链表锁
} stat_log_list_t;

//extern stat_log_list_t g_stat_log_list;

/* 统计文件 */
//extern char *stat_file;
//extern int stat_svr_fd;

void init_stat_log_list();

void statistic_person_time(uint32_t stat_id, uint32_t user_id);

int connect_to_stat_svr(); 

void send_to_stat_svr(uint32_t stat_id, const void *data, size_t len);

void* write_stat_log(void *data);


#endif   /* STAT_LOG_HPP_ */

