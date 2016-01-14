#include "global_data.hpp"
#include "cli_dispatch.hpp"
#include <libproject/event/event_mgr.hpp>

using namespace project;

struct list_head timers_list;
callback_func_t tcfs[max_timer_type];
AchievementXmlManager* achievement_xml_mgr;
AdventureXmlManager* adventure_xml_mgr;
AdventureSelectXmlManager* adventure_select_xml_mgr;
AdventureItemXmlManager* adventure_item_xml_mgr;
ArenaManager* arena_mgr;
ArenaAttrXmlManager* arena_attr_xml_mgr;
ArenaHeroXmlManager* arena_hero_xml_mgr;
ArenaBonusXmlManager* arena_bonus_xml_mgr;
ArenaLevelAttrXmlManager* arena_level_attr_xml_mgr;
BattleCacheMap* g_battle_cache_map;
BtlSoulXmlManager* btl_soul_xml_mgr;
BtlSoulLevelXmlManager* btl_soul_level_xml_mgr;
DivineItemXmlManager* divine_item_xml_mgr;
CommonFightXmlManager* common_fight_xml_mgr;
CommonFightDropXmlManager* common_fight_drop_xml_mgr;
EquipmentXmlManager* equip_xml_mgr;
EquipRefiningXmlManager* equip_refining_xml_mgr;
EquipCompoundXmlManager* equip_compound_xml_mgr;
EquipLevelXmlManager* equip_level_xml_mgr;
NickXmlManager* nick_xml_mgr;
GuildManager* guild_mgr;
HeroXmlManager* hero_xml_mgr;
HeroRankXmlManager* hero_rank_xml_mgr;
HeroRankStuffXmlManager* hero_rank_stuff_xml_mgr;
HeroLevelAttrXmlManager* hero_level_attr_xml_mgr;
LevelXmlManager* level_xml_mgr;
HeroHonorXmlManager* hero_honor_xml_mgr;
HeroHonorExpXmlManager* hero_honor_exp_xml_mgr;
HeroTitleXmlManager* hero_title_xml_mgr;
HorseAttrXmlManager* horse_attr_xml_mgr;
HorseExpXmlManager* horse_exp_xml_mgr;
HorseEquipXmlManager* horse_equip_xml_mgr;
InstanceXmlManager* instance_xml_mgr;
//TroopXmlManager* troop_xml_mgr;
MonsterXmlManager* monster_xml_mgr;
InstanceDropXmlManager* instance_drop_xml_mgr;
InstanceChapterXmlManager* instance_chapter_xml_mgr;
InstanceBagXmlManager* instance_bag_xml_mgr;
InternalAffairsXmlManager* internal_affairs_xml_mgr;
InternalAffairsRewardXmlManager* internal_affairs_reward_xml_mgr;
InternalAffairsLevelXmlManager* internal_affairs_level_xml_mgr;
ItemsXmlManager* items_xml_mgr;
HeroRankItemXmlManager* hero_rank_item_xml_mgr;
ItemPieceXmlManager* item_piece_xml_mgr;
RandomItemXmlManager* random_item_xml_mgr;
LuaScriptManage* lua_script_mgr;
PlayerManager* g_player_mgr;
RoleSkillXmlManager* role_skill_xml_mgr;
Redis* redis_mgr;
ResXmlManage* res_xml_mgr;
ShopXmlManager* shop_xml_mgr;
ItemShopXmlManager* item_shop_xml_mgr;
SkillXmlManager* skill_xml_mgr;
SkillEffectXmlManager* skill_effect_xml_mgr;
SkillLevelupGoldsXmlManager* skill_levelup_golds_xml_mgr;
SoldierXmlManager* soldier_xml_mgr;
SoldierRankXmlManager* soldier_rank_xml_mgr;
SoldierStarXmlManager* soldier_star_xml_mgr;
SoldierTrainCostXmlManager* soldier_train_cost_xml_mgr;
SoldierLevelAttrXmlManager* soldier_level_attr_xml_mgr;
HeroTalentXmlManager* hero_talent_xml_mgr;
SoldierTalentXmlManager* soldier_talent_xml_mgr;
TaskXmlManager* task_xml_mgr;
TenEvenDrawGoldsXmlManager* ten_even_draw_golds_xml_mgr;
TenEvenDrawDiamondXmlManager* ten_even_draw_diamond_xml_mgr;
TenEvenDrawDiamondSpecialXmlManager* ten_even_draw_diamond_special_xml_mgr;
TimeStampXmlManage* time_stamp_xml_info;
Timer* timer_mgr;
TreasureAttrXmlManager* treasure_attr_xml_mgr;
TreasureHeroXmlManager* treasure_hero_xml_mgr;
TreasureRewardXmlManager* treasure_reward_xml_mgr;
TrialTowerRewardXmlManager* trial_tower_reward_xml_mgr;
Utils* utils_mgr; 
VipXmlManager* vip_xml_mgr;

/*! 全局变量-服务器ID */
uint32_t g_server_id = -1;
int alarm_fd = -1;
int dbroute_fd = -1;
struct log_list_t g_log_list;
stat_log_list_t g_stat_log_list;
int stat_svr_fd = -1;
char *stat_file;
int switch_fd = -1;

pthread_t stat_tid;
pthread_t log_tid;


