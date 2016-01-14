/*
 * =====================================================================================
 *
 *  @file  soldier.hpp 
 *
 *  @brief  小兵系统
 *
 *  compiler  gcc4.4.7 
 *  
 *  platform  Linux
 *
 *  copyright:  King, Inc. ShangHai CN. All rights reserved
 *
 * =====================================================================================
 */

#ifndef SOLDIER_HPP_
#define SOLDIER_HPP_

#include "common_def.hpp"
#include "equipment.hpp"

class Player;
class soldier_xml_info_t;
class cli_soldier_info_t;
class db_get_player_soldiers_info_out;
class cli_get_soldiers_info_out;
class db_soldier_info_t;
class common_soldier_attr_info_t;
class cli_equip_info_t;
class cli_item_info_t;

/* @brief 小兵属性类型
 */
enum soldier_attr_type_t {
	em_soldier_attr_type_start 	= 0,
	em_soldier_attr_type_base		= 1,
	em_soldier_attr_type_equip		= 2,
	em_soldier_attr_type_talent	= 3,

	em_soldier_attr_type_end
};

/*! 小兵属性 */
struct soldier_attr_info_t {
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

/********************************************************************************/
/*							Soldier Class										*/
/********************************************************************************/

class Soldier {
public:
	uint32_t id;			/*! 小兵ID */
	uint32_t get_tm;		/*! 小兵获取时间 */
	uint32_t lv;			/*! 小兵等级 */
	uint32_t exp;			/*! 小兵经验 */
	uint32_t rank;			/*! 小兵品阶 */
	uint32_t rank_exp;		/*! 小兵品阶经验 */
	uint32_t star;			/*! 小兵星级 */
	uint32_t state;			/*! 小兵状态 */
	uint32_t btl_power;		/*! 小兵战斗力 */

	uint32_t hp;			/*! 当前血量 */
	uint32_t max_hp;		/*! 最大血量 */
	uint32_t ad;			/*! 攻击 */
	uint32_t armor;			/*! 护甲 */
	uint32_t resist;		/*! 魔抗 */
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

	uint32_t train_lv[4];	/*! 四项训练等级 */

	const soldier_xml_info_t *base_info;	/*! 小兵配置信息 */

public:
	Soldier(Player *p, uint32_t soldier_id);
	~Soldier();

	int init_soldier_base_info();
	int init_soldier_info(const db_soldier_info_t *p_info);

	int get_level_up_exp();
	int calc_soldier_max_lv();
	int add_exp(uint32_t add_value);
	int eat_exp_items(uint32_t item_id, uint32_t item_cnt);

	int get_rank_up_exp();
	int add_rank_exp(uint32_t add_value);

	uint32_t get_need_star();
	int rising_star();

	int strength_soldier(std::vector<cli_item_info_t> &cards);
	int soldier_training(uint32_t type, uint32_t add_lv);

	int set_soldier_status(uint32_t status);

	int calc_soldier_btl_power();
	int calc_soldier_base_btl_power();
	int calc_soldier_level_btl_power();
	int calc_soldier_rank_btl_power();

	void calc_talent_attr(soldier_attr_info_t &talent_attr1, soldier_attr_info_t &talent_attr2);
	void calc_talent_gain();
	void calc_all(bool flag=false);

	void pack_soldier_db_info(db_soldier_info_t &info);
	void pack_soldier_client_info(cli_soldier_info_t &info);

	int send_soldier_attr_change_noti();

private:
	Player *owner;
};



/********************************************************************************/
/*						SoldierManager Class										*/
/********************************************************************************/
typedef std::map<uint32_t, Soldier*> SoldierMap;
/* @brief 小兵管理信息类
 */
class SoldierManager {
public:
	SoldierManager(Player *p);
	~SoldierManager();

	Soldier* add_soldier(uint32_t soldier_id);
	Soldier* get_soldier(uint32_t soldier_id);
	int soldier_evolution(uint32_t old_soldier_id, uint32_t soldier_id);

	int calc_btl_power();
	int calc_over_cur_star_soldier_cnt(uint32_t star);
	int calc_over_cur_rank_soldier_cnt(uint32_t rank);

	int init_all_soldiers_info(db_get_player_soldiers_info_out *p_in); 
	int pack_all_soldiers_info(cli_get_soldiers_info_out &out);

private:
	Player *owner;
	SoldierMap soldiers;
};

/********************************************************************************/
/*						SoldierXmlManager Class									*/
/********************************************************************************/

struct soldier_xml_info_t {
	uint32_t id;
	uint32_t camp;				/*! 阵营 */
	uint32_t sex;				/*! 性别 */
	uint32_t army;				/*! 兵种 */
	uint32_t rank;				/*! 初始品阶 */
	uint32_t range;				/*! 射程 */
	uint32_t atk_type;			/*! 攻击类型 */
	uint32_t def_type;			/*! 护甲类型 */
	double max_hp;				/*! 血量上限 */
	double ad;					/*! 攻击 */
	double armor;				/*! 护甲 */
	double resist;				/*! 魔抗 */
	double hp_up;				/*! 每次训练血量提升 */
	double ad_up;				/*! 每次训练攻击提升 */
	double armor_up;			/*! 每次训练护甲提升 */
	double resist_up;			/*! 每次训练魔抗提升 */
};

typedef std::map<uint32_t, soldier_xml_info_t> SoldierXmlMap;

/* @brief 小兵配置管理类
 */
class SoldierXmlManager {
public:
	SoldierXmlManager();
	~SoldierXmlManager();

