CC = gcc

CCFLAGS = -g -pthread

LDFLAGS = -lpthread -pthread

BUILDDIR = build
EXECUTABLES = \
		utils.c\
		threaded-server.c \
		blocking-listener.c

all: $(EXECUTABLES)

threaded-server: utils.c threaded-server.c
	$(CC) $(CCFLAGS) $^ -o build/$@ $(LDFLAGS)

blocking-listener: utils.c blocking-listener.c
	$(CC) $(CCFLAGS) $^ -o build/$@ $(LDFLAGS)

clean:
		rm -f $(EXECUTABLES) *.o

.PHONY: clean
