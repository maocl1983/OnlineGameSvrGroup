/**
 * =====================================================================================
 *
 * @file  stat_log.cpp
 *
 * @brief 统计信息
 *
 * compiler  gcc4.4.7
 * 
 * platform  Linux
 *
 * copyright:  kings, Inc. ShangHai CN. All rights reserved.
 * 		
 * =====================================================================================
 */

extern "C"{
#include <libcommon/conf_parser/config.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
}


#include "./proto/xseer_online.hpp"
#include "./proto/xseer_online_enum.hpp"

#include "global_data.hpp"
#include "common_def.hpp"
#include "stat_log.hpp"

//char *stat_file;
//int stat_svr_fd = -1;

#define MAX_STAT_LOG_LIST_COUNT 8192

#define LOCK(q) while (__sync_lock_test_and_set(&(q)->lock,1)) {}
#define UNLOCK(q) __sync_lock_release(&(q)->lock);

//stat_log_list_t g_stat_log_list;

static inline 
int add_to_msglog_list(uint32_t stat_id, const void *data, size_t len)
{
	if (g_stat_log_list.count >= MAX_STAT_LOG_LIST_COUNT) {
		ERROR_LOG("stat log reach the max count of list, count=%u", g_stat_log_list.count);
		return -1;
	}
	stat_log_info_t *log_info = (stat_log_info_t*)g_slice_alloc(sizeof(*log_info) + len);
	INIT_LIST_HEAD(&(log_info->log_list));
	log_info->stat_id = stat_id;
	log_info->len = len;
	log_info->time = get_now_tv()->tv_sec;
	memcpy(log_info->data, data, len);

	LOCK(&g_stat_log_list);
	list_add_tail(&log_info->log_list, &(g_stat_log_list.list));
	g_stat_log_list.count++;
	UNLOCK(&g_stat_log_list);

	return 0;
}

static inline 
void remove_from_msglog_list(stat_log_info_t *p_info)
{
	if (p_info->log_list.next != 0) {
		list_del(&(p_info->log_list));
		g_slice_free1(sizeof(*p_info) + p_info->len, p_info);
	}
}

static inline 
int make_sure_dir_exists(const char *pathname )
{
	char buf[8192] = {0}; 
	uint32_t len = strlen(pathname);
	if (len > 8191) {
		return -1;
	}
	strncpy(buf, pathname, len); 

	char *p = buf;
	while (p) {
		char *t=strchr(p,'/');
		if (!t) {
			break;
		}
		*t = 0; 
		if (access(buf, 0) == -1) {
			if (mkdir(buf, 0777) == -1) {
				return -1;
			}
		}
		*t = '/';

		while ( *t == '/') {
			++t;
		}
		p = t;
	}
	return 0;
}


void init_stat_log_list()
{
	INIT_LIST_HEAD(&(g_stat_log_list.list));
}

/* @brief 统计日志线程函数
 */
void* 
write_stat_log(void *data)
{
	pthread_detach(pthread_self());
	while (1) {
		if (g_stat_log_list.count > 0) {
			LOCK(&g_stat_log_list);
			list_head_t *l, *p;
			list_for_each_safe(l, p, &(g_stat_log_list.list)) {
				stat_log_info_t *p_info = list_entry(l, stat_log_info_t, log_list);
				char pathname[1024] = {0};
				char filename[8192] = {0};
				int type = 0;
				if (p_info->stat_id > em_stat_id1_start && p_info->stat_id < em_stat_id1_end) {
					type = 1;
				} else if (p_info->stat_id > em_stat_id2_start && p_info->stat_id < em_stat_id2_end) {
					type = 2;
				} else if (p_info->stat_id > em_stat_id3_start && p_info->stat_id < em_stat_id3_end) {
					type = 3;
				} else if (p_info->stat_id > em_stat_id4_start && p_info->stat_id < em_stat_id4_end) {
					type = 4;
				}
				struct tm tm_result;
				time_t time = p_info->time;
				localtime_r(&time, &tm_result);
				sprintf(pathname, "%s%04d%02d%02d/%d/", 
						stat_file, tm_result.tm_year + 1900, tm_result.tm_mon + 1, tm_result.tm_mday, g_server_id);
				make_sure_dir_exists(pathname);
				sprintf(filename, "%sstat_%d", pathname, type);

				DEBUG_LOG("%s, %d", filename, p_info->stat_id);
				statistic_log(filename, g_server_id, p_info->stat_id, p_info->time, p_info->data, p_info->len);
				//释放链表
				remove_from_msglog_list(p_info);
				g_stat_log_list.count--;
			}
			UNLOCK(&g_stat_log_list);
		}
		//sleep 10ms
		usleep(10000);
	}
	pthread_exit(0);

	return NULL;
}

static inline void 
statistic_msglog(uint32_t stat_id, const void *data, size_t len)
{
	//写入文件
	add_to_msglog_list(stat_id, data, len);

	//发送至统计服务器
	send_to_stat_svr(stat_id, data, len);
}

void 
statistic_person_time(uint32_t stat_id, uint32_t user_id)
{
	uint32_t stat_tmp[] = {user_id};
	statistic_msglog(stat_id, stat_tmp, sizeof(stat_tmp));
}

void send_to_stat_svr(uint32_t stat_id, const void *data, size_t len)
{
	if (stat_svr_fd == -1) {
		stat_svr_fd = connect_to_stat_svr();
		if (stat_svr_fd == -1) {
			KERROR_LOG(0, "connect stat svr error!");
			return;
		}
	}

	char stat_buff[1024];
	stat_msg_header_t *head = reinterpret_cast<stat_msg_header_t*>(stat_buff);
	head->len = len + sizeof(stat_msg_header_t);
	head->sid = g_server_id;
	head->msg_id = stat_id;
	head->timestamp = get_now_tv()->tv_sec;
	head->from_type = 0;
	if (head->len > 1000) {
		KERROR_LOG(0, "stat msg len error[%d]",head->len);
		return;
	}
	memcpy(head->data, data, len);
	net_send(stat_svr_fd, stat_buff, head->len);
	return;
}

/* @brief 首次连接stat svr
 */
int connect_to_stat_svr()
{
	const char *stat_svr_ip = config_get_strval("stat_svr_ip");
	if (stat_svr_ip) {
		stat_svr_fd = connect_to_svr(config_get_strval("stat_svr_ip"), config_get_intval("stat_svr_port", 0), 65536, 1);
	}
	KDEBUG_LOG(0,"CONNECT STAT SVR\t[fd=%d]",stat_svr_fd);
	return stat_svr_fd;
}


/********************************************************************************/
/*							Client Request										*/
/********************************************************************************/

/* @brief 客户端统计协议 
 */
/*
int cli_statistic(Player *p, Cmessage *c_in)
{
	cli_statistic_in *p_in = P_IN;

	if (p_in->id < 100000000 || p_in->id > 200000000) {
		return p->send_to_self(p->wait_cmd, NULL, 1);
	}

	uint32_t stat_tmp[] = {p->user_id};
	statistic_msglog(p_in->id, stat_tmp, sizeof(stat_tmp));
	return p->send_to_self(p->wait_cmd, NULL, 1);
}
*/
