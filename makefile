all: server client
server: server.cpp
	g++ -g3 ./server.cpp -o server
client: client.cpp
	g++ ./client.cpp -o client