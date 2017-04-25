CXXFLAGS += -std=c++11 -w -ltasn1 -g
LDFLAGS += -lsqlite3 -lpthread
BIN=./bin
SRC=./src
BUILD=./build
LIB=./lib

.PHONY: all clean hash-library debug

all: server client

server: $(BIN)/server.o
	$(CXX) -o bin/server.o src/server.cpp $(CXXFLAGS) $(LDFLAGS)

client: $(BIN)/client.o client-commands
	$(CXX) -o $(BIN)/client.o $(SRC)/client.cpp $(BUILD)/client-commands.o build/sha256.o $(CXXFLAGS)

client-commands: $(BUILD)/client-commands.o hash-library
	$(CXX) -o $(BUILD)/client-commands.o -c $(SRC)/client-commands.cpp $(CXXFLAGS)

hash-library: $(BUILD)/sha256.o
	$(CXX) -o $(BUILD)/sha256.o -c lib/hash-library/sha256.cpp $(CXXFLAGS)

debug:
	$(CXX) -o $(BIN)/server.o $(SRC)/server.cpp $(CXXFLAGS) -g

clean:
	$(RM) $(BIN)/* $(BUILD)/*
