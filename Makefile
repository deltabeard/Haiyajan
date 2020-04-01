OPT ?= -g3 -Og
CFLAGS := $(OPT) -std=c99 -Wall -Wextra -Werror -I./inc \
	$(shell sdl2-config --cflags)
	LDLIBS := $(shell sdl2-config --libs)

.PHONY: test

all: parsley test
parsley: ./src/parsley.o ./src/load.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test:
	$(MAKE) -C ./test run

clean:
	$(RM) ./src/*.o
	$(RM) ./parsley
	$(MAKE) -C ./test clean

