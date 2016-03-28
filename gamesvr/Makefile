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
LIBS	= -lcommon -lproject -lxml2 -lrt -lm -lproto -llua -lpthread -lcurl

DATA_BINS	= libgamesvr_data.so
BINS	= libgamesvr.so

DATA_SRCS = global_data.cpp
SRCS 	= dll_interface.cpp cli_dispatch.cpp dbroute.cpp switch.cpp redis.cpp common_def.cpp timer.cpp hash.cpp utils.cpp login.cpp \
			player.cpp hero.cpp item.cpp restriction.cpp general.cpp equipment.cpp talent.cpp skill.cpp instance.cpp \
			soldier.cpp btl_soul.cpp ten_even_draw.cpp horse.cpp arena.cpp treasure_risk.cpp task.cpp shop.cpp adventure.cpp\
			achievement.cpp common_fight.cpp internal_affairs.cpp vip.cpp guild.cpp astrology.cpp trial_tower.cpp\
			general.cpp stat_log.cpp log_thread.cpp wheel_timer.cpp alarm.cpp lua_log.cpp lua_script_manage.cpp http_request.cpp \
# 		  ./proto/common.cpp ./proto/xseer_online.cpp ./proto/xseer_battle.cpp ./proto/xseer_db.cpp \
#		  ./proto/xseer_switch.cpp ./proto/kings_home.cpp ./proto/xseer_btlswitch.cpp ./proto/account_db.cpp \
#		  ./proto/kings_redis.cpp ./proto/pay_db.cpp 

PCH_X_H = common_def.hpp stdafx.hpp
PCH_X = $(patsubst %hpp,%hpp.gch,$(PCH_X_H))

.PHONY : all
all: $(BINS) $(DATA_BINS)
	



$(PCH_X): %.hpp.gch: %.hpp
	$(CC) $(FLAGS) $(INCLUDE) $<

$(BINS): $(SRCS:.cpp=.o)
	$(CC)  $(FLAGS) $(SOFLAGS) -o $@ $^ $(LIBPATH) $(LIBS)
	@-rm ./bin/$(BINS)
	@-cp $(BINS) ./bin/

$(DATA_BINS): $(DATA_SRCS:.cpp=.o)
	$(CC)  $(FLAGS) $(SOFLAGS) -o $@ $^ $(LIBPATH) $(LIBS)
	@-rm ./bin/$(BINS)
	@-cp $(BINS) ./bin/

%.o: %.cpp $(PCH_X)
	$(CC) $(FLAGS) $(INCLUDE) -c -o $@ $<
	@-mv -f $*.d .dep.$@

clean:
	@-rm -f *.o *.oxx *.po *.so *.d .dep.* $(BINS) $(E_BIN) $(PCH_X)
	@-rm -f ./proto/*.o ./proto/*.d
	@-rm ./bin/*.so

-include /dev/null $(wildcard .dep.*)
