/**
 *============================================================
 *
 * @file  dll_interface.cpp
 *
 * @brief  AsynServ的接口函数，AsynServ通过加载SO,处理具体的逻辑
 * compiler   gcc4.1.2
 * 
 * platform   Linux
 *
 *============================================================
 */

#include <ctime>
#include <string>

extern "C" {
#include <arpa/inet.h>
#include <libcommon/conf_parser/config.h>
#include <libcommon/list.h>
#include <libcommon/log.h>
#include <libcommon/time/time.h>
#include <libcommon/time/timer.h>
#include <pthread.h>
}
#include <asyn_serv/net_if.hpp>

#include <libproject/utils/strings.hpp> //bin2hex

#include "cli_dispatch.hpp"
#include "dbroute.hpp"
#include "mcast_proto.hpp"
#include "player.hpp"
#include "hero.hpp"
#include "equipment.hpp"
#include "item.hpp"
#include "switch.hpp"
#include "skill.hpp"
#include "talent.hpp"
#include "instance.hpp"
#include "wheel_timer.h"
#include "stat_log.hpp"
#include "log_thread.hpp"
#include "timer.hpp"
#include "soldier.hpp"
#include "btl_soul.hpp"
#include "ten_even_draw.hpp"
#include "horse.hpp"
#include "arena.hpp"
#include "treasure_risk.hpp"
#include "redis.hpp"
#include "task.hpp"
#include "shop.hpp"
#include "general.hpp"
#include "achievement.hpp"
#include "internal_affairs.hpp"
#include "common_fight.hpp"
#include "vip.hpp"
#include "trial_tower.hpp"
#include "alarm.hpp"
#include "lua_script_manage.hpp"
#include "global_data.hpp"


static void init_global_members()
{
	//ev_mgr = new EventMgr();
	achievement_xml_mgr = new AchievementXmlManager();
	adventure_xml_mgr = new AdventureXmlManager();
	adventure_select_xml_mgr = new AdventureSelectXmlManager();
	adventure_item_xml_mgr = new AdventureItemXmlManager();
	arena_mgr = new ArenaManager();
	arena_attr_xml_mgr = new ArenaAttrXmlManager();
	arena_hero_xml_mgr = new ArenaHeroXmlManager();
	arena_bonus_xml_mgr = new ArenaBonusXmlManager();
	arena_level_attr_xml_mgr = new ArenaLevelAttrXmlManager();
	g_battle_cache_map = new BattleCacheMap();
	btl_soul_xml_mgr = new BtlSoulXmlManager();
	btl_soul_level_xml_mgr = new BtlSoulLevelXmlManager();
	divine_item_xml_mgr = new DivineItemXmlManager();
	common_fight_xml_mgr = new CommonFightXmlManager();
	common_fight_drop_xml_mgr = new CommonFightDropXmlManager();
	equip_xml_mgr = new EquipmentXmlManager();
	equip_refining_xml_mgr = new EquipRefiningXmlManager();
	equip_compound_xml_mgr = new EquipCompoundXmlManager();
	equip_level_xml_mgr = new EquipLevelXmlManager();
	nick_xml_mgr = new NickXmlManager();
	guild_mgr = new GuildManager();
	hero_xml_mgr = new HeroXmlManager();
	hero_rank_xml_mgr = new HeroRankXmlManager();
	hero_rank_stuff_xml_mgr = new HeroRankStuffXmlManager();
	hero_level_attr_xml_mgr = new HeroLevelAttrXmlManager();
	level_xml_mgr = new LevelXmlManager();
	hero_honor_xml_mgr = new HeroHonorXmlManager();
	hero_honor_exp_xml_mgr = new HeroHonorExpXmlManager();
	hero_title_xml_mgr = new HeroTitleXmlManager();
	horse_attr_xml_mgr = new HorseAttrXmlManager();
	horse_exp_xml_mgr = new HorseExpXmlManager();
	horse_equip_xml_mgr = new HorseEquipXmlManager();
	instance_xml_mgr = new InstanceXmlManager();
	//troop_xml_mgr = new TroopXmlManager();
	monster_xml_mgr = new MonsterXmlManager();
	instance_drop_xml_mgr = new InstanceDropXmlManager();
	instance_chapter_xml_mgr = new InstanceChapterXmlManager();
	instance_bag_xml_mgr = new InstanceBagXmlManager();
	internal_affairs_xml_mgr = new InternalAffairsXmlManager();
	internal_affairs_reward_xml_mgr = new InternalAffairsRewardXmlManager();
	internal_affairs_level_xml_mgr = new InternalAffairsLevelXmlManager();
	items_xml_mgr = new ItemsXmlManager();
	hero_rank_item_xml_mgr = new HeroRankItemXmlManager();
	item_piece_xml_mgr = new ItemPieceXmlManager();
	random_item_xml_mgr = new RandomItemXmlManager();
	lua_script_mgr = new LuaScriptManage();
	g_player_mgr = new PlayerManager();
	role_skill_xml_mgr = new RoleSkillXmlManager();
	redis_mgr = new Redis();
	res_xml_mgr = new ResXmlManage();
	shop_xml_mgr = new ShopXmlManager();
	item_shop_xml_mgr = new ItemShopXmlManager();
	skill_xml_mgr = new SkillXmlManager();
	skill_effect_xml_mgr = new SkillEffectXmlManager();
	skill_levelup_golds_xml_mgr = new SkillLevelupGoldsXmlManager();
	soldier_xml_mgr = new SoldierXmlManager();
	soldier_rank_xml_mgr = new SoldierRankXmlManager();
	soldier_star_xml_mgr = new SoldierStarXmlManager();
	soldier_train_cost_xml_mgr = new SoldierTrainCostXmlManager();
	soldier_level_attr_xml_mgr = new SoldierLevelAttrXmlManager();
	hero_talent_xml_mgr = new HeroTalentXmlManager();
	soldier_talent_xml_mgr = new SoldierTalentXmlManager();
	task_xml_mgr = new TaskXmlManager();
	ten_even_draw_golds_xml_mgr = new TenEvenDrawGoldsXmlManager();
	ten_even_draw_diamond_xml_mgr = new TenEvenDrawDiamondXmlManager();
	ten_even_draw_diamond_special_xml_mgr = new TenEvenDrawDiamondSpecialXmlManager();
	time_stamp_xml_info = new TimeStampXmlManage();
	timer_mgr = new Timer(); 
	treasure_attr_xml_mgr = new TreasureAttrXmlManager();
	treasure_hero_xml_mgr = new TreasureHeroXmlManager();
	treasure_reward_xml_mgr = new TreasureRewardXmlManager();
	trial_tower_reward_xml_mgr = new TrialTowerRewardXmlManager();
	vip_xml_mgr = new VipXmlManager();
    http_request = new HttpRequest();
}

