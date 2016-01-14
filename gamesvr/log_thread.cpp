extern "C" {
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <jemalloc/jemalloc.h>
}
#include "global_data.hpp"
#include "log_thread.hpp"

#define LOCK(q) while (__sync_lock_test_and_set(&(q)->lock,1)) {}
#define UNLOCK(q) __sync_lock_release(&(q)->lock);

#ifdef  likely
#undef  likely
#endif
#define likely(x) __builtin_expect(!!(x), 1)

#ifdef  unlikely
#undef  unlikely
#endif
#define unlikely(x) __builtin_expect(!!(x), 0)

#define MAX_LOG_CNT 10000000

//struct log_list_t g_log_list;


void init_log_thread_list()
{
	INIT_LIST_HEAD(&(g_log_list.list));
}

int add_to_log_list(int level, uint32_t key, const char *fmt, ...)
{
	if (g_log_list.count >= MAX_LOG_CNT) {
		return -1;
	}
	va_list ap;
	va_start(ap, fmt);
	char buf[8192] = {};
	int len = vsnprintf(buf, 8192, fmt, ap);
	if (unlikely(len >= 8192)) {
		len = 8192;
		buf[8191] = '\n';
	}

	log_list_info_t *log_info = (log_list_info_t*)malloc(sizeof(*log_info) + len);
	//log_list_info_t *log_info = (log_list_info_t*)g_slice_alloc(sizeof(*log_info) + len);
	INIT_LIST_HEAD(&(log_info->log_list));
	log_info->level = level;
	log_info->key = key;
	log_info->len = len;
	memcpy(log_info->data, buf, len);
	
	list_add_tail(&log_info->log_list, &(g_log_list.list));
	g_log_list.count++;

	return 0;
}

void remove_from_log_list(log_list_info_t *p_info)
{
	if (p_info->log_list.next != 0 && p_info->log_list.prev != 0) {
		list_del(&(p_info->log_list));
		free(p_info);
		//g_slice_free1(sizeof(*p_info) + p_info->len, p_info);
	}
}

/* @brief 日志线程函数 调用T_开头的函数 如T_DEBUG_LOG
 */
/*
void *write_log_thread(void *arg)
{
	pthread_detach(pthread_self());
	while (1) {
		if (g_log_list.count > 0) {
			LOCK(&g_log_list);
			list_head_t *l, *p;
			list_for_each_safe(l, p, &(g_log_list.list)) {
				log_list_info_t *p_info = list_entry(l, log_list_info_t, log_list);
				write_log_for_thread(p_info->level, p_info->key, p_info->data, p_info->len);

				//释放链表元素
				remove_from_log_list(p_info);
				g_log_list.count--;
			}		
			UNLOCK(&g_log_list);
		}
		usleep(1000);
	}
	pthread_exit(0);

	return NULL;
}*/

/* @brief 日志线程函数 调用T_开头的函数 如T_DEBUG_LOG
 */
void *write_log_thread(void *arg)
{
	pthread_detach(pthread_self());
	while (1) {
		if (g_log_list.count > 0) {
			list_head_t *head = &(g_log_list.list);
			if (head->next != head) {
				log_list_info_t *p_info = list_entry(head->next, log_list_info_t, log_list);
				write_log_for_thread(p_info->level, p_info->key, p_info->data, p_info->len);

				//释放链表元素
				remove_from_log_list(p_info);
				g_log_list.count--;
			}		
		}
		usleep(100);
	}
	pthread_exit(0);

	return NULL;
}
