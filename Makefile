CFLAGS := -std=c99 -g3 -Wall -Wextra -pipe -I./inc $(shell sdl2-config --cflags)
LDLIBS := $(shell sdl2-config --libs)

ifeq ($(DEBUG),1)
	CFLAGS += -D DEBUG=1 -D SDL_ASSERT_LEVEL=3 -fsanitize=undefined -fsanitize=bounds-strict
	OPT ?= -Og
else
	# I don't want any warnings in release builds
	CFLAGS += -Werror -D SDL_ASSERT_LEVEL=1 -fPIE -flto=auto -fno-fat-lto-objects
	OPT ?= -Ofast
endif

CFLAGS += $(OPT)

.PHONY: test

all: parsley test
parsley: ./src/parsley.o ./src/load.o ./src/play.o
	+$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test:
	$(MAKE) -C ./test run

clean:
	$(RM) ./src/*.o
	$(RM) ./parsley
	$(MAKE) -C ./test clean

