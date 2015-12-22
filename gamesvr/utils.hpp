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

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include "common_def.hpp"

/* @brief 公共操作管理类
 */
class Utils {
public:
	Utils();
	~Utils();

	/*! 用户相关操作 */
	bool is_valid_uid(uint32_t user_id);

	/*! 时间相关操作 */
	uint32_t get_date(time_t t);
	uint32_t get_day(time_t t); 
	uint32_t get_hour(time_t t); 
	uint32_t get_min(time_t t); 
	uint32_t get_week_day(time_t t); 
	uint32_t get_year_day(time_t t);
	uint32_t get_year_month(time_t t); 
	uint32_t mk_struct_tm(time_t t, struct tm &tm_tmp);
	time_t mk_zero_tm(struct tm tm_cur);
	time_t mk_next_day_zero_tm();
	time_t mk_next_ndays_zero_tm(uint32_t nday);
	time_t mk_sunday_night_tm();
	time_t mk_tm(int year, int month, int day, int hour=0, int min=0, int sec=0);
	time_t mk_date_to_tm(uint32_t date);
	time_t get_month_last_day_tm(time_t now_tm);
	time_t get_today_zero_tm();
	time_t get_next_day_zero_tm(time_t t);
	time_t get_cur_hour_whole_tm();
	bool is_weekend();
	bool is_month_last_day();
	bool is_date_today(uint32_t tm);



	/*! 其他操作 */
	char* int2ipstr(uint32_t ip, char *buf);
	uint32_t get_binary_one_cnt(uint64_t i);
	uint32_t get_vec_same_cnt(std::vector<uint32_t> vec1, std::vector<uint32_t> vec2);

	/*! 坐标操作 */
	int get_move_point(uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y, uint32_t x1, uint32_t y1, uint32_t len, uint32_t& x2, uint32_t& y2);
	uint32_t get_two_point_distance(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2);
	
	template<typename T>
	inline uint32_t get_array_len(T& array) { return (sizeof(array) / sizeof(array[0])); }

	float frand();
	bool get_rand_vec(const std::vector<uint32_t>& vec1, uint32_t num, std::vector<uint32_t>& vec2);
};

extern Utils utils_mgr; 

int luaopen_utils(lua_State *L);

#endif