static int init_proto_handle_funs()
{
	init_cli_handle_funs();
	//init_switch_handle_funs();
	init_db_handle_funs();
	init_redis_handle_funs();
	return 0;	
}

/**
  * @brief Initialize service
  *
  */
extern "C" int init_service(int isparent)
{
	if (!isparent) {
		KDEBUG_LOG(0, "\n====================== SERVER START ====================");

		init_global_members();
		//init_cli_handle_funs();
		//init_switch_handle_funs();
		//init_db_handle_funs();
		//init_redis_handle_funs();
		init_proto_handle_funs();
		
		/*
		init_btl_handle_funs();
		init_btlsw_handle_funs();
		init_home_handle_funs();
		g_announce.init_good_news_info();
		*/

		g_server_id = get_server_id() - 1;
		setup_timer(&timers_list, tcfs, 1);
		init_timer_callback_type();
		struct timeval stv;
		gettimeofday(&stv,NULL); 
		srand(stv.tv_sec);

		/*配置文件加载*/
		if (hero_xml_mgr->read_from_xml("./conf/hero.xml") == -1 
				|| hero_rank_xml_mgr->read_from_xml("./conf/hero_rank_attr.xml") == -1
				|| hero_rank_stuff_xml_mgr->read_from_xml("./conf/hero_rank_stuff.xml") == -1
				|| items_xml_mgr->read_from_xml("./conf/item.xml") == -1
				|| hero_rank_item_xml_mgr->read_from_xml("./conf/hero_rank_item.xml") == -1
				|| item_piece_xml_mgr->read_from_xml("./conf/item_piece.xml") == -1
				|| random_item_xml_mgr->read_from_xml("./conf/random_item.xml") == -1
				|| equip_xml_mgr->read_from_xml("./conf/equipment.xml") == -1
				|| equip_refining_xml_mgr->read_from_xml("./conf/equip_refining.xml") == -1
				|| equip_compound_xml_mgr->read_from_xml("./conf/equip_compound.xml") == -1
				|| equip_level_xml_mgr->read_from_xml("./conf/equip_level.xml") == -1
				|| hero_talent_xml_mgr->read_from_xml("./conf/hero_talent.xml") == -1
				|| soldier_talent_xml_mgr->read_from_xml("./conf/soldier_talent.xml") == -1
				|| skill_xml_mgr->read_from_xml("./conf/skill.xml") == -1
				|| skill_effect_xml_mgr->read_from_xml("./conf/skill_effect.xml") == -1
				|| skill_levelup_golds_xml_mgr->read_from_xml("./conf/skill_levelup_golds.xml") == -1
				|| soldier_xml_mgr->read_from_xml("./conf/soldier.xml") == -1
				|| soldier_rank_xml_mgr->read_from_xml("./conf/soldier_rank.xml") == -1
				|| soldier_star_xml_mgr->read_from_xml("./conf/soldier_star.xml") == -1
				|| soldier_train_cost_xml_mgr->read_from_xml("./conf/soldier_train_cost.xml") == -1
				|| soldier_level_attr_xml_mgr->read_from_xml("./conf/soldier_level_attr.xml") == -1
				|| instance_xml_mgr->read_from_xml("./conf/instance.xml") == -1
				|| instance_drop_xml_mgr->read_from_xml("./conf/drop.xml") == -1
				|| instance_chapter_xml_mgr->read_from_xml("./conf/instance_chapter.xml") == -1
				|| instance_bag_xml_mgr->read_from_xml("./conf/instance_bag.xml") == -1
				|| level_xml_mgr->read_from_xml("./conf/level.xml") == -1
				|| hero_level_attr_xml_mgr->read_from_xml("./conf/hero_level_attr.xml") == -1
				|| btl_soul_xml_mgr->read_from_xml("./conf/btl_soul.xml") == -1
				|| btl_soul_level_xml_mgr->read_from_xml("./conf/btl_soul_level.xml") == -1
				|| divine_item_xml_mgr->read_from_xml("./conf/divine_item.xml") == -1
				|| ten_even_draw_golds_xml_mgr->read_from_xml("./conf/ten_draw_100.xml") == -1
				|| ten_even_draw_diamond_xml_mgr->read_from_xml("./conf/ten_draw_280.xml") == -1
				|| ten_even_draw_diamond_special_xml_mgr->read_from_xml("./conf/ten_draw_special.xml") == -1
				|| hero_honor_xml_mgr->read_from_xml("./conf/honor_attr.xml") == -1
				|| hero_honor_exp_xml_mgr->read_from_xml("./conf/honor_exp.xml") == -1
				|| horse_attr_xml_mgr->read_from_xml("./conf/horse_attr.xml") == -1
				|| horse_exp_xml_mgr->read_from_xml("./conf/horse_exp.xml") == -1
				|| horse_equip_xml_mgr->read_from_xml("./conf/horse_equip.xml") == -1
				|| arena_attr_xml_mgr->read_from_xml("./conf/arena_attr.xml") == -1
				|| arena_hero_xml_mgr->read_from_xml("./conf/arena_hero.xml") == -1
				|| arena_bonus_xml_mgr->read_from_xml("./conf/arena_bonus.xml") == -1
				|| arena_level_attr_xml_mgr->read_from_xml("./conf/arena_level_attr.xml") == -1
				|| treasure_attr_xml_mgr->read_from_xml("./conf/treasure_attr.xml") == -1
				|| treasure_hero_xml_mgr->read_from_xml("./conf/treasure_hero.xml") == -1
				|| treasure_reward_xml_mgr->read_from_xml("./conf/treasure_reward.xml") == -1
				|| task_xml_mgr->read_from_xml("./conf/task.xml") == -1
				|| shop_xml_mgr->read_from_xml("./conf/shop.xml") == -1
				|| hero_title_xml_mgr->read_from_xml("./conf/hero_title.xml") == -1
				|| adventure_xml_mgr->read_from_xml("./conf/adventure.xml") == -1
				|| adventure_select_xml_mgr->read_from_xml("./conf/adventure_select.xml") == -1
				|| adventure_item_xml_mgr->read_from_xml("./conf/adventure_item.xml") == -1
				|| nick_xml_mgr->read_from_xml("./conf/nick.xml") == -1
				|| achievement_xml_mgr->read_from_xml("./conf/achievement.xml") == -1
				|| internal_affairs_xml_mgr->read_from_xml("./conf/affairs.xml") == -1
				|| internal_affairs_reward_xml_mgr->read_from_xml("./conf/affairs_reward.xml") == -1
				|| internal_affairs_level_xml_mgr->read_from_xml("./conf/affairs_level.xml") == -1
				|| common_fight_xml_mgr->read_from_xml("./conf/common_fight.xml") == -1
				|| common_fight_drop_xml_mgr->read_from_xml("./conf/common_fight_drop.xml") == -1
				|| item_shop_xml_mgr->read_from_xml("./conf/item_shop.xml") == -1
				|| role_skill_xml_mgr->read_from_xml("./conf/role_skill.xml") == -1
				|| vip_xml_mgr->read_from_xml("./conf/vip.xml") == -1
				|| trial_tower_reward_xml_mgr->read_from_xml("./conf/trial_tower_reward.xml") == -1
				) {
			return -1;
		}
		
		connect_to_switch();
		connect_to_switch_timely(NULL, NULL);

		//TODO only test!
		connect_to_alarm();
		//send_msg_to_alarm("Hi Frankie, I am a alarm!");
		//send_msg_to_alarm("验证码:9836");

		connect_to_stat_svr();
		stat_file = config_get_strval("statistic_file");

		//启动定时器
		init_global_timer();

		//统计线程
		init_stat_log_list();
		pthread_create(&stat_tid, 0, &write_stat_log, 0);

		//日志线程
		init_log_thread_list();
		pthread_create(&log_tid, 0, &write_log_thread, 0);

        //curl线程
        pthread_create(&http_tid, 0, &deal_request_thread, 0);

		//加载lua脚本
		lua_script_mgr->LoadLuaFile("./lualib/");
	} else {
	
	}
	return 0;
}

