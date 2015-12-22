/*
 * =====================================================================================
 *
 *  @file  wheel_timer.cpp 
 *
 *  @brief  基于内核的时间轮定时器
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  kings, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#include "wheel_timer.h"
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "common_def.hpp"


typedef void (*timer_exec_func)(void *ud, void *arg);

#define LOCK(q) while (__sync_lock_test_and_set(&(q)->lock,1)) {}
#define UNLOCK(q) __sync_lock_release(&(q)->lock);


#define TIME_NEAR_SHIFT 8
#define TIME_LEVEL_SHIFT 6
#define TIME_NEAR (1 << TIME_NEAR_SHIFT)
#define TIME_LEVEL (1 << TIME_LEVEL_SHIFT)
#define TIME_NEAR_MASK (TIME_NEAR - 1)
#define TIME_LEVEL_MASK (TIME_LEVEL - 1)


/*
struct timer_event {
	uint32_t handle;
	int session;
};*/
struct timer_event {
	callback_func_t func;
	void *owner;
	void *data;
};

struct timer_node {
	struct timer_node *next;
	uint32_t expire;
};

struct link_list {
	struct timer_node head;
	struct timer_node *tail;
};

struct timer {
	struct link_list near[TIME_NEAR];
	struct link_list t[4][TIME_LEVEL];
	int lock;
	uint32_t time;			//轮子转动次数
	uint32_t current;		//启动时间-10ms数 + 运行多少个10ms
	uint32_t starttime;		//启动时间-秒数
	uint64_t current_point; //定时器当前时间=>多少个10ms(定时器最小刻度)
	uint64_t origin_point;  //定时器启动时的时间=>多少个10ms(定时器最小刻度)
};


static struct timer * TI = NULL;

static inline struct timer_node *
link_clear(struct link_list *list)
{
	struct timer_node *ret = list->head.next;
	list->head.next = 0;
	list->tail = &(list->head);

	return ret;
}

static inline void
link(struct link_list *list, struct timer_node *node)
{
	list->tail->next = node;
	list->tail = node;
	node->next = 0;
}

