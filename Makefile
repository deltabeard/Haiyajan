CFLAGS := -std=c99 -Wall -Wextra -I./inc $(shell sdl2-config --cflags)
LDLIBS := $(shell sdl2-config --libs)

ifeq ($(DEBUG),1)
	OPT := -g3 -Og
else
	# I don't want any warnings in release builds
	CFLAGS += -Werror
	OPT ?= -s -Ofast -march=native
endif

CFLAGS += $(OPT)

.PHONY: test

all: parsley test
parsley: ./src/parsley.o ./src/load.o ./src/play.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

./src/load.o: ./src/play.o

test:
	$(MAKE) -C ./test run

clean:
	$(RM) ./src/*.o
	$(RM) ./parsley
	$(MAKE) -C ./test clean

