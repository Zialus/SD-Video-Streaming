# COMPILER = clang++
FLAGS = -std=c++11

all: portal server

slice: StreamServer.ice
	slice2cpp -I . StreamServer.ice

objects: slice
	$(COMPILER) $(FLAGS) -I . -c Portal.cpp Server.cpp StreamServer.cpp

portal: objects
	$(COMPILER) $(FLAGS) -o portal Portal.o StreamServer.o -lIce -lIceUtil -lpthread

server: objects
	$(COMPILER) $(FLAGS) -o server Server.o StreamServer.o -lIce -lIceUtil -lpthread

clean:
	rm *.o StreamServer.h StreamServer.cpp portal server
