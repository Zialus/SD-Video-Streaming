COMPILER = clang++
FLAGS = -std=c++11

all: portal

slice:
	slice2cpp -I . StreamServer.ice

objects: slice
	$(COMPILER) $(FLAGS) -I . -c Portal.cpp StreamServer.cpp

portal: objects
	$(COMPILER) $(FLAGS) -o portal Portal.o StreamServer.o -lIce -lIceUtil

clean:
	rm *.o StreamServer.h StreamServer.cpp portal
