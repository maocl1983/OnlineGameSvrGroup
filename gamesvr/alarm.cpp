/**
 * =====================================================================================
 *
 * @file  alarm.cpp
 *
 * @brief 处理告警信息
 *
 * compiler  GCC4.1.2
 * platform  Linux
 *
 * =====================================================================================
 */


extern "C"{
#include <string.h>
#include <libcommon/conf_parser/config.h>
#include <libcommon/log.h>
#include <libcommon/time/timer.h> 
}
#include <asyn_serv/net_if.hpp> 


#include "alarm.hpp"
#include "common_def.hpp"

int alarm_fd = -1;


/* @brief 向alarm发送包
 * @param msg 发送的包体的内容，不包括包头
 */
int send_msg_to_alarm(const char *msg)
{
	if (alarm_fd == -1) {
		connect_to_alarm();
	}

	if (alarm_fd == -1) {
		ERROR_LOG("send to ALARM failed: [FD = %d]", alarm_fd );
		return 0;
	}

	alarm_proto_head_t pkg;
	pkg.len = sizeof(alarm_proto_head_t) + strlen(msg) + 1;
	pkg.game_id = 1;
	memcpy(pkg.body, msg, strlen(msg)+1);

	DEBUG_LOG("send to alarm:[%s]", msg);

	return net_send_msg(alarm_fd, reinterpret_cast<char *>(&pkg), NULL);
}

/* @brief 首次连接alarm，并向其发送gamesvr信息
 */
int connect_to_alarm()
{
	alarm_fd = connect_to_service(config_get_strval("alarm"), 1, 65535, 1);
	if (alarm_fd != -1) {
	} else {
		const char *alarm_ip = config_get_strval("alarm_ip");
		if (alarm_ip) {
			alarm_fd = connect_to_svr(config_get_strval("alarm_ip"), config_get_intval("alarm_port", 0), 65536, 1);
		}
	}

	KDEBUG_LOG(0,"CONNECT ALARM\t[fd=%d]",alarm_fd);
	return 0;
}

/* @brief 添加定时器，3秒重连alarm
 */
int connect_to_alarm_timely(void *owner, void *data)
{
	if (alarm_fd == -1) {
		connect_to_alarm();
	}
	add_timer_event(0, connect_to_alarm_timely, NULL, NULL, 2000);

	return 0;
}

