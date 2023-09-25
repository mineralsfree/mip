CC = gcc
CFLAGS = -I include -Wall

src = $(wildcard src/*.c)
obj = $(patsubst src/%.c, build/%.o, $(src))
headers = $(wildcard include/*.h)

mipd: $(obj)
	$(CC) $(CFLAGS) $(obj) -o mipd

build/%.o: src/%.c ${headers}
	$(CC) $(CFLAGS) -c $< -o $@

all: mipd install

install:
	mkdir -p bin && mv mipd bin/

clean:
	rm -r build/*.o bin/
