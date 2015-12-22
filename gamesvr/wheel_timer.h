#ifndef WHEEL_TIMER_H
#define WHEEL_TIME_H

#include <stdint.h>

typedef int (*callback_func_t)(void*, void*);

int set_timeout(callback_func_t handle, void *owner, void *data, int64_t time);
void update_timer();
void timer_init();
uint32_t gettime_fixsec();
void wheel_init_timer();
void wheel_update_timer();

#endif
