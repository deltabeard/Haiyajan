OPT ?= -g3 -Og
CFLAGS := $(OPT) -std=c99 -pedantic -Wall -Wextra -Werror -I./inc \
	$(shell sdl2-config --cflags)
	LDLIBS := $(shell sdl2-config --libs)

all: parsley
parsley: ./src/parsley.o ./src/load.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test:
	$(MAKE) -C ./test

clean:
	$(RM) ./src/*.o
	$(RM) ./parsley
	$(MAKE) -C ./test clean
