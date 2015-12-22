/**
 *============================================================
 *  @file      utils.hpp
 *
 *  @brief	   通用接口 
 * 
 *  compiler   gcc4.4.7
 *
 *  platform   Linux
 *
 *  copyright:  kings, Inc. ShangHai CN. All rights reserved.
 *
 *============================================================
 */


#include "utils.hpp"
#include <math.h>

using namespace std;
using namespace project;

Utils utils_mgr; 

/************************************************************************/
/*                          Utils class                                 */ 
/************************************************************************/

Utils::Utils()
{

}

Utils::~Utils()
{

}

/* @brief 判断是否是合法的用户帐号 
 */
bool
Utils::is_valid_uid(uint32_t user_id)
{
	if (user_id <100000 && user_id > 10000) {
		return true;
	}

	return false;
}

/* @brief 获取当前日期，精确到日 
 */
uint32_t
Utils::get_date(time_t t)
{   
	struct tm tm_tmp;
	localtime_r(&t, &tm_tmp);
	return (tm_tmp.tm_year+1900)*10000+(tm_tmp.tm_mon+1)*100+tm_tmp.tm_mday;
} 

/* @brief 获取天数
 */
uint32_t
Utils::get_day(time_t t)
{
	struct tm tm_tmp;
	localtime_r(&t, &tm_tmp);
	return tm_tmp.tm_mday;
}

/* @brief 获取小时 
 */
uint32_t
Utils::get_hour(time_t t)
{
	struct tm tm_tmp;
	localtime_r(&t, &tm_tmp);
	return tm_tmp.tm_hour;
}

/* @brief 获取分钟
 */
uint32_t
Utils::get_min(time_t t)
{
	struct tm tm_tmp;
	localtime_r(&t, &tm_tmp);
	return tm_tmp.tm_min;
}

/* @brief 获取当前为1年之中第几天
 */
uint32_t
Utils::get_year_day(time_t t)
{
	struct tm tm_tmp;
	localtime_r(&t, &tm_tmp);
	return tm_tmp.tm_yday;
}

/* @brief 获取当前星期 
 */
uint32_t
Utils::get_week_day(time_t t)
{   
	struct tm tm_tmp;
	localtime_r(&t, &tm_tmp);
	return tm_tmp.tm_wday;
}

/* @brief 获取当前日期，精确到月 
 */
uint32_t
Utils::get_year_month(time_t t)
{       
	struct tm tm_tmp;
	localtime_r(&t, &tm_tmp);
	return (tm_tmp.tm_year+1900)*100+tm_tmp.tm_mon+1;
}

/* @brief 将日期转化为时间戳
 */
time_t 
Utils::mk_date_to_tm(uint32_t date)
{
	struct tm tm_tmp;
	memset(&tm_tmp, 0, sizeof(tm));
	tm_tmp.tm_year = date / 10000 - 1900;
	tm_tmp.tm_mon = (date % 10000) / 100 - 1;
	tm_tmp.tm_mday = date % 100;

	return mktime(&tm_tmp);
}

/* @brief 获取本月最后一天0点时间戳
 */
time_t
Utils::get_month_last_day_tm(time_t now_tm)
{
	struct tm tm_tmp;
	localtime_r(&now_tm, &tm_tmp);
	tm_tmp.tm_mon += 1;
	if (tm_tmp.tm_mon > 11) {
		tm_tmp.tm_year += 1;
		tm_tmp.tm_mon = 0;
	}
	tm_tmp.tm_mday = 1;
	tm_tmp.tm_hour = 0;
	tm_tmp.tm_min = 0;
	tm_tmp.tm_sec = 0;

	return mktime(&tm_tmp) - 24 * 60 * 60;
}

/* @brief 获取当前零点时间
 */
time_t
Utils::mk_zero_tm(struct tm tm_cur)
{
	tm_cur.tm_hour = 0;
	tm_cur.tm_min = 0;
	tm_cur.tm_sec = 0;

	return mktime(&tm_cur);
}

/* @brief 获取当前小时整点时间
 */
