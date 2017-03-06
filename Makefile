CC=g++
FLAGS=-lsqlite3 -w -std=c++11

all:
	$(CC) -o server server.cpp $(FLAGS)

Debug:
	$(CC) -o server server.cpp $(FLAGS) -g
