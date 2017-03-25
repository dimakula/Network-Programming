CC=g++
FLAGS=-lsqlite3 -w -std=c++11 -lpthread

all: server client

server:
	$(CC) -o server server.cpp $(FLAGS)

client: client-commands.cpp
	$(CC) -o client client.cpp client-commands.cpp hash-library/sha256.cpp $(FLAGS)

Debug:
	$(CC) -o server server.cpp $(FLAGS) -g