static void
add_node(struct timer *T, struct timer_node *node)
{
	uint32_t time = node->expire;
	uint32_t current_time = T->time;
	if ((time | TIME_NEAR_MASK) == (current_time | TIME_NEAR_MASK)) {//与当前时间差值小于256个最小刻度,则加入第一个时间轮
		link(&(T->near[time & TIME_NEAR_MASK]), node);
	} else {//此时每6位向前查找，然后插入对应的时间轮
		int i;
		uint32_t mask = TIME_NEAR << TIME_LEVEL_SHIFT;
		for (i = 0; i < 3; i++) {
			if ((time | (mask - 1)) == (current_time | (mask - 1))) {
				break;
			}
			mask <<= TIME_LEVEL_SHIFT;
		}
		
		link(&(T->t[i][(time >> (TIME_NEAR_SHIFT + i * TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK]), node);
	}
}

static void
timer_add(struct timer *T, void *arg, size_t sz, int time)
{
	struct timer_node * node = (struct timer_node*)malloc(sizeof(*node) + sz);
	memcpy(node + 1, arg, sz);//把事件挂在node后面
	LOCK(T);
		node->expire = time + T->time;
		add_node(T, node);
		//DEBUG_LOG("--------exipre=%d", node->expire);
	UNLOCK(T);

}

static void
move_list(struct timer *T, int level, int idx)
{
	struct timer_node *current = link_clear(&T->t[level][idx]);
	while (current) {
		struct timer_node *temp = current->next;
		add_node(T, current);
		current = temp;
	}
}

/* @brief 轮子转动
 */
static void
timer_shift(struct timer *T)
{
	LOCK(T);
	int mask = TIME_NEAR;
	uint32_t ct = ++T->time;
	if (ct == 0) {
		move_list(T, 3, 0);
	} else {
		uint32_t time = ct >> TIME_NEAR_SHIFT;
		int i = 0;
		//根据定时器运行时间，依次转动每个轮子，比如:运行了256个最小刻度时间，则第一个轮子转完一圈，则二个轮子当前插槽的节点需要移至第一个轮子
		while ((ct & (mask - 1)) == 0) {			
			int idx = time & TIME_LEVEL_MASK;
			if (idx != 0) {//后面的轮子插槽0其实就是指向前面的轮子，idx等于0说明是255的整数倍，前面的轮子255/63个插槽已经足够放，所以后面的轮子插槽0不可能有节点
				move_list(T, i, idx);
				break;
			}
			mask <<= TIME_LEVEL_SHIFT;
			time >>= TIME_LEVEL_SHIFT;
			++i;
		}
	}
	UNLOCK(T);
}

static inline void
dispatch_list(struct timer_node *current)
{
	do {//依次处理该插槽每个节点
		struct timer_event *event = (struct timer_event*)(current + 1);
		//do something, push event
		//...
		//call the callback func
		//DEBUG_LOG("====Frankie=== Call Timer Function\t[time=%u, current_point=%ld]", TI->time, TI->current_point);
		event->func(event->owner, event->data);
		//DEBUG_LOG("===Frankie==handle=%u, session=%d, time=%u, current_point=%ld\n", event->handle, event->session, TI->time, TI->current_point);
		//do something end
		
		struct timer_node *temp = current;
		current = current->next;
		if (temp) {//处理完释放该节点
			free(temp);
			temp = 0;
		}

	} while (current);
}

static inline void
timer_execute(struct timer *T)
{
	LOCK(T);
	int idx = T->time & TIME_NEAR_MASK;
	while (T->near[idx].head.next) {
		struct timer_node *current = link_clear(&(T->near[idx]));
		UNLOCK(T);
		dispatch_list(current);
		LOCK(T);
	}

	UNLOCK(T);
}

static void
timer_update(struct timer *T)
{
	timer_execute(T);

	timer_shift(T);

	timer_execute(T);
}

static struct timer *
timer_create_timer()
{
	struct timer *r = (struct timer *)malloc(sizeof(*r));
	memset(r, 0, sizeof(*r));

	int i, j;

	for (i = 0; i < TIME_NEAR; i++) {
		link_clear(&(r->near[i]));
	}

	for (i = 0; i < 4; i++) {
		for (j = 0; j < TIME_LEVEL; j++) {
			link_clear(&(r->t[i][j]));
		}
	}

	r->lock = 0;
	r->current = 0;

	return r;
}

int
set_timeout(callback_func_t handle, void *owner, void *data, int64_t time)
{
	time = (int)(time / 10);//covert to 10ms per
	if (time == 0) {
		//do something
	} else {
		struct timer_event event;
		event.func = handle;
		event.owner = owner;
		event.data = data;
		timer_add(TI, &event, sizeof(event), time);
	}
	return 0;
}

//最小精度10ms
static void
systime(uint32_t *sec, uint32_t *cs)
{
	/*
	struct timeval tv;
	gettimeofday(&tv, NULL);
	*sec = tv.tv_sec;
	*cs = tv.tv_usec / 10000;
	*/
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	*sec = (uint32_t)tp.tv_sec;
	*cs = (uint32_t)(tp.tv_nsec / 10000000);
}

static uint64_t
gettime()
{
	uint64_t t;
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	t = (uint64_t)tp.tv_sec * 100;
	t += tp.tv_nsec / 10000000;

	return t;
}

void wheel_update_timer()
{
	uint64_t cp = gettime();
	if (cp < TI->current_point) {
		DEBUG_LOG("time diff error: change from %lu to %lu.", cp, TI->current_point);
		TI->current_point = cp;
	} else if (cp != TI->current_point) {
		uint32_t diff = (uint32_t)(cp - TI->current_point);
		TI->current_point = cp;

		uint32_t oc = TI->current;
		TI->current += diff;
		//溢出了，超过4个字节最大数
		if (TI->current < oc) {//diff > 497 days, rewind
			TI->starttime += 0xffffffff / 100;
		}
		uint32_t i;
		for (i = 0; i < diff; i++) {
			timer_update(TI);
		}
	}
}

uint32_t
wheel_gettime_fixsec()
{
	return TI->starttime;
}

uint32_t
wheel_gettime()
{
	return TI->current;
}

void 
wheel_init_timer()
{
	TI = timer_create_timer();
	systime(&TI->starttime, &TI->current);
	uint64_t point = gettime();
	TI->current_point = point;
	TI->origin_point = point;
}

#if 0
int main()
{
	wheel_init_timer();
	printf("starttime=%u, current=%u, current_point=%llu, time=%u\n", TI->starttime, TI->current, TI->current_point, TI->time);
	set_timeout(1, 10, 10);
	set_timeout(2, 50, 40);
	while (1) {
		wheel_update_timer();
		usleep(10);
	}
	return 0;
}
#endif
