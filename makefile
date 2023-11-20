CC = gcc
CCFLAGS = -g -pthread
LDFLAGS = -lpthread -pthread
LDLIBUV = `pkg-config --cflags --libs libuv`

EXECUTABLES = \
	utils.c \
	threaded-server.c \
	blocking-listener.c \
	nonblocking-listener.c \
	select-server.c \
	uv-server.c

all: build $(EXECUTABLES)

build:
	mkdir -p build

threaded-server: utils.c threaded-server.c | build
	$(CC) $(CCFLAGS) $^ -o build/$@ $(LDFLAGS)

blocking-listener: utils.c blocking-listener.c | build
	$(CC) $(CCFLAGS) $^ -o build/$@ $(LDFLAGS)

nonblocking-listener: utils.c nonblocking-listener.c | build
	$(CC) $(CCFLAGS) $^ -o build/$@ $(LDFLAGS)

select-server: utils.c select-server.c | build
	$(CC) $(CCFLAGS) $^ -o build/$@ $(LDFLAGS)

uv-server: utils.c uv-server.c | build
	$(CC) $(CCFLAGS) $^ -o build/$@ $(LDFLAGS) $(LDLIBUV)

clean:
	rm -f build/$(EXECUTABLES) *.o

.PHONY: clean
