CC=g++

all:
	$(CC) -o server server.cpp

Debug:
	$(CC) -o server server.cpp -g
