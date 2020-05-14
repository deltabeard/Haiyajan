CFLAGS := -std=c99 -g3 -fPIE -Wall -Wextra -pipe -I./inc $(shell sdl2-config --cflags)
TARGETS := haiyajan

ifeq ($(DEBUG),1)
	CFLAGS += -D DEBUG=1 -D SDL_ASSERT_LEVEL=3
	OPT ?= -Og
else
	# I don't want any warnings in release builds
	CFLAGS += -Werror -D SDL_ASSERT_LEVEL=1 -flto=auto -fno-fat-lto-objects
	OPT ?= -O2
	TARGETS += haiyajan.sym
endif
CFLAGS += $(OPT)

ifeq ($(STATIC),1)
	LDLIBS := $(shell sdl2-config --static-libs)
	CFLAGS += -static
else
	LDLIBS := $(shell sdl2-config --libs)
endif

GIT_VERSION := $(shell git rev-parse --short HEAD 2>/dev/null)
REL_VERSION := $(shell git describe --tags 2>/dev/null)
ifneq ($(GIT_VERSION),)
	CFLAGS += -D GIT_VERSION=\"$(GIT_VERSION)\"
endif
ifneq ($(REL_VERSION),)
	CFLAGS += -D REL_VERSION=\"$(REL_VERSION)\"
endif

.PHONY: test

all: $(TARGETS)
haiyajan: ./src/haiyajan.o ./src/load.o ./src/play.o ./src/load.o \
		./src/timer.o ./src/font.o ./src/input.o ./src/gl.o ./src/png.o
	+$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

./src/haiyajan.o: ./src/haiyajan.c ./inc/*.h
./src/gl.o: ./src/gl.c ./inc/gl.h ./inc/libretro.h
./src/load.o: ./src/load.c ./inc/load.h ./inc/haiyajan.h ./inc/libretro.h
./src/play.o: ./src/play.c ./inc/play.h ./inc/haiyajan.h ./inc/libretro.h
./src/png.o: ./src/png.c ./inc/png.h
./src/timer.o: ./src/timer.c ./inc/timer.h
./src/input.o: ./src/input.c ./inc/input.h ./inc/gamecontrollerdb.h

# Saves debug symbols in a separate file, and strips the main executable.
# To get information from stack trace: `addr2line -e haiyajan.debug addr`
haiyajan.sym: haiyajan
	strip --only-keep-debug -o $@ $<
	strip -s $<
	@chmod -x $@

test: haiyajan
	$(MAKE) -C ./test run

clean:
	$(RM) ./src/*.o ./src/*.gcda
	$(RM) ./haiyajan ./haiyajan.exe ./haiyajan.sym
	$(MAKE) -C ./test clean

help:
	@echo "Options:"
	@echo "  DEBUG=1    Enables all asserts and reduces optimisation"
	@echo "  STATIC=1   Enables static build"
	@echo "  OPT=\"\"     Set custom optimisation options"
	@echo
	@echo "  Example: make DEBUG=1 OPT=\"-Ofast -march=native\""
	@echo
	@echo "Copyright (C) 2020 Mahyar Koshkouei"
	@echo "Haiyajan is free software; see the LICENSE file for copying conditions. There is "
	@echo "NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
	@echo ""