time_t
Utils::get_cur_hour_whole_tm()
{
	struct tm tm_cur;
	time_t now_sec = time(0);
	localtime_r(&now_sec, &tm_cur);
	tm_cur.tm_min = 0;
	tm_cur.tm_sec = 0;

	return mktime(&tm_cur);
}

/* @brief 获取当天零点时间
 */
time_t
Utils::get_today_zero_tm()
{
	struct tm tm_cur;
	time_t now_sec = time(0);
	localtime_r(&now_sec, &tm_cur);
	tm_cur.tm_hour = 0;
	tm_cur.tm_min = 0;
	tm_cur.tm_sec = 0;

	return mktime(&tm_cur);
}

/* @brief 获取指定时间第二天零点时间
 */
time_t
Utils::get_next_day_zero_tm(time_t t)
{
	struct tm tm_tmp;
	localtime_r(&t, &tm_tmp);
	
	tm_tmp.tm_hour = 23;
	tm_tmp.tm_min = 59;
	tm_tmp.tm_sec = 59;

	return mktime(&tm_tmp) + 1;

}

/* @brief 获取次日的零点时间
 */
time_t
Utils::mk_next_day_zero_tm()
{
	struct tm now = *get_now_tm();
	now.tm_hour = 23;
	now.tm_min = 59;
	now.tm_sec = 59;

	return mktime(&now);
}

/* @brief 获取本周日23:59:59的时间
 */
time_t
Utils::mk_sunday_night_tm()
{
	struct tm now = *get_now_tm();
	int wday = (now.tm_wday==0) ? 7 : now.tm_wday;
	int days = 7 - wday;
	now.tm_hour = 23;
	now.tm_min = 59;
	now.tm_sec = 59;

	uint32_t tonight_tm = mktime(&now);
	return tonight_tm + days * 86400;
}

/* @brief 获取N天后的零点时间
 */
time_t
Utils::mk_next_ndays_zero_tm(uint32_t nday)
{
	time_t t = get_now_tv()->tv_sec + nday * 86400;
	struct tm tm_tmp;
	localtime_r(&t, &tm_tmp);
	return mk_zero_tm(tm_tmp);
}

time_t
Utils::mk_tm(int year, int month, int day, int hour, int min, int sec)
{
	struct tm set_tm;
	set_tm.tm_year = year - 1900;
	set_tm.tm_mon= month - 1;
	set_tm.tm_mday = day;
	set_tm.tm_hour = hour;
	set_tm.tm_min= min;
	set_tm.tm_sec= sec;
	return mktime(&set_tm);
}

/* @brief 获取当前时间的结构体
 */
uint32_t
Utils::mk_struct_tm(time_t t, struct tm& tm_tmp)
{
	localtime_r(&t, &tm_tmp);
	return 0;
}


/* @brief 判断是否是今天
 */
bool
Utils::is_date_today(uint32_t tm)
{
	uint32_t zero_tm = get_today_zero_tm();
	if (tm > zero_tm && (tm - zero_tm) < 86400) {
		return true;
	}
	return false;
}

/* @brief 判断是否月尾
 */
bool
Utils::is_month_last_day()
{
	uint32_t last_day[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	struct tm tm_tmp;
	time_t t = time(0);
	localtime_r(&t, &tm_tmp);
	uint32_t year = tm_tmp.tm_year + 1900;
	uint32_t mon = tm_tmp.tm_mon + 1;
	uint32_t day = tm_tmp.tm_mday;
	if (mon == 2) {
		if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
			return (day == 29);
		} else {
			return (day == 28);
		}	
	} else {
		return (day == last_day[mon - 1]);
	}
}

/* @brief 是否是周末 
 */
bool
Utils::is_weekend()
{
	uint32_t wday = get_week_day(time(0));
	if (wday == 5 || wday == 6 || wday == 0) {
		return true;
	}

	return false;
}


/* @brief 获取二进制数中1的个数 查表法
 */
uint32_t
Utils::get_binary_one_cnt(uint64_t i)
{
	const char *p = "\0\1\1\2\1\2\2\3\1\2\2\3\2\3\3\4";
	uint32_t cnt = 0;
	while (i) {
		cnt += p[i&0xf];
		i >>= 4;
	}   

	return cnt;
}

