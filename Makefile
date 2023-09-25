CC = gcc
CFLAGS = -I include -Wall

src = $(wildcard src/*.c)
obj = $(patsubst src/%.c, build/%.o, $(src))
headers = $(wildcard include/*.h)

mipd: $(obj)
	$(CC) $(CFLAGS) $(obj) -o mipd
client_ping:
	$(CC) $(CFLAGS) ping/main.c -o client_ping
build/%.o: src/%.c ${headers}
	$(CC) $(CFLAGS) -c $< -o $@

all: client_ping mipd install

install:
	mkdir -p bin && mv mipd bin/ && mv client_ping bin/

clean:
	rm -r build/*.o bin/
