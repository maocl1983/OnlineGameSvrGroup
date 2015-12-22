/*
 * =====================================================================================
 *
 *  @file  player.hpp 
 *
 *  @brief  处理跟玩家有关的函数
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef PLAYER_HPP_
#define PLAYER_HPP_


#include "common_def.hpp"
#include "hero.hpp"
#include "item.hpp"
#include "restriction.hpp"
#include "instance.hpp"
#include "soldier.hpp"
#include "btl_soul.hpp"
#include "ten_even_draw.hpp"
#include "horse.hpp"
#include "treasure_risk.hpp"
#include "task.hpp"
#include "shop.hpp"
#include "adventure.hpp"
#include "achievement.hpp"
#include "common_fight.hpp"
#include "internal_affairs.hpp"
#include "vip.hpp"

class HeroManager;
class ItemsManager;
class VipManager;
class AstrologyManager;
class TrialTowerManager;
class db_get_player_login_info_out;
class cli_proto_login_out;
class cli_player_base_info_t;
class db_player_simple_info_t;
class db_get_troop_list_out;
class cli_get_troop_list_out;
class arena_info_t;
class cli_troop_info_t;
class cli_role_skills_info_t;
class cli_item_buy_info_t;



/********************************************************************************/
/*							Player Class										*/
/********************************************************************************/

/* @brief 玩家信息类
 */
class Player : public  project::EventableObject {
public:
	Player(uint32_t uid, fdsession_t *fds);
	~Player();

public:
	uint32_t user_id;				/*! 用户ID */
	uint32_t account_id;			/*! 账号ID */
	char nick[NICK_LEN];			/*! 用户昵称 */
	uint32_t reg_tm;				/*! 注册时间 */
	uint32_t sex;					/*! 性别 */
	uint32_t role_id;				/*! 用户主角ID */
	uint32_t guild_id;				/*! 公会ID */

	uint32_t lv;					/*! 战队等级 */
	uint32_t exp;					/*! 用户当前经验 */
	uint32_t vip_lv;				/*! vip等级 */
	uint32_t btl_power;				/*! 战斗力 */
	uint32_t max_btl_power;			/*! 历史最高战斗力 */

	uint32_t horse_lv;				/*! 战马当前等级 */
	uint32_t horse_exp;				/*! 战马经验 */

	uint32_t golds;					/*! 用户金币数量 */
	uint32_t diamond;				/*! 用户钻石数量 */
	uint32_t hero_soul;				/*! 用户魂玉数量 */
	uint32_t max_energy;			/*! 体力上限 */
	uint32_t energy;				/*! 当前体力 */
	uint32_t used_energy;			/*! 当前消耗的体力 */
	uint32_t endurance;				/*! 当前耐力值 */
	uint32_t adventure;				/*! 当前奇遇点 */
	uint32_t last_login_tm;			/*! 上次登入时间 */
	uint32_t last_logout_tm;		/*! 上次登出时间 */
	uint32_t today_online_tm;		/*! 当天在线时间 */
	uint32_t cur_login_tm;			/*! 当前登入时间 */
	uint32_t login_step;			/*! 登陆步骤 */
	uint32_t login_completed;		/*! 登陆是否完成 */
	uint32_t skill_point;			/*! 当前技能点数 */
	uint32_t soldier_train_point;	/*! 当前小兵训练点数 */
	uint32_t btl_soul_exp;			/*! 当前战魂经验 */

	TroopMap troop_map;				/*! 阵容信息 */


	ItemsManager *items_mgr;				/*! 用户物品信息 */
	HeroManager *hero_mgr;					/*! 用户英雄信息 */
	SoldierManager *soldier_mgr;			/*! 用户小兵信息 */
	EquipmentManager *equip_mgr;			/*! 用户装备信息 */
	Restriction *res_mgr;					/*! 用户限制信息 */
	InstanceManager *instance_mgr;			/*! 用户副本信息 */
	BtlSoulManager *btl_soul_mgr;			/*! 用户战魂信息 */
	TenEvenDrawManager* ten_even_draw_mgr;	/*! 用户十连抽信息 */
	Horse *horse;							/*! 战马信息 */
	HorseEquipManager* horse_equip_mgr;		/*! 战马装备信息 */
	TreasureManager *treasure_mgr;			/*! 夺宝信息 */
	TaskManager *task_mgr;					/*! 任务信息 */
	ShopManager *shop_mgr;					/*! 商城信息 */
	AdventureManager *adventure_mgr;		/*! 奇遇信息 */
	AchievementManager *achievement_mgr;	/*! 成就信息 */
	CommonFight *common_fight;				/*! 通用玩法信息 */
	InternalAffairs *internal_affairs;		/*! 内政信息 */
	VipManager *vip_mgr;					/*! VIP信息 */
	AstrologyManager *astrology_mgr;		/*! 占星信息 */
	TrialTowerManager *trial_tower_mgr;		/*! 试练塔信息 */

	uint32_t session_key;			/*! 会话验证秘钥 */
	uint32_t last_conn_tm;			/*! 上次连接时间 */

	/*! 定时器 */
	timer_struct_t *keep_alive_tm;			/*! 心跳包 */
	timer_struct_t *skill_point_tm;     	/*! 技能点定时器 */
	timer_struct_t *soldier_train_point_tm; /*! 小兵训练点定时器 */
	timer_struct_t *energy_tm;     			/*! 体力恢复定时器 */
	timer_struct_t *endurance_tm;     		/*! 耐力恢复定时器 */
	timer_struct_t *adventure_tm;     		/*! 奇遇点恢复定时器 */

