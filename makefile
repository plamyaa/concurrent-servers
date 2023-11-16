CC = gcc

CCFLAGS = -g -pthread

LDFLAGS = -lpthread -pthread

BUILDDIR = build
EXECUTABLES = $(BUILDDIR)/threaded-server

SOURCES = utils.c threaded-server.c
OBJECTS = $(patsubst %.c, $(BUILDDIR)/%.o, $(SOURCES))

$(EXECUTABLES): $(OBJECTS) | $(BUILDDIR)
		$(CC) $(CCFLAGS) $^ -o $@ $(LDFLAGS)
		rm -f $(BUILDDIR)/*.o

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
		$(CC) $(CCFLAGS) -c $< -o $@

$(BUILDDIR):
		mkdir -p $(BUILDDIR)

clean:
		rm -f $(EXECUTABLES) $(BUILDDIR)/*.o

.PHONY: clean