ifndef COMPILER
    COMPILER = clang++
endif

FLAGS = -std=c++11 -Wall -Wextra -Wpedantic $(EXTRAFLAGS) -g
LIBS = -lIce -lIceUtil -lpthread $(EXTRALIBS)
SRCS = Portal.cpp Server.cpp Client.cpp StreamServer.cpp

OBJS = $(patsubst %.cpp,%.o,$(SRCS))

EXECS = portal server client

SLICE = StreamServer.h StreamServer.cpp

all: $(EXECS)

$(SLICE): StreamServer.ice
	slice2cpp -I . StreamServer.ice

%.o: %.cpp
	$(COMPILER) $(FLAGS) -I . -c -o $@ $+

portal: StreamServer.o Portal.o
	$(COMPILER) $(FLAGS) -o portal Portal.o StreamServer.o $(LIBS)

server: StreamServer.o Server.o
	$(COMPILER) $(FLAGS) -o server Server.o StreamServer.o $(LIBS)

client: StreamServer.o Client.o
	$(COMPILER) $(FLAGS) -o client Client.o StreamServer.o $(LIBS)

clean:
	rm -rf *.o StreamServer.h StreamServer.cpp $(EXECS)

test:
	./test.sh