	fdsession_t *fdsess;
	uint32_t wait_cmd;				/*! 缓存命令 */
	uint32_t seqno;					/*! 序列码 */

public:
	int init_player_login_info(db_get_player_login_info_out* p_in);
	int init_skill_point();
	int init_soldier_train_point();
	int init_energy();
	int init_endurance();
	int init_adventure();
	int init_troop_list(db_get_troop_list_out *p_in);

	int chg_golds(int32_t chg_value);
	int chg_diamond(int32_t chg_value);
	int chg_hero_soul(int32_t chg_value);
	int chg_energy(int32_t chg_value, bool recovery_flag=false, bool role_exp_flag=false );
	int chg_endurance(int32_t chg_value, bool recovery_flag=false, bool role_exp_flag=false);
	int chg_adventure(int32_t chg_value, bool recovery_flag=false);
	int chg_btl_soul_exp(int32_t chg_value);
	int set_player_level(uint32_t lv);
	int add_role_exp(uint32_t add_value);
	int chg_skill_point(int32_t chg_value);
	int chg_soldier_train_point(int32_t chg_value);

	int calc_max_energy();
	int calc_max_endurance();
	int calc_max_adventure();
	int calc_btl_power();
	int calc_arena_btl_power(const arena_info_t *p_info);
	int calc_energy_buy_max_tms();
	int calc_endurance_buy_max_tms();

	int deal_role_levelup(uint32_t old_lv, uint32_t lv);

	int add_skill_point_tm();
	int add_soldier_train_point_tm();
	int add_energy_tm();
	int add_endurance_tm();
	int add_adventure_tm();

	int get_today_online_tm();
	int set_player_last_login_tm();
	int set_player_last_logout_tm();
	int set_troop(cli_troop_info_t &troop_info);
	const troop_info_t *get_troop(uint32_t type);
	bool check_is_troop_hero(uint32_t hero_id);

	int send_energy_change_noti();

	//
	int deal_something_when_login();
	int complete_new_player_lead_task(uint32_t lead_id);

	void handle_timer_login_init();
	void handle_role_login_init();
	void handle_redis_login_init();
	void set_login_day();

	int get_buy_item_left_tms(uint32_t type);
	int buy_item(uint32_t type);
	int eat_energy_items(uint32_t item_id, uint32_t item_cnt);
	int eat_endurance_items(uint32_t item_id, uint32_t item_cnt);
	int eat_adventure_items(uint32_t item_id, uint32_t item_cnt);
	int use_battle_items(uint32_t item_id, uint32_t item_cnt);

	int set_role_skills(std::vector<uint32_t> &skills);
	bool get_role_skill_state(uint32_t skill_id);

	//pack function
	int pack_player_login_info(cli_proto_login_out &cli_out);
	int pack_player_base_info(cli_player_base_info_t &info);
	int pack_player_simple_info(db_player_simple_info_t &simple_info);
	void pack_player_troop_list(cli_get_troop_list_out &cli_out);
	void pack_items_buy_list(std::vector<cli_item_buy_info_t> &vec);

	int send_to_self(uint16_t cmd, Cmessage *c_out, int completed);
	int send_to_self_error(uint16_t cmd, int err, int completed);
	int send_login_info_to_client();

	int gen_session_key();
	int update_session_key();
	int gen_session(char *session);

private:
	int send_msg_to_cli(uint16_t cmd, Cmessage *msg);
};


/********************************************************************************/
/*							PlayerManager Class									*/
/********************************************************************************/

typedef std::map<uint32_t, Player*> PlayerUidMap;
typedef std::map<uint32_t, Player*> PlayerFdMap;
typedef std::map<uint32_t, Player*> PlayerAcctMap;

/* @brief 玩家管理类
 */
class PlayerManager {
public:
	PlayerManager(); 
	~PlayerManager();

public:
	Player* get_player_by_uid(uint32_t uid);
	Player* get_player_by_fd(uint32_t fd);
	Player* get_player_by_acct_id(uint32_t acct_id);
	Player* add_player(uint32_t uid, fdsession_t* fds, bool *cache_flag);
	int init_player(uint32_t uid, Player* p);
	void del_player(Player* p);
	void del_all_player();
	void del_expire_player(Player *p);

private:
	PlayerUidMap uid_map;
	PlayerFdMap fd_map;
	PlayerAcctMap acct_map;
};
extern PlayerManager g_player_mgr;


/********************************************************************************/
/*								RoleSkillXmlManager								*/
/********************************************************************************/
struct role_skill_xml_info_t {
	uint32_t id;
	uint32_t skill_id;
	uint32_t unlock_lv;
};
typedef std::map<uint32_t, role_skill_xml_info_t> RoleSkillXmlMap;

class RoleSkillXmlManager {
public:
	RoleSkillXmlManager();
	~RoleSkillXmlManager();

	int read_from_xml(const char *filename);
	const role_skill_xml_info_t * get_role_skill_xml_info(uint32_t id);
	const role_skill_xml_info_t * get_role_skill_xml_info_by_skill(uint32_t skill_id);
	void pack_unlock_role_skill(Player *p, std::vector<cli_role_skills_info_t> &skills);

private:
	int load_role_skill_xml_info(xmlNodePtr cur);

private:
	RoleSkillXmlMap role_skill_xml_map;
};
extern RoleSkillXmlManager role_skill_xml_mgr;

/********************************************************************************/
/*								Lua Interface									*/
/********************************************************************************/
int luaopen_player(lua_State *L);

#endif
