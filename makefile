all: server client
server: server.cpp
	g++ -Werror -Wall -g3 ./server.cpp -o server
client: client.cpp
	g++ -Werror -Wall -g3 ./client.cpp -o client