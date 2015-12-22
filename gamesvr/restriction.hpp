/**
  * ============================================================
  *
  *  @file      restriction.hpp
  *
  *  @brief     处理限制的相关信息
  * 
  *  compiler   gcc4.1.2
  *
  *  platform   Linux
  *
  *  copyright:  kings, Inc. ShangHai CN. All rights reserved.
  *
  * ============================================================
  */

#ifndef RESTRICTION_HPP_
#define RESTRICTION_HPP_

#include "common_def.hpp"

class db_res_info_t;
class Player;

/* @brief 限制类型 
 */
enum res_type_t {
	/*******************************! 永久不清零 1 ~ 9999 **********************************/
	forever_name						= 0,
	forever_role_login_init_flag		= 1,		/*! 首次进入游戏初始化状态 */
	forever_complete_instance_id		= 2,		/*! 完成副本的ID */
	forever_divine_id 					= 3,		/*! 当前占星ID */
	forever_ten_even_draw_free_tm		= 4,		/*! 十连抽元宝抽奖时间 */
	forever_ten_even_draw_first			= 5,		/*! 十连抽元宝首次单抽标志 */
	forever_horse_train_out_flag		= 6,		/*! 战马培养突破标志，按位存，每10级一次 */
	forever_arena_history_ranking		= 7,		/*! 竞技场历史最高排名 */
	forever_redis_login_init_flag		= 8,		/*! 首次进入游戏redis初始化状态 */
	forever_treasure_risk_protection_tm = 9,		/*! 夺宝保护时间 */
	forever_shop_1_free_refresh_tms		= 10,		/*! 商店1免费刷新次数*/
	forever_shop_2_free_refresh_tms		= 11,		/*! 商店2免费刷新次数*/
	forever_shop_3_free_refresh_tms		= 12,		/*! 商店3免费刷新次数*/
	forever_new_player_lead_id			= 13,		/*! 新手引导完成ID */
	forever_ten_even_draw_golds_first	= 14,		/*! 十连抽金币首次单抽 */
	forever_ten_even_draw_diamond_first	= 15,		/*! 十连抽钻石首次单抽 */
	forever_cur_battle_instance_id		= 16,		/*! 当前挑战副本ID */

	forever_main_hero_skill_1			= 17,		/*! 主公技能1 */
	forever_main_hero_skill_2			= 18,		/*! 主公技能2 */
	forever_main_hero_skill_3			= 19,		/*! 主公技能3 */

	/*! 主公技能等级 21-40 */
	forever_main_hero_skill_1_lv		= 21,		/*! 主公技能1等级 */
	forever_main_hero_skill_2_lv		= 22,		/*! 主公技能2等级 */
	forever_main_hero_skill_3_lv		= 23,		/*! 主公技能3等级 */
	forever_main_hero_skill_4_lv		= 24,		/*! 主公技能4等级 */
	forever_main_hero_skill_5_lv		= 25,		/*! 主公技能5等级 */
	forever_main_hero_skill_6_lv		= 26,		/*! 主公技能6等级 */
	forever_main_hero_skill_7_lv		= 27,		/*! 主公技能7等级 */
	forever_main_hero_skill_8_lv		= 28,		/*! 主公技能8等级 */
	forever_main_hero_skill_9_lv		= 29,		/*! 主公技能9等级 */
	forever_main_hero_skill_10_lv		= 30,		/*! 主公技能10等级 */
	forever_main_hero_skill_11_lv		= 31,		/*! 主公技能11等级 */
	forever_main_hero_skill_12_lv		= 32,		/*! 主公技能12等级 */
	/*! 主公技能等级 21-40 */

	forever_player_login_day				= 41,		/*! 玩家登陆天数 */
	forever_new_player_checkin_reward_stat	= 42,		/*! 新手签到状态 */
	forever_total_cost_diamond				= 43,		/*! 消费总钻石数 */ 
	forever_level_gift_stat					= 44,		/*! 等级礼包领取状态 按位存*/
	forever_diamond_gift_stat				= 45,		/*! 元宝礼包领取状态 按位存*/
	forever_ten_even_draw_diamond_tms		= 46,		/*! 十连抽钻石抽次数 */

	forever_trial_tower_finish_floor		= 47,		/*! 试练塔当前通关层数 */
	forever_trial_tower_sweep_start_floor	= 48,		/*! 试练塔扫荡前起始通关层数 */
	forever_trial_tower_history_max_floor	= 49,		/*! 试练塔历史最高通关层数 */
	forever_trial_tower_life				= 50,		/*! 试练塔当前生命值 */
	forever_trial_tower_sweep_tm			= 51,		/*! 试练塔扫荡起始时间 */

	/*******************************! 永久不清零 1 ~ 9999 **********************************/




