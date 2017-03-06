CC=g++
FLAGS=-lsqlite3 -w

all:
	$(CC) -o server server.cpp $(FLAGS)

Debug:
	$(CC) -o server server.cpp $(FLAGS) -g
