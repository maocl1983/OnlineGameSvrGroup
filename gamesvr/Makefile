CC = g++
#CC = ccache distcc g++

#FLAGS  = -Wall -DDEBUG -g -fPIC -MMD  
#FLAGS  = -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -shared -fPIC -D_REENTRANT  
FLAGS  = -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -fPIC -D_REENTRANT -MMD 
ifndef RELEASE
FLAGS += -DENABLE_TRACE_LOG  
else
FLAGS += -DRELEASE_VER 
endif


SOFLAGS = -shared

INCLUDE = -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include/ -I/usr/include/libxml2
LIBPATH = -L ./proto/ -L/usr/local/lib
LIBS	= -lcommon -lproject -lxml2 -lrt -lm -lproto -llua -lpthread

BINS	= libgamesvr.so

SRCS 	= dll_interface.cpp cli_dispatch.cpp dbroute.cpp switch.cpp redis.cpp common_def.cpp timer.cpp hash.cpp utils.cpp login.cpp \
			player.cpp hero.cpp item.cpp restriction.cpp general.cpp equipment.cpp talent.cpp skill.cpp instance.cpp \
			soldier.cpp btl_soul.cpp ten_even_draw.cpp horse.cpp arena.cpp treasure_risk.cpp task.cpp shop.cpp adventure.cpp\
			achievement.cpp common_fight.cpp internal_affairs.cpp vip.cpp guild.cpp astrology.cpp trial_tower.cpp\
			general.cpp stat_log.cpp log_thread.cpp wheel_timer.cpp alarm.cpp lua_log.cpp lua_script_manage.cpp \
# ./proto/common.cpp ./proto/xseer_online.cpp ./proto/xseer_battle.cpp ./proto/xseer_db.cpp \
#		  ./proto/xseer_switch.cpp ./proto/kings_home.cpp ./proto/xseer_btlswitch.cpp ./proto/account_db.cpp \
#		  ./proto/kings_redis.cpp ./proto/pay_db.cpp 

all: $(BINS) $(E_BIN)
	



$(BINS): $(SRCS:.cpp=.o)
	$(CC)  $(FLAGS) $(SOFLAGS) -o $@ $^ $(LIBPATH) $(LIBS)
	@-rm ./bin/$(BINS)
	@-cp $(BINS) ./bin/

%.o: %.cpp Makefile  
	$(CC) $(FLAGS) $(INCLUDE) -c -o $@ $<
	@-mv -f $*.d .dep.$@

clean:
	@-rm -f *.o *.oxx *.po *.so *.d .dep.* $(BINS) $(E_BIN) 
	@-rm -f ./proto/*.o ./proto/*.d
	@-rm ./bin/*.so

-include /dev/null $(wildcard .dep.*)