	/******************************! 每日清零 10000 ~ 19999 ********************************/
	daily_checkin_stat						= 10001,	/*! 每日签到状态 */
	daily_ten_even_draw_free_tms			= 10002,	/*! 十连抽铜钱免费使用次数 */
	daily_ten_even_draw_free_tm				= 10003,	/*! 十连抽铜钱抽奖时间 */
	daily_horse_train_golds_tms				= 10004,	/*! 战马每日金币培养次数 */
	daily_horse_train_diamond_tms			= 10005,	/*! 战马每日钻石培养次数 */
	daily_arena_challenge_tms				= 10006,	/*! 竞技场每日免费挑战次数 */
	daily_arena_challenge_tm				= 10007,	/*! 竞技场挑战时间 */

	/*! 过关斩将 */
	daily_kill_guard_cur_idx				= 10008,	/*! 过关斩将当前idx */
	daily_kill_guard_lose_tms				= 10009,	/*! 过关斩将当日失败次数 */
	daily_kill_guard_hero_1					= 10010,	/*! 过关斩将第1关杀死Boss武将ID */
	daily_kill_guard_hero_2					= 10011,	/*! 过关斩将第2关杀死Boss武将ID */
	daily_kill_guard_hero_3					= 10012,	/*! 过关斩将第3关杀死Boss武将ID */
	daily_kill_guard_hero_4					= 10013,	/*! 过关斩将第4关杀死Boss武将ID */
	daily_kill_guard_hero_5					= 10014,	/*! 过关斩将第5关杀死Boss武将ID */
	daily_kill_guard_hero_6					= 10015,	/*! 过关斩将第6关杀死Boss武将ID */
	daily_kill_guard_hero_7					= 10016,	/*! 过关斩将第7关杀死Boss武将ID */
	daily_kill_guard_hero_8					= 10017,	/*! 过关斩将第8关杀死Boss武将ID */
	daily_kill_guard_hero_9					= 10018,	/*! 过关斩将第9关杀死Boss武将ID */
	daily_kill_guard_hero_10				= 10019,	/*! 过关斩将第10关杀死Boss武将ID */

	/*! 通用玩法已完成次数 预留 10020-10039 */
	daily_trial_fight_tms					= 10020,	/*! 试炼玩法当前已完成tms */
	daily_soldier_fight_tms					= 10021,	/*! 兵战玩法当前已完成tms */
	daily_defend_fight_tms					= 10022,	/*! 守城战玩法当前已完成tms */
	daily_golds_fight_tms					= 10023,	/*! 金币玩法当前已完成tms */
	daily_riding_alone_fight_tms			= 10024,	/*! 千里走单骑玩法当前已完成tms */
	/*! 通用玩法已完成次数 预留 10020-10039 */

	daily_energy_buy_tms					= 10040,	/*! 每日体力购买次数 */
	daily_endurance_buy_tms					= 10041,	/*! 每日耐力购买次数 */
	daily_adventure_complete_tms			= 10042,	/*! 每日奇遇完成次数 */
	daily_new_player_checkin_flag			= 10043,	/*! 新手签到标志 */
	daily_energy_gift_stat					= 10044,	/*! 每日免费体力领取状态 */
	daily_skill_point_buy_tms				= 10045,	/*! 每日技能点购买次数 */
	daily_soldier_train_point_buy_tms		= 10046,	/*! 每日小兵训练点购买次数 */
	daily_vip_gift_stat						= 10047,	/*! 每日VIP特权礼包购买状态 */

	daily_yesterday_internal_affairs_cnt	= 10060,	/*! 昨日内政完成次数 */
	daily_internal_affairs_role_exp_flag	= 10061,	/*! 内政主公经验领取标志 */

	daily_trial_tower_challenge_tms			= 10062,	/*! 试练塔挑战次数 */

	/*! 通用玩法已完成次数 预留 10090-10109 */
	daily_trial_fight_last_tm				= 10090,	/*! 试炼玩法上次挑战时间 */
	daily_soldier_fight_last_tm				= 10091,	/*! 兵战玩法上次挑战时间 */
	daily_defend_fight_last_tm				= 10092,	/*! 守城战玩法上次挑战时间 */
	daily_golds_fight_last_tm				= 10093,	/*! 金币玩法上次挑战时间 */
	daily_riding_alone_fight_last_tm		= 10094,	/*! 千里走单骑玩法上次挑战时间 */
	/*! 通用玩法已完成次数 预留 10090-10109 */

	daily_kill_guard_last_tm				= 10110,	/*! 过关斩将上次挑战时间 */

	/*! 道具商城限制 预留10200-10299 */
	daily_shop_3_item_100001_buy_tms		= 10200,	/*! 道具商店道具1购买次数-武将经验道具 */
	daily_shop_3_item_102001_buy_tms		= 10201,	/*! 道具商店道具2购买次数-兵种经验道具 */
	daily_shop_3_item_120006_buy_tms		= 10202,	/*! 道具商店道具3购买次数-奇遇令 */
	daily_shop_3_item_120007_buy_tms		= 10203,	/*! 道具商店道具4购买次数-体力丹 */
	daily_shop_3_item_120008_buy_tms		= 10204,	/*! 道具商店道具5购买次数-耐力丹 */
	daily_shop_3_item_120001_buy_tms		= 10205,	/*! 道具商店道具7购买次数-陨铁 */
	/*! 道具商城限制 预留10200-10299 */

