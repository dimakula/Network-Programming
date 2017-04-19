CXXFLAGS += -std=c++11 -w -ltasn1
LDFLAGS += -lsqlite3 -lpthread

.PHONY: all clean hash-library debug

all: server client

server:
	$(CXX) -o bin/server.o src/server.cpp $(CXXFLAGS) $(LDFLAGS)

client: client-commands
	$(CXX) -o bin/client.o src/client.cpp build/client-commands.o build/sha256.o $(CXXFLAGS)

client-commands: hash-library
	$(CXX) -o build/client-commands.o -c src/client-commands.cpp $(CXXFLAGS)

hash-library:
	$(CXX) -o build/sha256.o -c lib/hash-library/sha256.cpp $(CXXFLAGS)

debug:
	$(CXX) -o bin/server.o src/server.cpp $(CXXFLAGS) -g

clean:
	$(RM) bin/* build/*
