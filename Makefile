CC = gcc
CFLAGS = -I include -Wall

src = $(wildcard src/*.c)
obj = $(patsubst src/%.c, build/%.o, $(src))
headers = $(wildcard include/*.h)

src_rout = $(wildcard routing/*.c)
rout = $(patsubst routing/%.c, build/%.o, $(src_rout))


routing_d: $(rout)
	$(CC) $(CFLAGS) $(rout) -o routingd

build/%.o: routing/%.c ${headers}
	$(CC) $(CFLAGS) -c $< -o $@

mipd: $(obj)
	$(CC) $(CFLAGS) $(obj) -o mipd
ping_client:
	$(CC) $(CFLAGS) ping/ping_client.c -o ping_client

ping_server:
	$(CC) $(CFLAGS) ping/ping_server.c -o ping_server
#routing_d:
#	$(CC) $(CFALGS) routing/routingd.c -o routingd

build/%.o: src/%.c ${headers}
	$(CC) $(CFLAGS) -c $< -o $@



all: ping_client ping_server mipd routing_d install

install:
	mkdir -p bin && mv mipd bin/ && mv ping_client bin/ && mv ping_server bin/ && mv routingd bin/

clean:
	rm -r build/*.o bin/
