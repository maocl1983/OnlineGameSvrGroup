/*
 * =====================================================================================
 *
 *  @file  hero.hpp 
 *
 *  @brief  英雄系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef HERO_HPP_
#define HERO_HPP_

#include "common_def.hpp"
#include "equipment.hpp"
#include "btl_soul.hpp"

class Player;
class hero_xml_info_t;
class cli_hero_info_t;
class db_get_player_heros_info_out;
class cli_get_heros_info_out;
class db_hero_simple_info_t;
class db_hero_info_t;
class common_hero_attr_info_t;
class cli_equip_info_t;
class cli_btl_soul_info_t;
class cli_hero_put_on_equipment_out;

/* @brief 英雄属性类型
 */
enum hero_attr_type_t {
	em_hero_attr_type_start 	= 0,
	em_hero_attr_type_base		= 1,
	em_hero_attr_type_equip		= 2,
	em_hero_attr_type_talent	= 3,
	em_hero_attr_type_btl_soul	= 4,
	em_hero_attr_type_honor		= 5,
	em_hero_attr_type_horse		= 6,

	em_hero_attr_type_end
};

/********************************************************************************/
/*							Hero Class										*/
/********************************************************************************/

class Hero {
public:
	uint32_t id;			/*! 英雄ID */
	uint32_t get_tm;		/*! 英雄获取时间 */
	uint32_t rank;			/*! 英雄品阶 */
	uint32_t star;			/*! 英雄星级 */
	uint32_t lv;			/*! 英雄等级 */
	uint32_t lv_factor;		/*! 等级系数 */
	uint32_t exp;			/*! 英雄经验 */
	uint32_t honor_lv;		/*! 英雄战功等级 */
	uint32_t honor;			/*! 英雄战功 */
	uint32_t btl_power;		/*! 英雄战斗力 */
	uint32_t trip_id;		/*! 羁绊ID */
	uint32_t title;			/*! 官衔 */
	uint32_t state;			/*! 英雄状态 */
	uint32_t is_in_affairs;	/*! 是否在内政 */

	/*! 基础属性 */
	double str_growth;	/*! 力量成长 */
	double int_growth;	/*! 智力成长 */
	double agi_growth;	/*! 敏捷成长 */

	uint32_t strength;		/*! 力量 */
	uint32_t intelligence;	/*! 智力 */
	uint32_t agility;		/*! 敏捷 */
	uint32_t speed;			/*! 速度 */

	uint32_t hp;			/*! 当前血量 */
	uint32_t max_hp;		/*! 最大血量 */
	uint32_t hp_regain;		/*! 血量回复 */
	uint32_t atk_spd;		/*! 攻速 */
	uint32_t ad;			/*! 攻击 */
	uint32_t armor;			/*! 护甲 */
	uint32_t ad_cri;		/*! 暴击 */
	uint32_t ad_ren;		/*! 韧性 */
	uint32_t resist;		/*! 魔抗 */
	uint32_t ad_chuan;		/*! 破甲 */
	uint32_t ap_chuan;		/*! 破魔 */
	uint32_t hit;			/*! 命中率*/
	uint32_t miss;			/*! 闪避率 */

	hero_attr_info_t base_attr;		/*! 英雄基础属性 */
	hero_attr_info_t equip_attr;	/*! 英雄装备属性 */
	hero_attr_info_t talent_attr;	/*! 英雄天赋属性 */
	hero_attr_info_t btl_soul_attr;	/*! 英雄战魂属性 */
	hero_attr_info_t honor_attr;	/*! 英雄战功属性 */
	hero_attr_info_t horse_attr;	/*! 英雄战马属性 */


	uint32_t skill_lv;				/*! 技能等级 */
	uint32_t unique_skill_lv;		/*! 绝招技能等级 */
	EquipmentMap equips;	/*! 英雄所携装备信息 */
	BtlSoulMap btl_souls;	/*! 英雄所装备战魂信息 */
	
	std::vector<uint32_t> talents;		/*!  当前解锁天赋 */

	const hero_xml_info_t *base_info;	/*! 英雄配置信息 */

public:
	Hero(Player *p, uint32_t hero_id);
	~Hero();

	int init_hero_base_info(uint32_t init_lv = 0);
	int init_hero_db_info(const db_hero_info_t *p_info);

