CFLAGS := -std=c99 -g3 -fPIE -Wall -Wextra -pipe -I./inc $(shell sdl2-config --cflags)
TARGETS := haiyajan

DEBUG ?= 0
ifeq ($(DEBUG),1)
	CFLAGS += -D DEBUG=1 -D SDL_ASSERT_LEVEL=3
	OPT ?= -Og
else
	# I don't want any warnings in release builds
	CFLAGS += -Werror -D SDL_ASSERT_LEVEL=1 -flto
	OPT ?= -O2
	TARGETS += haiyajan.sym
endif
CFLAGS += $(OPT)

# Enable static build by default on Windows.
ifeq ($(OS),Windows_NT)
	STATIC ?= 1
else
	STATIC ?= 0
endif
ifeq ($(STATIC),1)
	LDLIBS += $(shell sdl2-config --static-libs)
	CFLAGS += -static
else
	LDLIBS += $(shell sdl2-config --libs)
endif

GIT_VERSION := $(shell git rev-parse --short HEAD 2>/dev/null)
REL_VERSION := $(shell git describe --tags 2>/dev/null)
ifneq ($(GIT_VERSION),)
	CFLAGS += -D GIT_VERSION=\"$(GIT_VERSION)\"
endif
ifneq ($(REL_VERSION),)
	CFLAGS += -D REL_VERSION=\"$(REL_VERSION)\"
endif

# Checks if the given library is available for linking. Works with GCC and
# Clang.
IS_LIB_AVAIL = $(shell $(CC) -l$(CHECK_LIB) 2>&1 >/dev/null | grep "cannot find" > /dev/null; echo $$?)

# Check if WEBP is available. Otherwise use BMP for screencaps.
CHECK_LIB := webp
USE_WEBP ?= $(IS_LIB_AVAIL)
ifeq ($(USE_WEBP),1)
	LDLIBS += -lwebp
	CFLAGS += -D USE_WEBP=1
endif

CHECK_LIB := x264
USE_X264 ?= $(IS_LIB_AVAIL)
ifeq ($(USE_X264),1)
	LDLIBS += -lx264
	CFLAGS += -D USE_X264=1
endif

CHECK_LIB := wavpack
USE_WAVPACK ?= $(IS_LIB_AVAIL)
ifeq ($(USE_WAVPACK),1)
	LDLIBS += -lwavpack
	CFLAGS += -D USE_WAVPACK=1
endif

.PHONY: test

all: $(TARGETS)
haiyajan: ./src/haiyajan.o ./src/load.o ./src/play.o ./src/load.o \
		./src/timer.o ./src/font.o ./src/input.o ./src/gl.o ./src/cap.o
	+$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

./src/haiyajan.o: ./src/haiyajan.c ./inc/*.h
./src/rec.o: ./src/rec.c ./inc/cap.h
./src/gl.o: ./src/gl.c ./inc/gl.h ./inc/libretro.h
./src/load.o: ./src/load.c ./inc/load.h ./inc/haiyajan.h ./inc/libretro.h
./src/play.o: ./src/play.c ./inc/play.h ./inc/haiyajan.h ./inc/libretro.h
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
	@echo "Available options and their descriptions when enabled:"
	@echo "  DEBUG=$(DEBUG)     Enables all asserts and reduces optimisation."
	@echo "  STATIC=$(STATIC)    Enables static build."
	@echo "  USE_WEBP=$(USE_WEBP)  Uses libwebp to encode screencaps instead of BMP."
	@echo "              If not set, the linker will check for its availability."
	@echo "  USE_X264=$(USE_X264)  Enables h264 video recording using libx264."
	@echo "              If not set, the linker will check for its availability."
	@echo "              Note that video recordings will be encoded in h264 RGB24 format, which is not "
	@echo "              compatible with many media players. Use ffplay if unsure."
	@echo "  OPT=\"$(OPT)\"   Set custom optimisation options."
	@echo
	@echo "  Example: make DEBUG=1 OPT=\"-Ofast -march=native\""
	@echo
	@echo "Copyright (C) 2020 Mahyar Koshkouei"
	@echo "Haiyajan is free software; see the LICENSE file for copying conditions. There is "
	@echo "NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
	@echo ""
