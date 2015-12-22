#ifndef LOG_THREAD_HPP_
#define LOG_THREAD_HPP_

extern "C" {
#include <stdint.h>
#include <libcommon/list.h>
#include <libcommon/log.h>
}

#pragma pack(1)
typedef struct log_list_info {
	list_head_t log_list;
	uint8_t level;
	uint32_t key;
	int len;
	char data[];
}log_list_info_t;
#pragma pack()

struct log_list_t {
	list_head_t list;
	int count;
	int lock;
};

extern struct log_list_t g_log_list;

inline
void init_log_thread_list()
{
	INIT_LIST_HEAD(&(g_log_list.list));
}

int add_to_log_list(int level, uint32_t key, const char *fmt, ...) LOG_CHECK_FMT(3, 4);

void* write_log_thread(void *arg);


#ifndef LOG_USE_SYSLOG
#define TLOG_DETAIL(level, key, fmt, args...) \
	add_to_log_list(level, key, "[%s][%d]%s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define TLOG_DETAIL(level, key, fmt, args...) \
	write_syslog(level, "[%s][%d]%s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#endif

//线程LOG
#ifndef LOG_USE_SYSLOG
#define TLOG(level, key, fmt, args...) \
	add_to_log_list(level, key, fmt "\n", ##args)
#else
#define TLOG(level, key, fmt, args...) \
	write_syslog(level, fmt "\n", ##args)
#endif

//emerg
#ifndef DISABLE_EMERG_LOG
#define T_EMERG_LOG(fmt, args...) \
	TLOG_DETAIL(log_lvl_emerg, 0, fmt, ##args)
#define T_KEMERG_LOG(key, fmt, args...) \
	TLOG_DETAIL(log_lvl_emerg, key, fmt, ##args)
#else
#define T_EMERG_LOG(fmt, args...)
#define T_KEMERG_LOG(key, fmt, args...) 
#endif


//alert
#ifndef DISABLE_ALERT_LOG
#define T_ALERT_LOG(fmt, args...) \
	TLOG_DETAIL(log_lvl_alert, 0, fmt, ##args)
#define T_KALERT_LOG(key, fmt, args...) \
	TLOG_DETAIL(log_lvl_alert, key, fmt, ##args)
#else
#define T_ALERT_LOG(fmt, args...)
#define T_KALERT_LOG(key, fmt, args...) 
#endif

//crit
#ifndef DISABLE_CRIT_LOG
#define T_CRIT_LOG(fmt, args...) \
		TLOG_DETAIL(log_lvl_crit, 0, fmt, ##args)
#define T_KCRIT_LOG(key, fmt, args...) \
		TLOG_DETAIL(log_lvl_crit, key, fmt, ##args)
#else
#define T_CRIT_LOG(fmt, args...)
#define T_KCRIT_LOG(key, fmt, args...) 
#endif

//error
#ifndef DISABLE_ERROR_LOG
#define T_ERROR_LOG(fmt, args...) \
		TLOG_DETAIL(log_lvl_error, 0, fmt, ##args)
#define T_KERROR_LOG(key, fmt, args...) \
		TLOG_DETAIL(log_lvl_error, key, fmt, ##args)
#else
#define T_ERROR_LOG(fmt, args...)
#define T_KERROR_LOG(key, fmt, args...) 
#endif

//warn
#ifndef DISABLE_WARN_LOG
#define T_WARN_LOG(fmt, args...) \
		TLOG(log_lvl_warning, 0, fmt, ##args)
#define T_KWARN_LOG(key, fmt, args...) \
		TLOG(log_lvl_warning, key, fmt, ##args)
#else
#define T_WARN_LOG(fmt, args...)
#define T_KWARN_LOG(key, fmt, args...) 
#endif

//noti
#ifndef DISABLE_NOTI_LOG
#define T_NOTI_LOG(fmt, args...) \
		TLOG(log_lvl_notice, 0, fmt, ##args)
#define T_KNOTI_LOG(key, fmt, args...) \
		TLOG(log_lvl_notice, key, fmt, ##args)
#else
#define T_NOTI_LOG(fmt, args...)
#define T_KNOTI_LOG(key, fmt, args...) 
#endif

//info
#ifndef DISABLE_INFO_LOG
#define T_INFO_LOG(fmt, args...) \
		TLOG(log_lvl_info, 0, fmt, ##args)
#define T_KINFO_LOG(key, fmt, args...) \
		TLOG(log_lvl_info, key, fmt, ##args)
#else
#define T_INFO_LOG(fmt, args...)
#define T_KINFO_LOG(key, fmt, args...) 
#endif

//debug
#ifndef DISABLE_DEBUG_LOG
#define T_DEBUG_LOG(fmt, args...) \
		TLOG(log_lvl_debug, 0, fmt, ##args)
#define T_KDEBUG_LOG(key, fmt, args...) \
		TLOG(log_lvl_debug, key, fmt, ##args)
#else
#define T_DEBUG_LOG(fmt, args...)
#define T_KDEBUG_LOG(key, fmt, args...) 
#endif

//trace
#ifdef ENABLE_TRACE_LOG
#define T_TRACE_LOG(fmt, args...) \
		TLOG_DETAIL(log_lvl_trace, 0, fmt, ##args)
#define T_KTRACE_LOG(key, fmt, args...) \
		TLOG_DETAIL(log_lvl_trace, key, fmt, ##args)
#else
#define T_TRACE_LOG(fmt, args...)
#define T_KTRACE_LOG(key, fmt, args...) 
#endif


#endif//LOG_THREAD_HPP_