	int get_level_up_exp(uint32_t lv);
	int add_exp(uint32_t add_value);
	int get_level_up_honor(uint32_t lv);
	int add_honor(uint32_t add_honor, bool item_flag = false);
	uint32_t get_need_star();
	int rising_rank();
	int eat_exp_items(uint32_t item_id, uint32_t item_cnt);
	int eat_honor_items(uint32_t item_id, uint32_t item_cnt);

	int get_hero_grant_state();
	int get_hero_max_grant_title();
	bool check_hero_is_can_grant(uint32_t title_id);
	int grant_hero_title(uint32_t title_id);
	int set_hero_status(uint32_t status);

	Equipment *get_hero_equipment(uint32_t get_tm);
	Equipment *get_hero_same_type_equip(uint32_t equip_type);
	int put_on_equipment(uint32_t get_tm, uint32_t &off_get_tm);
	int put_on_equipment_list(std::vector<uint32_t> &equips, cli_hero_put_on_equipment_out &cli_out);
	int put_off_equipment(uint32_t get_tm);

	BtlSoul *get_hero_btl_soul(uint32_t get_tm);
	BtlSoul *get_same_type_btl_soul(uint32_t type);
	int put_on_btl_soul(uint32_t get_tm, uint32_t &off_get_tm);
	int put_off_btl_soul(uint32_t get_tm);

	int skill_level_up(uint32_t type, uint32_t add_lv);
	int normal_skill_level_up(uint32_t add_lv);
	int unique_skill_level_up(uint32_t add_lv);
	int role_skill_level_up(uint32_t type, uint32_t add_lv);

	/*! 战斗力相关计算 */
	int calc_hero_btl_power();
	double calc_hero_base_btl_power();
	double calc_hero_skill_btl_power();
	double calc_hero_equip_btl_power();
	double calc_hero_rank_btl_power();
	double calc_hero_btl_soul_btl_power();
	double calc_hero_horse_btl_power();
	double calc_hero_honor_btl_power();

	/*! 属性相关计算 */
	int calc_hero_max_lv();
	int calc_hero_max_honor_lv();
	int calc_hero_growth();
	int calc_cur_rank_attr(hero_attr_info_t &rank_attr);
	int calc_cur_talent_attr(hero_attr_info_t &talent_attr1, hero_attr_info_t &talent_attr2);
	int calc_cur_level_attr(hero_attr_info_t &level_attr);
	void calc_hero_base_attr();
	void calc_hero_equip_attr();
	void calc_hero_talent_attr();
	void calc_hero_btl_soul_attr();
	void calc_hero_honor_attr();
	void calc_hero_horse_attr();
	void calc_all(bool flag=false);

	void pack_hero_client_info(cli_hero_info_t &info);
	void pack_hero_simple_info(db_hero_simple_info_t &info);
	void pack_hero_db_info(db_hero_info_t &info);
	void pack_hero_attr_info(uint32_t type, common_hero_attr_info_t &info);
	void pack_hero_equips_info(std::vector<cli_equip_info_t> &equip_vec);
	void pack_hero_btl_soul_info(std::vector<cli_btl_soul_info_t> &btl_soul_vec);

	int send_hero_attr_change_noti();

private:
	Player *owner;
};



/********************************************************************************/
/*						HeroManager Class										*/
/********************************************************************************/
typedef std::map<uint32_t, Hero*> HeroMap;
/* @brief 英雄管理信息类
 */
class HeroManager {
public:
	HeroManager(Player *p);
	~HeroManager();

	Hero* add_hero(uint32_t hero_id);
	Hero* get_hero(uint32_t hero_id);

	int set_trip_hero(uint32_t hero_id, uint32_t trip_id);

	int calc_btl_power();
	int calc_hero_max_lv();
	int calc_all_heros_honor_lv();

	int calc_over_cur_lv_hero_cnt(uint32_t lv);
	int calc_over_cur_rank_hero_cnt(uint32_t rank);
	int calc_over_cur_star_hero_cnt(uint32_t star);
	int calc_over_cur_honor_lv_hero_cnt(uint32_t honor_lv); 

	int deal_hero_title_grant_state();

	int init_all_heros_info(db_get_player_heros_info_out *p_in); 
	int pack_all_heros_info(cli_get_heros_info_out &out);

private:
	Player *owner;
	HeroMap heros;
};

/********************************************************************************/
/*						HeroXmlManager Class									*/
/********************************************************************************/

