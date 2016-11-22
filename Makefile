# COMPILER = clang++
FLAGS = -std=c++14 -Wall -Wextra -Wpedantic

SRCS = Portal.cpp Server.cpp Client.cpp StreamServer.cpp

OBJS = $(patsubst %.cpp,%.o,$(SRCS))

EXECS = portal server client

SLICE = StreamServer.h StreamServer.cpp

all: $(EXECS)

$(SLICE): StreamServer.ice
	slice2cpp -I . StreamServer.ice

%.o: %.cpp
	$(COMPILER) $(FLAGS) -I . -c -o $@ $+

portal: $(SLICE) $(OBJS)
	$(COMPILER) $(FLAGS) -o portal Portal.o StreamServer.o -lIce -lIceUtil -lpthread

server: $(SLICE) $(OBJS)
	$(COMPILER) $(FLAGS) -o server Server.o StreamServer.o -lIce -lIceUtil -lpthread

client: $(SLICE) $(OBJS)
	$(COMPILER) $(FLAGS) -o client Client.o StreamServer.o -lIce -lIceUtil -lpthread

clean:
	rm -rf *.o StreamServer.h StreamServer.cpp $(EXECS)

test:
	./test.sh