/**
  * @brief Finalize service
  *
  */
extern "C" int fini_service(int isparent)
{
	if (!isparent) {
		destroy_timer();
	}
	return 0;
}

/**
  * @brief Process events such as timers and signals
  *
  */
extern "C" void proc_events()
{
	handle_timer();
	//wheel_update_timer();
	//ev_mgr.process_events();
}

/**
  * @brief Return length of the receiving package
  *
  */
extern "C" int get_pkg_len(int fd, const void *avail_data, int avail_len, int isparent)
{
	static char request[]  = "<policy-file-request/>";
	static char response[] = "<?xml version=\"1.0\"?>"
								"<!DOCTYPE cross-domain-policy>"
								"<cross-domain-policy>"
								"<allow-access-from domain=\"*\" to-ports=\"*\" />"
								"</cross-domain-policy>";

	if (avail_len < 4) {
		return 0;
	}

	int len = -1;
  	if (isparent) {
		// the client requests for a socket policy file
		if ((avail_len == sizeof(request)) && !memcmp(avail_data, request, sizeof(request))) {
			net_send(fd, response, sizeof(response));
			//TRACE_LOG("Policy Req [%s] Received, Rsp [%s] Sent", request, response);
			return 0;
		}

		cli_proto_head_t *pkg = (cli_proto_head_t *)avail_data;

		len = (pkg->len);
		if ((len > cli_proto_max_len) || (len < static_cast<int>(sizeof(cli_proto_head_t)))) {
			ERROR_LOG("C->S INVALID [LEN = %d] FROM [FD = %d] %lu", len, fd, sizeof(cli_proto_head_t));
			return -1;
		}
	} else {
		len = *(uint32_t*)(avail_data);
		/* DB，BATTLE...都经过此处理 */
		/*if ((len < static_cast<int>(sizeof(db_proto_head_t)))) {
			ERROR_LOG("DB or BATTLE SERVER ... invalid len=%d from fd=%d", len, fd);
			return -1;
		}*/
	}	
	return len;
}

