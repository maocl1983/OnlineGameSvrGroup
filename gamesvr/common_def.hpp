/*
 * =====================================================================================
 *
 *  @file  common_def.hpp 
 *
 *  @brief  保存常用的结构体信息，和常量的定义。
 *
 *  compiler  gcc4.4.7 
 *	
 *  platform  Linux
 *
 * =====================================================================================
 */

#ifndef COMMON_DEF_H_
#define COMMON_DEF_H_

extern "C"{
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <libcommon/log.h>
#include <libcommon/list.h>
#include <libcommon/tm_dirty/tm_dirty.h>
#include <libcommon/time/time.h>
#include <libcommon/time/timer.h>
#include <libcommon/conf_parser/config.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <math.h>
}
#include <libproject/utils/md5.h>
#include <asyn_serv/net_if.hpp>
#include <libproject/protobuf/Cproto_msg.h>
#include <libproject/protobuf/Cproto_util.h>
#include <libproject/protobuf/Cproto_map.h>
#include <libproject/memory/mempool.hpp>
#include <libproject/event/eventable_obj.hpp>
#include <libproject/event/event_mgr.hpp>
#include <libproject/conf_parser/xmlparser.hpp>
#include <libproject/inet/pdumanip.hpp>
#include <libproject/bitmanip/bitmanip.hpp>
#include <libproject/random/random.hpp>

#include "log_thread.hpp"

#include <set>
#include <map>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>

#define SAFE_DELETE(p) \
{ \
	if ((p)) { \
		delete (p); \
		(p) = 0; \
	} \
}

#define XMLCHAR_CAST(str) reinterpret_cast<const xmlChar*>(str)

#define XML_PARSE_FILE(filename) \
	xmlDocPtr doc = xmlParseFile(filename);\
	if (!doc) {\
		throw XmlParseError(std::string("failed to parse '") + filename + "'");\
		ERROR_LOG("failed to parse %s!", filename);\
	}\
	xmlNodePtr cur = xmlDocGetRootElement(doc);\
	if (!cur) {\
		xmlFreeDoc(doc);\
		throw XmlParseError(std::string("xmlDocGetRootElement error when loading '") + filename + "'");\
		ERROR_LOG("xmlDocGetRootElement error when loading %s!", filename);\
	}

//脏词检测-遇到脏词直接弹错误码
#define CHECK_DIRTYWORD_ERR_RET(p_, msg_) \
	do { \
		int r_ = tm_dirty_check(7, static_cast<char*>(msg_));\
		T_KDEBUG_LOG(p->user_id, "refuse dirty word [id=%u,nick=%s,msg=%s,ret=%d]", (p_)->user_id, (p_)->nick, msg_, r_); \
		if(r_ > 0) { \
			return (p_)->send_to_self_error((p_)->wait_cmd, cli_dirtyword_err, 1); \
		} \
	} while (0)