struct hero_xml_info_t {
	uint32_t id;
	uint32_t camp;				/*! 阵营 */
	uint32_t sex;				/*! 性别 */
	uint32_t army;				/*! 兵种 */
	uint32_t star;				/*! 初始星级 */
	uint32_t str_growth;		/*! 力量成长 */
	uint32_t int_growth;		/*! 智力成长 */
	uint32_t agi_growth;		/*! 敏捷成长 */
	uint32_t range;				/*! 射程 */
	uint32_t atk_type;			/*! 攻击类型 */
	uint32_t def_type;			/*! 护甲类型 */
	hero_attr_info_t attr;		/*! 英雄基础属性 */

	std::vector<uint32_t> skills;	/*! 英雄技能 */
	std::vector<uint32_t> talents;	/*! 英雄天赋 */
};

typedef std::map<uint32_t, hero_xml_info_t> HeroXmlMap;

/* @brief 英雄配置管理类
 */
class HeroXmlManager {
public:
	HeroXmlManager();
	~HeroXmlManager();

	int read_from_xml(const char *filename);

	const hero_xml_info_t* get_hero_xml_info(uint32_t hero_id);

private:
	int load_hero_xml_info(xmlNodePtr cur);
	
private:
	HeroXmlMap hero_xml_map;
};
extern HeroXmlManager hero_xml_mgr;

/********************************************************************************/
/*						HeroRankXmlManager Class						*/
/********************************************************************************/
struct hero_rank_detail_xml_info_t {
	uint32_t rank;
	uint32_t talent_id;
	hero_attr_info_t attr;
};
typedef std::map<uint32_t, hero_rank_detail_xml_info_t> HeroRankDetailXmlMap;

struct hero_rank_xml_info_t {
	uint32_t hero_id;
	HeroRankDetailXmlMap rank_map;
};
typedef std::map<uint32_t, hero_rank_xml_info_t> HeroRankXmlMap;

class HeroRankXmlManager {
public:
	HeroRankXmlManager();
	~HeroRankXmlManager();

	int read_from_xml(const char *filename);
	const hero_rank_detail_xml_info_t* get_hero_rank_xml_info(uint32_t hero_id, uint32_t rank);

private:
	int load_hero_rank_xml_info(xmlNodePtr cur);
	int load_hero_rank_detail_xml_info(xmlNodePtr cur, HeroRankDetailXmlMap &rank_map);

private:
	HeroRankXmlMap hero_map;
};
extern HeroRankXmlManager hero_rank_xml_mgr;

/********************************************************************************/
/*							HeroRankStuffXmlManager Class						*/
/********************************************************************************/
struct hero_rank_stuff_item_info_t {
	uint32_t stuff_id;	/*! 材料ID */
	uint32_t num;		/*! 材料个数 */
};

struct hero_rank_stuff_detail_xml_info_t {
	uint32_t rank;							//品阶
	uint32_t hero_card_cnt;					//所需本体卡数量
	uint32_t golds;							//所需金币
	hero_rank_stuff_item_info_t stuff[2];	//所需材料
};
typedef std::map<uint32_t, hero_rank_stuff_detail_xml_info_t> HeroRankStuffDetailXmlMap;

struct hero_rank_stuff_xml_info_t {
	uint32_t army_id;
	HeroRankStuffDetailXmlMap rank_map;
};
typedef std::map<uint32_t, hero_rank_stuff_xml_info_t> HeroRankStuffXmlMap;

/* @brief 英雄品阶属性提升配置表
 */
class HeroRankStuffXmlManager {
public:
	HeroRankStuffXmlManager();
	~HeroRankStuffXmlManager();

	int read_from_xml(const char *filename);
	const hero_rank_stuff_detail_xml_info_t * get_hero_rank_stuff_xml_info(uint32_t hero_id, uint32_t rank);

private:
	int load_hero_rank_stuff_xml_info(xmlNodePtr cur);

private:
	HeroRankStuffXmlMap stuff_map;
};
extern HeroRankStuffXmlManager hero_rank_stuff_xml_mgr;

/********************************************************************************/
/*							HeroLevelAttrXmlManager Class							*/
/********************************************************************************/
struct hero_level_attr_xml_info_t {
	uint32_t hero_id;	/*! 英雄ID */
	double maxhp;		/*! 血量 */
	double ad;			/*! 攻击 */
	double armor;		/*! 护甲 */
	double resist;		/*! 魔抗 */
	double hp_regain;	/*! 血量回复 */
};
typedef std::map<uint32_t, hero_level_attr_xml_info_t> HeroLevelAttrXmlMap;

