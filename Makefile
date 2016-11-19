# CXX = clang++
CFLAGS = -std=c++14

all: portal

slice:
	slice2cpp -I . StreamServer.ice

objects: slice
	$(CXX) $(CFLAGS) -I . -c Portal.cpp StreamServer.cpp

portal: objects
	$(CXX) $(CFLAGS) -o portal Portal.o StreamServer.o -lIce -lIceUtil

clean:
	rm *.o StreamServer.h StreamServer.cpp portal
