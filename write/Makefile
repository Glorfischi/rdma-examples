APPS = server client

LDFLAGS =  -libverbs -lpthread #-ldl
CFLAGS += -Wall -std=c++11 -I./  

all: CFLAGS += -O2
all: ${APPS}

 
server: clean
	g++ server.cpp $(CFLAGS) $(LDFLAGS) -lrdmacm   -o server

client: clean
	g++ client.cpp $(CFLAGS) $(LDFLAGS) -lrdmacm   -o client


debug: CFLAGS += -DDEBUG -g -O0
debug: ${APPS}

.DELETE_ON_ERROR:
.PHONY: all clean