/* @brief 获取两个vector中相同元素的个数，遇到相同的就双双删除类似连连看，所以不传引用 
 */
uint32_t
Utils::get_vec_same_cnt(vector<uint32_t> vec1, vector<uint32_t> vec2)
{
	uint32_t same_cnt = 0;
	vector<uint32_t>::iterator it1 = vec1.begin(); 
	for (; it1 != vec1.end();) {
		uint32_t flag = 0;
		uint32_t value1 = *it1;
		vector<uint32_t>::iterator it2 = vec2.begin(); 
		for (; it2 != vec2.end(); it2++) {
			if (value1 == *it2) {
				same_cnt++;
				vec2.erase(it2);
				it1 = vec1.erase(it1);
				flag = 1;
				break;
			}
		}
		if (!flag) {
			it1++;
		}
	}

	return same_cnt;
}


/* @brief 获取0-1的随机数
 */
float
Utils::frand()
{
	return rand() % RAND_MAX;
}

/* @brief 从vector1中随机取num个元素放到vector2中
 */
bool
Utils::get_rand_vec(const vector<uint32_t> &vec1, uint32_t num, vector<uint32_t> &vec2)
{
	uint32_t size = vec1.size();
	if (size < num || num == 0) {
		return false;
	}	
	vec2.clear();
	vec2.reserve(num);
	for (uint32_t i = 0, n = size; i < n && num != 0; i++) {
		float f_rand = frand();
		if (f_rand < (float)(num) / size) {
			vec2.push_back(vec1[i]);
			num--;
		}
		size--;
	}
	return true;
}

/* @brief 已知一条路径上一点，根据移动距离计算另一点的坐标
 * @param start_x, start_y:路径起始点坐标
 * @param end_x, end_y:路径结束点坐标
 * @param x1, y1:起始点坐标
 * @param len:移动距离
 * @param x2, y2:到达点坐标
 */
int 
Utils::get_move_point(uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t x1, uint32_t y1, uint32_t len, uint32_t &x2, uint32_t &y2)
{
	x2 = y2 = 0;
	uint32_t max_len = get_two_point_distance(x1, x2, end_x, end_y);
	if (len > max_len) {
		len = max_len;
	}
	int32_t chg_x = end_x - start_x;
	int32_t chg_y = end_y - start_y;
	float a = atan2(chg_y, chg_x);
	int32_t x = (int32_t)(x1 + len * cosf(a));
	int32_t y = (int32_t)(y1 + len * sinf(a));
	if ((chg_x > 0 && x > (int32_t)end_x) || (chg_x < 0 && x < (int32_t)end_x)) {
		x = end_x;
	}
	if ((chg_y > 0 && y > (int32_t)end_y) || (chg_y < 0 && y < (int32_t)end_y)) {
		y = end_y;
	}
	x2 = x;
	y2 = y;

	return 0;
}

/* @brief 计算两点之间距离
 * @param x1,y1:起始点坐标
 * @param x2,y2:终止点坐标
 */
