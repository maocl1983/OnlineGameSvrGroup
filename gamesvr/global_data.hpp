#ifndef GLOBAL_DATA_HPP_
#define GLOBAL_DATA_HPP_


extern "C"{
#include <libcommon/time/timer.h> 
}
#include "achievement.hpp"
#include "adventure.hpp"
#include "alarm.hpp"
#include "arena.hpp"
#include "battle.hpp"
#include "btl_soul.hpp"
#include "common_fight.hpp"
#include "equipment.hpp"
#include "general.hpp"
#include "guild.hpp"
#include "hero.hpp"
#include "horse.hpp"
#include "instance.hpp"
#include "internal_affairs.hpp"
#include "item.hpp"
#include "log_thread.hpp"
#include "lua_script_manage.hpp"
#include "player.hpp"
#include "redis.hpp"
#include "restriction.hpp"
#include "shop.hpp"
#include "global_data.hpp"
#include "soldier.hpp"
#include "stat_log.hpp"
#include "switch.hpp"
#include "skill.hpp"
#include "talent.hpp"
#include "task.hpp"
#include "ten_even_draw.hpp"
#include "timer.hpp"
#include "treasure_risk.hpp"
#include "trial_tower.hpp"
#include "utils.hpp"
#include "vip.hpp"

extern AchievementXmlManager* achievement_xml_mgr;
extern AdventureXmlManager* adventure_xml_mgr;
extern AdventureSelectXmlManager* adventure_select_xml_mgr;
extern AdventureItemXmlManager* adventure_item_xml_mgr;
extern ArenaManager* arena_mgr;
extern ArenaAttrXmlManager* arena_attr_xml_mgr;
extern ArenaHeroXmlManager* arena_hero_xml_mgr;
extern ArenaBonusXmlManager* arena_bonus_xml_mgr;
extern ArenaLevelAttrXmlManager* arena_level_attr_xml_mgr;
extern BattleCacheMap* g_battle_cache_map;
extern BtlSoulXmlManager* btl_soul_xml_mgr;
extern BtlSoulLevelXmlManager* btl_soul_level_xml_mgr;
extern DivineItemXmlManager* divine_item_xml_mgr;
extern CommonFightXmlManager* common_fight_xml_mgr;
extern CommonFightDropXmlManager* common_fight_drop_xml_mgr;
extern EquipmentXmlManager* equip_xml_mgr;
extern EquipRefiningXmlManager* equip_refining_xml_mgr;
extern EquipCompoundXmlManager* equip_compound_xml_mgr;
extern EquipLevelXmlManager* equip_level_xml_mgr;
extern NickXmlManager* nick_xml_mgr;
extern GuildManager* guild_mgr;
extern HeroXmlManager* hero_xml_mgr;
extern HeroRankXmlManager* hero_rank_xml_mgr;
extern HeroRankStuffXmlManager* hero_rank_stuff_xml_mgr;
extern HeroLevelAttrXmlManager* hero_level_attr_xml_mgr;
extern LevelXmlManager* level_xml_mgr;
extern HeroHonorXmlManager* hero_honor_xml_mgr;
extern HeroHonorExpXmlManager* hero_honor_exp_xml_mgr;
extern HeroTitleXmlManager* hero_title_xml_mgr;
extern HorseAttrXmlManager* horse_attr_xml_mgr;
extern HorseExpXmlManager* horse_exp_xml_mgr;
extern HorseEquipXmlManager* horse_equip_xml_mgr;
extern InstanceXmlManager* instance_xml_mgr;
//extern TroopXmlManager* troop_xml_mgr;
extern MonsterXmlManager* monster_xml_mgr;
extern InstanceDropXmlManager* instance_drop_xml_mgr;
extern InstanceChapterXmlManager* instance_chapter_xml_mgr;
extern InstanceBagXmlManager* instance_bag_xml_mgr;
extern InternalAffairsXmlManager* internal_affairs_xml_mgr;
extern InternalAffairsRewardXmlManager* internal_affairs_reward_xml_mgr;
extern InternalAffairsLevelXmlManager* internal_affairs_level_xml_mgr;
extern ItemsXmlManager* items_xml_mgr;
extern HeroRankItemXmlManager* hero_rank_item_xml_mgr;
extern ItemPieceXmlManager* item_piece_xml_mgr;
extern RandomItemXmlManager* random_item_xml_mgr;
extern LuaScriptManage* lua_script_mgr;
extern PlayerManager* g_player_mgr;
extern RoleSkillXmlManager* role_skill_xml_mgr;
extern Redis* redis_mgr;
extern ResXmlManage* res_xml_mgr;
extern ShopXmlManager* shop_xml_mgr;
extern ItemShopXmlManager* item_shop_xml_mgr;
extern SkillXmlManager* skill_xml_mgr;
extern SkillEffectXmlManager* skill_effect_xml_mgr;
extern SkillLevelupGoldsXmlManager* skill_levelup_golds_xml_mgr;
extern SoldierXmlManager* soldier_xml_mgr;
extern SoldierRankXmlManager* soldier_rank_xml_mgr;
extern SoldierStarXmlManager* soldier_star_xml_mgr;
extern SoldierTrainCostXmlManager* soldier_train_cost_xml_mgr;
extern SoldierLevelAttrXmlManager* soldier_level_attr_xml_mgr;
extern HeroTalentXmlManager* hero_talent_xml_mgr;
extern SoldierTalentXmlManager* soldier_talent_xml_mgr;
extern TaskXmlManager* task_xml_mgr;
extern TenEvenDrawGoldsXmlManager* ten_even_draw_golds_xml_mgr;
extern TenEvenDrawDiamondXmlManager* ten_even_draw_diamond_xml_mgr;
extern TenEvenDrawDiamondSpecialXmlManager* ten_even_draw_diamond_special_xml_mgr;
extern TimeStampXmlManage* time_stamp_xml_info;
extern Timer* timer_mgr;
extern TreasureAttrXmlManager* treasure_attr_xml_mgr;
extern TreasureHeroXmlManager* treasure_hero_xml_mgr;
extern TreasureRewardXmlManager* treasure_reward_xml_mgr;
extern TrialTowerRewardXmlManager* trial_tower_reward_xml_mgr;
extern Utils* utils_mgr; 
extern VipXmlManager* vip_xml_mgr;

/*! 全局变量-服务器ID */
extern uint32_t g_server_id;
extern int alarm_fd;
extern int dbroute_fd;
extern struct log_list_t g_log_list;
extern stat_log_list_t g_stat_log_list;
extern int stat_svr_fd;
extern char *stat_file;
extern int switch_fd;
extern struct list_head timers_list;
#define max_timer_type 1000
extern callback_func_t tcfs[max_timer_type];

extern pthread_t stat_tid;
extern pthread_t log_tid;

#endif