#define STR(x) #x
#define REQUIRE(libname)\
	lua_register(L, STR(luaopen_##libname), luaopen_##libname);


#define CHECK_SVR_ERR(p_, err_) \
	do { \
		if ((err_)) { \
			return p_->send_to_self_error(p_->wait_cmd, err_, 1); \
		} \
	} while(0)

#define CHECK_NEED_HONOR_ERR(p_, need_honor_) \
	do { \
		if ((p_->honor) < (need_honor_)) { \
			return p_->send_to_self_error(p_->wait_cmd, cli_not_enough_honor_err, 1); \
		} \
	} while(0)

#define CHECK_NEED_COINS_ERR(p_, need_coins_) \
	do { \
		if ((p_->coins) < (need_coins_)) { \
			return p_->send_to_self_error(p_->wait_cmd, cli_no_enough_coins_err, 1); \
		} \
	} while(0)

#define CHECK_NEED_GOLDS_ERR(p_, need_golds_) \
	do {\
		if ((p_->golds) < (need_golds_)) {\
			return p_->send_to_self_error(p_->wait_cmd, cli_not_enough_golds_err, 1);\
		}\
	} while(0)

#define CHECK_NEED_ENERGY_ERR(p_, need_energy_) \
	do {\
		if ((p_->energy) < (need_energy_)) {\
			return p_->send_to_self_error(p_->wait_cmd, cli_not_enough_energy_err, 1);\
		}\
	} while(0)

/*! 版本号-每次发版本后跟前端统一 */
const uint32_t cur_version = 1;

/*! 全局变量-服务器ID */
//extern uint32_t g_server_id;

/*! 全局变量-用户相关 */
const uint32_t MAX_SESSLEN = 1000; 					/*! 用户最大缓存长度 */
const uint32_t MAX_ENERGY = 150; 					/*! 用户最大行动力 */
const uint32_t MAX_ENDURANCE = 20; 					/*! 用户最大耐力 */
const uint32_t MAX_GOLDS = 10000000; 				/*! 用户最大金币数 */
const uint32_t MAX_DIAMOND = 10000000; 				/*! 用户最大钻石数 */
const uint32_t MAX_HERO_SOUL = 10000000; 			/*! 用户最大魂玉数 */
const uint32_t NICK_LEN = 32;						/*! 用户昵称长度 */
const uint32_t MAX_BTL_SOUL_EXP = 10000000;			/*! 用户最大战魂经验 */

/*! 全局变量-定时器相关 */
const uint32_t SKILL_POINT_PER_SEC = 360;			/*! 技能点恢复时间 */
const uint32_t SOLDIER_TRAIN_POINT_PER_SEC = 180;	/*! 兵种训练点恢复时间-3分钟 */
const uint32_t ENERGY_PER_SEC = 180;				/*! 体力恢复时间-3分钟 */
const uint32_t ENDURANCE_PER_SEC = 180;				/*! 耐力恢复时间-3分钟 */
const uint32_t ADVENTURE_PER_SEC = 900;				/*! 奇遇点恢复时间-15分钟 */

/*! 全局变量-英雄相关 */
const uint32_t MAX_HERO_LEVEL = 80;   		/*! 英雄最高等级 */
const uint32_t MAX_HERO_RANK = 12;			/*! 英雄最高品阶*/
const uint32_t MAX_HERO_SKILL_LEVEL = 100;	/*! 英雄技能最高等级 */
const uint32_t MAX_HERO_TALENT_LEVEL = 100;	/*! 英雄天赋最高等级 */
const uint32_t MAX_HERO_HONOR = 10000;   	/*! 英雄最高战功 */

/*! 全局变量-小兵相关 */
const uint32_t MAX_SOLDIER_RANK = 7;		/*! 小兵最大品阶 */
const uint32_t MAX_SOLDIER_STAR = 5;		/*! 小兵最大星级 */

/*! 全局变量-装备相关 */
const uint32_t MAX_EQUIPMENT_LEVEL = 80;			/*! 装备最大等级 */
const uint32_t MAX_EQUIPMENT_REFINING_LEVEL = 7;	/*! 装备最大精炼等级 */

/*! 全局变量-战魂相关 */
const uint32_t MAX_BTL_SOUL_LEVEL = 40;			/*! 战魂最大等级 */

/*! 全局变量-战马相关 */
const uint32_t MAX_HORSE_LEVEL = 80;			/*! 战马最大等级 */

/*! 全局变量-战斗力基础值 */
const uint32_t LEVEL_BASE_BTL_POWER = 150;			/*! 等级基础值 */
const uint32_t EQUIP_BASE_BTL_POWER = 8;			/*! 装备基础值 */
const uint32_t SUIT_BASE_BTL_POWER = 300;			/*! 套装基础值 */
const uint32_t SKILL_BASE_BTL_POWER = 100;			/*! 技能基础值 */
const uint32_t SOLDIER_TRAIN_BASE_BTL_POWER = 100;	/*! 兵种培养基础值 */
const uint32_t SOLDIER_LEVEL_BASE_BTL_POWER = 150;	/*! 兵种等级基础值 */
const uint32_t BTL_SOUL_BASE_BTL_POWER = 10;		/*! 战魂基础值 */

/*! 全局变量-内政相关 */
const uint32_t MAX_INTERNAL_AFFAIRS_LEVEL = 10;		/*! 内政最大等级 */


/*! 全局变量-服务器相关 */
const uint32_t MAX_BTLSVR_CNT = 20;			/*! 最大对战服务器连接数 */
const uint32_t MAX_HOME_CNT = 10;			/*! 最大小屋服务器连接数 */


enum day_type_t {
	em_normal_day = 1,
	em_weekend_day = 2,
};

/*! bit位开关 */
enum bit_type_t{
	em_bit_off	= 0,
	em_bit_on	= 1
};

/*! 英雄属性 */
struct hero_attr_info_t {
	uint32_t strength;		/*! 力量 */
	uint32_t intelligence;	/*! 智力 */
	uint32_t agility;		/*! 敏捷 */
	uint32_t speed;			/*! 速度 */

	double max_hp;			/*! 最大血量 */
	double ad;				/*! 物理攻击 */
	double armor;			/*! 物理护甲 */
	double resist;			/*! 魔法抗性 */
	uint32_t hp_regain;		/*! 血量回复 */
	uint32_t atk_spd;		/*! 攻速 */
	uint32_t ad_cri;		/*! 物理暴击 */
	uint32_t ad_ren;		/*! 物理韧性 */
	uint32_t ad_chuan;		/*! 物穿 */
	uint32_t ap_chuan;		/*! 法穿 */
	uint32_t hit;			/*! 命中率*/
	uint32_t miss;			/*! 闪避率 */
	uint32_t cri_damage;	/*! 暴伤 */
	uint32_t cri_avoid;		/*! 抗暴 */
	uint32_t hp_steal;		/*! 吸血 */
	uint32_t ad_avoid;		/*! 物理免伤 */
	uint32_t ap_avoid;		/*! 法术免伤 */
};

struct item_info_t {
	uint32_t id;
	uint32_t cnt;
};
#endif