/**
  * @brief Process packages from clients
  *
  */
extern "C" int proc_pkg_from_client(void* data, int len, fdsession_t* fdsess)
{
	/* 返回非零，断开FD的连接*/
	//KDEBUG_LOG(0,"fd=%d",fdsess->fd);
	return dispatch(data, fdsess);
}

/**
  * @brief Process packages from servers
  *
  */
extern "C" void proc_pkg_from_serv(int fd, void* data, int len)
{
	if (fd == dbroute_fd) {
		handle_db_return(reinterpret_cast<db_proto_head_t*>(data), len);
	} else if (fd == switch_fd) {
		handle_switch_return(reinterpret_cast<sw_proto_head_t*>(data), len);
	}
	/*
	int btlsvr_index = btlsvr_xml_info.get_btlsvr_index_by_fd(fd); 
	int homesvr_index = homesvr_xml_info.get_homesvr_index_by_fd(fd); 
	if (btlsvr_index >= 0) {
	    handle_btl_return(reinterpret_cast<btl_proto_head_t*>(data), len);		    
	} else if (homesvr_index >= 0) {
	    handle_home_return(reinterpret_cast<home_proto_head_t*>(data), len);		    
	} else if (fd == btlsw_fd) {
	    handle_btlsw_return(reinterpret_cast<btl_proto_head_t*>(data), len);		    
	} else if (fd == switch_fd) {
		handle_switch_return(reinterpret_cast<sw_proto_head_t*>(data), len);
	}*/
}

