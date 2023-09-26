CC = gcc
CFLAGS = -I include -Wall

src = $(wildcard src/*.c)
obj = $(patsubst src/%.c, build/%.o, $(src))
headers = $(wildcard include/*.h)

mipd: $(obj)
	$(CC) $(CFLAGS) $(obj) -o mipd
ping_client:
	$(CC) $(CFLAGS) ping/ping_client.c -o ping_client

ping_server:
	$(CC) $(CFLAGS) ping/ping_server.c -o ping_server

build/%.o: src/%.c ${headers}
	$(CC) $(CFLAGS) -c $< -o $@

all: ping_client ping_server mipd install

install:
	mkdir -p bin && mv mipd bin/ && mv ping_client bin/ && mv ping_server bin/

clean:
	rm -r build/*.o bin/
