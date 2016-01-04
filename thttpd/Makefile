CC = g++

CFLAGS  = -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -fPIC -D_REENTRANT -MMD -shared
#CFLAGS  = -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -shared -fPIC -D_REENTRANT
#CFLAGS  = -g -Wall -pipe -DNDEBUG -O2 -D_GNU_SOURCE -shared -fPIC -D_REENTRANT
LIB 	= -ldl -lrt -lproject -lcommon
INCLUDE = -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include/ 

OO	= thttp.o
TARGETS = ./libthttp.so

all: $(OO) $(TARGETS)

$(TARGETS):
	$(CC) $(CFLAGS) $(OO) -o $@ $(LIBDIR) $(LIB)

.cpp.o:
	$(CC)  $(CFLAGS) -c $(INCLUDE) $<

%:%.cpp
	$(CC) $(CFLAGS) -o $@ $< $(OO) $(LDFLAGS)

clean:
	rm -f *.o
	rm -f *.d
	rm -f $(TARGETS)