/**
  * @brief Called each time on client connection closed
  *
  */
extern "C" void on_client_conn_closed(int fd)
{
	Player *p = g_player_mgr->get_player_by_fd(fd);
	if (p && p->fdsess->fd == fd) {
		g_player_mgr->del_player(p);
	}

}

/**
  * @brief Called each time on close of the fds created by the child process
  *
  */
extern "C" void on_fd_closed(int fd)
{
	if (fd == dbroute_fd) {
		dbroute_fd = -1; 
	} else if (fd == switch_fd) {
		switch_fd = -1; 
	}		   
}

/**
  * @brief Called to process mcast package from the address and port configured in the config file
  */
extern "C" void proc_mcast_pkg(const void* data, int len)
{
	const mcast_pkg_t* pkg = reinterpret_cast<const mcast_pkg_t*>(data);
	//KDEBUG_LOG(0, "proc_mast_pkg cmd=%u,minorcmd=%u,serverid=%u", pkg->main_cmd, pkg->minor_cmd,pkg->server_id);

	if ( pkg->server_id != get_server_id() ) {
		switch (pkg->main_cmd) {
			/*
			case mcast_talk_info: 
				Chat::chat_to_world(pkg->body, len - sizeof(mcast_pkg_t));
				break;
				*/
			default:
				//KERROR_LOG(0, "proc_mast_pkg unsurported cmd %u", pkg->main_cmd);
				break;
		}
	}
}

/**
  * @brief Called to process udp package from the address and port configured in the config file
  */
extern "C" void proc_udp_pkg(int fd, const void* avail_data, int avail_len, struct sockaddr_in *from, socklen_t fromlen)
{
	const char *data_str = (char*)avail_data;
	DEBUG_LOG("recv udp data addr[%d %d %d]", from->sin_family, from->sin_port, from->sin_addr.s_addr);
	DEBUG_LOG("recv udp data len=%d str=%.*s", avail_len, avail_len, data_str);
	sendto(fd, data_str, avail_len, 0, (struct sockaddr*)from, sizeof(*from));
}

extern "C" void before_global_reload()
{
	if (stat_tid && pthread_cancel(stat_tid) == 0) {
		DEBUG_LOG("cancel statlog pthread!");
	}
	if (log_tid && pthread_cancel(log_tid) == 0) {
		DEBUG_LOG("cancel log pthread!");
	}
	if (http_tid && pthread_cancel(http_tid) == 0) {
		DEBUG_LOG("cancel http pthread!");
	}
	DEBUG_LOG("before global reload");
}

extern "C" void reload_global_data()
{
	init_proto_handle_funs();

	setup_timer(&timers_list, tcfs, 0);
	init_timer_callback_type();
	refresh_timers_callback();

	pthread_create(&stat_tid, 0, &write_stat_log, 0);
	pthread_create(&log_tid, 0, &write_log_thread, 0);
    pthread_create(&http_tid, 0, &deal_request_thread, 0);
	DEBUG_LOG("reload global data");
}



