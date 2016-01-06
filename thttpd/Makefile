CC = g++

CFLAGS  = -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -fPIC -D_REENTRANT -MMD -shared -std=c++11
#CFLAGS  = -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -shared -fPIC -D_REENTRANT
#CFLAGS  = -g -Wall -pipe -DNDEBUG -O2 -D_GNU_SOURCE -shared -fPIC -D_REENTRANT
LIBDIR	= -L./libjson/lib/
LIB 	= -ldl -lrt -lproject -lcommon -ljsoncpp
INCLUDE = -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include/ -I./libjson/include/

OO	= thttp.o
TARGETS = ./libthttp.so

all: $(OO) $(TARGETS)

$(TARGETS): $(OO)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBDIR) $(LIB)

%.o: %.cpp Makefile
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<
	@-mv -f $*.d .dep.$@

clean:
	rm -f *.o
	rm -f *.d
	rm -f $(TARGETS)
