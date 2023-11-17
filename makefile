CC = gcc

CCFLAGS = -g -pthread

LDFLAGS = -lpthread -pthread

BUILDDIR = build
EXECUTABLES = \
		utils.c\
		threaded-server.c \
		blocking-listener.c \
		nonblocking-listener.c \
		select-server.c

all: $(EXECUTABLES)

threaded-server: utils.c threaded-server.c
		$(CC) $(CCFLAGS) $^ -o build/$@ $(LDFLAGS)

blocking-listener: utils.c blocking-listener.c
		$(CC) $(CCFLAGS) $^ -o build/$@ $(LDFLAGS)

nonblocking-listener: utils.c nonblocking-listener.c
		$(CC) $(CCFLAGS) $^ -o build/$@ $(LDFLAGS)

select-server: utils.c select-server.c
		$(CC) $(CCFLAGS) $^ -o build/$@ $(LDFLAGS)

clean:
		rm -f $(EXECUTABLES) *.o

.PHONY: clean