/* @brief 英雄等级属性配置类
 */
class HeroLevelAttrXmlManager {
public:
	HeroLevelAttrXmlManager();
	~HeroLevelAttrXmlManager();

	int read_from_xml(const char *filename);
	const hero_level_attr_xml_info_t * get_hero_level_attr_xml_info(uint32_t hero_id);

private:
	int load_hero_level_attr_xml_info(xmlNodePtr cur);

private:
	HeroLevelAttrXmlMap hero_map;
};
extern HeroLevelAttrXmlManager hero_level_attr_xml_mgr;

/********************************************************************************/
/*							LevelXmlManager Class							*/
/********************************************************************************/
struct level_xml_info_t {
	uint32_t level;
	uint32_t master_exp;
	uint32_t hero_exp;
	uint32_t soldier_exp;
	uint32_t levelup_endurance;
	uint32_t adventure;
	uint32_t max_lv;
	uint32_t equip_max_lv;
	uint32_t item_id;
	uint32_t item_cnt;
	uint32_t unlock_soldier;
	double btl_power_scale;
};
typedef std::map<uint32_t, level_xml_info_t> LevelXmlMap;

class LevelXmlManager {
public:
	LevelXmlManager();
	~LevelXmlManager();

	int read_from_xml(const char *filename);
	const level_xml_info_t * get_level_xml_info(uint32_t level);

private:
	int load_level_xml_info(xmlNodePtr cur);

private:
	LevelXmlMap level_map;
};
extern LevelXmlManager level_xml_mgr;

/********************************************************************************/
/*							HeroHonorXmlManager Class							*/
/********************************************************************************/
struct hero_honor_level_xml_info_t {
	uint32_t lv;
	double max_hp;
	double ad;
	double armor;
	double resist;
};
typedef std::map<uint32_t, hero_honor_level_xml_info_t> HeroHonorLevelXmlMap;

struct hero_honor_xml_info_t {
	uint32_t army;
	HeroHonorLevelXmlMap level_map;
};
typedef std::map<uint32_t, hero_honor_xml_info_t> HeroHonorXmlMap;

class HeroHonorXmlManager {
public:
	HeroHonorXmlManager();
	~HeroHonorXmlManager();

	int read_from_xml(const char *filename);
	const hero_honor_level_xml_info_t *get_hero_honor_xml_info(uint32_t army, uint32_t lv);

private:
	int load_hero_honor_xml_info(xmlNodePtr cur);

private:
	HeroHonorXmlMap honor_map;
};
extern HeroHonorXmlManager hero_honor_xml_mgr;


/********************************************************************************/
/*							HeroHonorExpXmlManager Class						*/
/********************************************************************************/
struct hero_honor_exp_xml_info_t {
	uint32_t lv;
	uint32_t exp;
	double cri_prob;
	uint32_t btl_power;
};
typedef std::map<uint32_t, hero_honor_exp_xml_info_t> HeroHonorExpXmlMap;

class HeroHonorExpXmlManager {
public:
	HeroHonorExpXmlManager();
	~HeroHonorExpXmlManager();

	int read_from_xml(const char *filename);
	const hero_honor_exp_xml_info_t* get_hero_honor_exp_xml_info(uint32_t lv);

private:
	int load_hero_honor_exp_xml_info(xmlNodePtr cur);

private:
	HeroHonorExpXmlMap honor_exp_map;
};
extern HeroHonorExpXmlManager hero_honor_exp_xml_mgr;

/********************************************************************************/
/*							HeroTitleXmlManager Class							*/
/********************************************************************************/
struct hero_title_xml_info_t {
	uint32_t title_lv;
	std::vector<uint32_t> titles;
};
typedef std::map<uint32_t, hero_title_xml_info_t> HeroTitleXmlMap;

class HeroTitleXmlManager {
public:
	HeroTitleXmlManager();
	~HeroTitleXmlManager();

	uint32_t random_one_title(uint32_t title_lv);
	int read_from_xml(const char *filename);

private:
	int load_hero_title_xml_info(xmlNodePtr cur);

private:
	HeroTitleXmlMap hero_title_xml_map;
};
extern HeroTitleXmlManager hero_title_xml_mgr;

#endif //HERO_HPP_