	/*! 占星限制 预留100300-10340 */
	daily_astrology_dst_star_1				= 10300,	/*! 占星目标星座1的ID */
	daily_astrology_dst_star_2				= 10301,	/*! 占星目标星座2的ID */
	daily_astrology_dst_star_3				= 10302,	/*! 占星目标星座3的ID */
	daily_astrology_dst_star_4				= 10303,	/*! 占星目标星座4的ID */
	daily_astrology_dst_star_5				= 10304,	/*! 占星目标星座5的ID */
	daily_astrology_dst_star_6				= 10305,	/*! 占星目标星座6的ID */
	daily_astrology_dst_star_7				= 10306,	/*! 占星目标星座7的ID */
	daily_astrology_dst_star_8				= 10307,	/*! 占星目标星座8的ID */
	daily_astrology_dst_star_9				= 10308,	/*! 占星目标星座9的ID */
	daily_astrology_dst_star_10				= 10309,	/*! 占星目标星座10的ID */
	daily_astrology_dst_star_11				= 10310,	/*! 占星目标星座11的ID */
	daily_astrology_dst_star_12				= 10311,	/*! 占星目标星座12的ID */
	daily_astrology_dst_star_13				= 10312,	/*! 占星目标星座13的ID */
	daily_astrology_dst_star_14				= 10313,	/*! 占星目标星座14的ID */
	daily_astrology_dst_star_15				= 10314,	/*! 占星目标星座15的ID */
	daily_astrology_dst_star_16				= 10315,	/*! 占星目标星座16的ID */

	daily_astrology_select_star_1			= 10316,	/*! 占星选择1星座ID */
	daily_astrology_select_star_2			= 10317,	/*! 占星选择2星座ID */
	daily_astrology_select_star_3			= 10318,	/*! 占星选择3星座ID */
	daily_astrology_select_star_4			= 10319,	/*! 占星选择4星座ID */
	daily_astrology_select_star_5			= 10320,	/*! 占星选择5星座ID */

	daily_astrology_light_star_stat			= 10321,	/*! 占星点亮星座状态，按位存 */
	daily_astrology_star_num				= 10322,	/*! 当前获得星数 */
	daily_astrology_tms						= 10323,	/*! 当前占星次数 */
	daily_astrology_reward_stat				= 10324,	/*! 当前占星奖励领取状态 */
	daily_astrology_init_flag				= 10325,	/*! 占星初始标志 */
	daily_astrology_refresh_tms				= 10326,	/*! 占星刷新标志 */
	/*! 占星限制 预留100300-10340 */

	/******************************! 每日清零 10000 ~ 19999 ********************************/




	/******************************! 每月清零 20000 ~ 29999 ********************************/
	month_checkin_days_stat			= 20001,	/*! 每月签到状态 按位存*/
	
	/******************************! 每月清零 20000 ~ 29999 ********************************/



	/******************************! 每周(周四节点)清零 30000 ~ 39999 ********************************/

	/******************************! 每周(周四节点)清零 30000 ~ 39999 ********************************/



	/******************************! 每周(周日节点)清零 40000 ~ 49999 ********************************/

	/******************************! 每周(周日节点)清零 40000 ~ 49999 ********************************/
	
	
	/******************************! 带期限的限制 50000 ~ 59999 ********************************/

	/******************************! 带期限的限制 50000 ~ 59999 ********************************/
};


/* @brief 限制信息-缓存
 */
struct res_cache_info_t {
	uint32_t type;
	uint32_t value;
	uint32_t expire_tm;
};
typedef std::map<uint32_t, res_cache_info_t> ResCacheMap;

/* @brief 限制管理类
 */ 
class Restriction {
public:
    Restriction(Player *p);
    ~Restriction();

	const res_cache_info_t* get_res_info(uint32_t type);
	uint32_t get_res_value(uint32_t type);
	void set_res_value(uint32_t type, uint32_t value, uint32_t expire_tm = 0);

	void init_res_value_info(std::vector<db_res_info_t> *p_vec);

private:
	uint32_t get_res_value_with_expire(uint32_t type);
	void set_res_value_with_expire(uint32_t type, uint32_t value, uint32_t expire_tm);

private:
	Player *owner;
    ResCacheMap res_map;
};



/* @brief 限制配置表信息
 */
struct res_xml_info_t {
    uint32_t id;
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t min;
    uint32_t sec;
    uint32_t expire_tm;	
};
typedef std::map<uint32_t, res_xml_info_t> ResXmlMap;

/* @brief 限制时间配置表管理类
 */
class ResXmlManage {
public:
    ResXmlManage();
    ~ResXmlManage();

    int read_from_xml(const char *filename);
	int init_res_day_info(res_xml_info_t &info, const char *day_str);

    const res_xml_info_t* get_res_xml_info(uint32_t res_id);

private:
    int load_res_info(xmlNodePtr cur);

private:
    ResXmlMap res_xml_map;
};

extern ResXmlManage res_xml_mgr;

/********************************************************************************/
/*								Lua Interface 									*/
/********************************************************************************/

int luaopen_restriction(lua_State *L);

#endif
