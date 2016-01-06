CC = g++

CFLAGS  = -Wall -pipe -DDEBUG -g -D_GNU_SOURCE -fPIC -D_REENTRANT -MMD -shared -std=c++11 -Wconversion -Wshadow -Wextra
LIB 	= 
INCLUDE = -I../include/ 

SRCS	= json_reader.cpp json_value.cpp json_writer.cpp
TARGETS = ../lib/libjsoncpp.a

all: $(TARGETS) $(E_TARGETS)

$(TARGETS): $(SRCS:.cpp=.o)
	ar rcs $@ $^ $(LIBDIR) $(LIB)

%.o: %.cpp Makefile
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<
	@-mv -f $*.d .dep.$@

clean:
	rm -f *.o
	rm -f *.d
	rm -f $(TARGETS)
