CC=g++
CFLAGS=-std=c++20 -Iinclude -c -Wall
LDFLAGS=
SOURCES=src/baseclientserver.cpp src/server.cpp src/main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=tcpudpserver

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@