	int read_from_xml(const char *filename);

	const soldier_xml_info_t* get_soldier_xml_info(uint32_t soldier_id);

private:
	int load_soldier_xml_info(xmlNodePtr cur);
	
private:
	SoldierXmlMap soldier_xml_map;
};
//extern SoldierXmlManager soldier_xml_mgr;

/********************************************************************************/
/*						SoldierRankXmlManager Class						*/
/********************************************************************************/
struct soldier_rank_detail_xml_info_t {
	uint32_t rank;
	uint32_t max_hp;
	uint32_t ad;
	uint32_t armor;
	uint32_t resist;
	uint32_t exp;
	uint32_t talent_id;
};
typedef std::map<uint32_t, soldier_rank_detail_xml_info_t> SoldierRankDetailXmlMap;

struct soldier_rank_xml_info_t {
	uint32_t soldier_id;
	SoldierRankDetailXmlMap rank_map;
};
typedef std::map<uint32_t, soldier_rank_xml_info_t> SoldierRankXmlMap;

class SoldierRankXmlManager {
public:
	SoldierRankXmlManager();
	~SoldierRankXmlManager();

	int read_from_xml(const char *filename);
	const soldier_rank_detail_xml_info_t* get_soldier_rank_xml_info(uint32_t soldier_id, uint32_t rank);

private:
	int load_soldier_rank_xml_info(xmlNodePtr cur);

private:
	SoldierRankXmlMap soldier_map;
};
//extern SoldierRankXmlManager soldier_rank_xml_mgr;

/********************************************************************************/
/*							SoldierStarXmlManager Class							*/
/********************************************************************************/
struct soldier_star_detail_xml_info_t {
	uint32_t star;		/*! 小兵星级 */
	uint32_t soul_id;	/*! 灵魂碎片ID */
	uint32_t soul_cnt;	/*! 灵魂碎片个数 */
	uint32_t golds;
	uint32_t evolution_id;
};
typedef std::map<uint32_t, soldier_star_detail_xml_info_t> SoldierStarDetailXmlMap;

struct soldier_star_xml_info_t {
	uint32_t soldier_id;
	SoldierStarDetailXmlMap star_map;
};
typedef std::map<uint32_t, soldier_star_xml_info_t> SoldierStarXmlMap;

/* @brief 小兵星级配置类
 */
class SoldierStarXmlManager {
public:
	SoldierStarXmlManager();
	~SoldierStarXmlManager();

	int read_from_xml(const char *filename);
	const soldier_star_detail_xml_info_t * get_soldier_star_xml_info(uint32_t soldier_id, uint32_t star);

private:
	int load_soldier_star_xml_info(xmlNodePtr cur);

private:
	SoldierStarXmlMap soldier_map;
};
//extern SoldierStarXmlManager soldier_star_xml_mgr;

/********************************************************************************/
/*							SoldierTrainCostXmlManager Class					*/
/********************************************************************************/
struct soldier_train_cost_xml_info_t {
	uint32_t level;
	uint32_t cost[4];
};
typedef std::map<uint32_t, soldier_train_cost_xml_info_t> SoldierTrainCostXmlMap;

class SoldierTrainCostXmlManager {
public:
	SoldierTrainCostXmlManager();
	~SoldierTrainCostXmlManager();

	int read_from_xml(const char *filename);
	const soldier_train_cost_xml_info_t * get_soldier_train_cost_xml_info(uint32_t level);

private:
	int load_soldier_train_cost_xml_info(xmlNodePtr cur);

private:
	SoldierTrainCostXmlMap train_cost_map;
};
//extern SoldierTrainCostXmlManager soldier_train_cost_xml_mgr;

/********************************************************************************/
/*							SoldierLevelAttrXmlManager Class					*/
/********************************************************************************/
struct soldier_level_attr_xml_info_t {
	uint32_t soldier_id;
	double max_hp;
	double ad;
	double armor;
	double resist;
};
typedef std::map<uint32_t, soldier_level_attr_xml_info_t> SoldierLevelAttrXmlMap;

class SoldierLevelAttrXmlManager {
public:
	SoldierLevelAttrXmlManager();
	~SoldierLevelAttrXmlManager();

	int read_from_xml(const char *filename);
	const soldier_level_attr_xml_info_t *get_soldier_level_attr_xml_info(uint32_t soldier_id);

private:
	int load_soldier_level_attr_xml_info(xmlNodePtr cur);

private:
	SoldierLevelAttrXmlMap soldier_level_attr_xml_map;
};
//extern SoldierLevelAttrXmlManager soldier_level_attr_xml_mgr;

#endif //SOLDIER_HPP_