uint32_t
Utils::get_two_point_distance(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
{
	int32_t chg_x = x2 - x1;
	int32_t chg_y = y2 - y1;
	uint32_t distance = (uint32_t)sqrt(powf(chg_x, 2) + powf(chg_y, 2));
	return distance;
}

/********************************************************************************/
/*								Lua Interface									*/
/********************************************************************************/
static int
lua_get_date(lua_State *L)
{   
	time_t t = luaL_checkinteger(L, 1);

	uint32_t date = utils_mgr.get_date(t);

	lua_pushinteger(L, date);

	return 1;
} 

static int
lua_get_day(lua_State *L)
{
	time_t t = luaL_checkinteger(L, 1);
	uint32_t day = utils_mgr.get_day(t);
	lua_pushinteger(L, day);

	return 1;
}

/* @brief 获取小时 
 */
static int
lua_get_hour(lua_State *L)
{
	time_t t = luaL_checkinteger(L, 1);
	uint32_t hour = utils_mgr.get_hour(t);
	lua_pushinteger(L, hour);

	return 1;
}

/* @brief 获取分钟
 */
static int
lua_get_min(lua_State *L)
{
	time_t t = luaL_checkinteger(L, 1);
	uint32_t min = utils_mgr.get_min(t);
	lua_pushinteger(L, min);

	return 1;
}

/* @brief 获取当前为1年之中第几天
 */
static int
lua_get_year_day(lua_State *L)
{
	time_t t = luaL_checkinteger(L, 1);
	uint32_t yday = utils_mgr.get_year_day(t);
	lua_pushinteger(L, yday);

	return 1;
}

/* @brief 获取当前星期 
 */
static int
lua_get_week_day(lua_State *L)
{   
	time_t t = luaL_checkinteger(L, 1);
	uint32_t wday = utils_mgr.get_week_day(t);
	lua_pushinteger(L, wday);

	return 1;
}

/* @brief 获取当前日期，精确到月 
 */
static int
lua_get_year_month(lua_State *L)
{       
	time_t t = luaL_checkinteger(L, 1);
	uint32_t m = utils_mgr.get_year_month(t);
	lua_pushinteger(L, m);

	return 1;
}

/* @brief 将日期转化为时间戳
 */
static int
lua_mk_date_to_tm(lua_State *L)
{
	uint32_t date = luaL_checkinteger(L, 1);
	uint32_t t = utils_mgr.mk_date_to_tm(date);
	lua_pushinteger(L, t);

	return 1;
}

/* @brief 获取本月最后一天0点时间戳
 */
static int
lua_get_month_last_day_tm(lua_State *L)
{
	uint32_t now_tm = luaL_checkinteger(L, 1);
	uint32_t t = utils_mgr.get_month_last_day_tm(now_tm);
	lua_pushinteger(L, t);

	return 1;
}

/* @brief 获取当前零点时间
 */
static int
lua_mk_zero_tm(lua_State *L)
{
	struct tm *p = (struct tm*)lua_touserdata(L, 1);
	uint32_t t = utils_mgr.mk_zero_tm(*p);
	lua_pushinteger(L, t);

	return 1;
}

/* @brief 获取当天零点时间
 */
static int
lua_get_today_zero_tm(lua_State *L)
{
	uint32_t t = utils_mgr.get_today_zero_tm();
	lua_pushinteger(L, t);

	return 1;
}

/* @brief 获取次日的零点时间
 */
static int
lua_mk_next_day_zero_tm(lua_State *L)
{
	uint32_t t = utils_mgr.mk_next_day_zero_tm();
	lua_pushinteger(L, t);

	return 1;
}

static int
lua_test_bit_on(lua_State *L)
{
	uint32_t stat = luaL_checkinteger(L, 1);
	uint32_t bit = luaL_checkinteger(L, 2);
	if (bit <= 32) {
		bool flag = test_bit_on(stat, bit);
		lua_pushboolean(L, flag);

		return 1;
	}

	return 0;
}

static int
lua_set_bit_on(lua_State *L)
{
	uint32_t stat = luaL_checkinteger(L, 1);
	uint32_t bit = luaL_checkinteger(L, 2);
	if (bit <= 32) {
		stat = set_bit_on(stat, bit);
		lua_pushinteger(L, stat);

		return 1;
	}

	return 0;
}

int
luaopen_utils(lua_State *L)
{
	luaL_checkversion(L);

	luaL_Reg l[] = {
		{"get_date", lua_get_date},
		{"get_day", lua_get_day},
		{"get_hour", lua_get_hour},
		{"get_min", lua_get_min},
		{"get_year_day", lua_get_year_day},
		{"get_week_day", lua_get_week_day},
		{"get_year_month", lua_get_year_month},
		{"mk_date_to_tm", lua_mk_date_to_tm},
		{"get_month_last_day_tm", lua_get_month_last_day_tm},
		{"mk_zero_tm", lua_mk_zero_tm},
		{"get_today_zero_tm", lua_get_today_zero_tm},
		{"mk_next_day_zero_tm", lua_mk_next_day_zero_tm},
		{"test_bit_on", lua_test_bit_on},
		{"set_bit_on", lua_set_bit_on},
		{NULL, NULL}
	};

	luaL_newlib(L, l);

	return 1;
}

