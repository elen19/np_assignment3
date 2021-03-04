CC = gcc
CC_FLAGS = -w -g



all: test client server


main.o: main.cpp
	$(CXX) -Wall -c main.cpp -I.

client.o: client.cpp
	$(CXX) -Wall -c client.cpp -I.
	
server.o: server.cpp
	$(CXX) -Wall -c server.cpp -I.
	
test: main.o
	$(CXX) -I./ -Wall -lncurses  -o test main.o 


client: client.o
	$(CXX) -I./ -Wall -o cchat client.o

server: server.o
	$(CXX) -I./ -Wall -o cserverd server.o


clean:
	rm *.o test cserverd cchat
