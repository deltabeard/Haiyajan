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
	STATIC := 1
else
	STATIC := 0
endif
ifeq ($(STATIC),1)
	LDLIBS += $(shell sdl2-config --static-libs)
	CFLAGS += -static
else
	LDLIBS += $(shell sdl2-config --libs)
endif

GIT_VERSION := $(shell git rev-parse --short HEAD 2>/dev/null)
ifneq ($(GIT_VERSION),)
	CFLAGS += -D GIT_VERSION=\"$(GIT_VERSION)\"
endif

# Checks if the given library is available for linking. Works with GCC and
# Clang.
IS_LIB_AVAIL = $(shell $(CC) -l$(CHECK_LIB) 2>&1 >/dev/null | grep "cannot find" > /dev/null; echo $$?)

# Check if WEBP is available. Otherwise use BMP for screenshots.
CHECK_LIB := webp
USE_WEBP := $(IS_LIB_AVAIL)
ENABLE_WEBP_SCREENSHOTS ?= $(USE_WEBP)
ifeq ($(ENABLE_WEBP_SCREENSHOTS),1)
	LDLIBS += -lwebp
	CFLAGS += -D ENABLE_WEBP_SCREENSHOTS=1
endif

CHECK_LIB := x264
USE_X264 := $(IS_LIB_AVAIL)
CHECK_LIB := wavpack
USE_WAVPACK := $(IS_LIB_AVAIL)

ifeq ($(USE_X264)$(USE_WAVPACK),11)
	ENABLE_VIDEO_RECORDING ?= 1
else
	ENABLE_VIDEO_RECORDING ?= 0
endif
ifeq ($(ENABLE_VIDEO_RECORDING),1)
	LDLIBS += -lx264 -lwavpack
	CFLAGS += -D ENABLE_VIDEO_RECORDING=1
endif

SRC_DIR	:= ./src
INC_DIR	:= ./inc
SRCS	:= $(wildcard ./src/*.c)
HDRS	:= $(wildcard ./inc/*.h)
OBJS	:= $(SRCS:.c=.o)
DEPS	:= Makefile.depend
override CFLAGS += -I$(INC_DIR)

.PHONY: test

all: $(TARGETS)
haiyajan: $(DEPS) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(DEPS): $(SRCS)
	$(CC) $(CFLAGS) -MM $^ > $(DEPS)
	@sed -i -E "s/^(.+?).o: ([^ ]+?)\1/\2\1.o: \2\1/g" $(DEPS)

-include $(DEPS)

# Saves debug symbols in a separate file, and strips the main executable.
# To get information from stack trace: `addr2line -e haiyajan.debug addr`
haiyajan.sym: haiyajan
	strip --only-keep-debug -o $@ $<
	strip -s $<
	@chmod -x $@

test: haiyajan
	$(MAKE) -C ./test run

clean:
	$(RM) $(OBJS) $(DEPS) $(SRCS:.c=.gcda)
	$(RM) ./haiyajan ./haiyajan.exe ./haiyajan.sym
	$(MAKE) -C ./test clean

# 80char      |-------------------------------------------------------------------------------|
help:
	@echo "Available options and their descriptions when enabled:"
	@echo "  DEBUG=$(DEBUG)"
	@echo "          Enables all asserts and reduces optimisation."
	@echo
	@echo "  STATIC=$(STATIC)"
	@echo "          Enables static build."
	@echo
	@echo "  ENABLE_WEBP_SCREENSHOTS=$(ENABLE_WEBP_SCREENSHOTS)"
	@echo "          Uses libwebp to encode screenshots instead of BMP."
	@echo "          This option will be enabled automatically if the linker is able to"
	@echo "          detect the availability of libwebp."
	@echo "          If this option is disabled, screenshots will be saved as BMP files."
	@echo
	@echo "  ENABLE_VIDEO_RECORDING=$(ENABLE_VIDEO_RECORDING)"
	@echo "          Enables video recording of gameplay using libx264 and libwebpack."
	@echo "          This option will be enabled automatically if the linker is able to"
	@echo "          detect the availability of libx264 and libwebpack."
	@echo
	@echo "  OPT=\"$(OPT)\"   Set custom optimisation options."
	@echo
	@echo "  Example: make DEBUG=1 OPT=\"-Ofast -march=native\""
	@echo
	@echo
	@echo "Copyright (C) 2020 Mahyar Koshkouei"
	@echo "Haiyajan is free software; see the LICENSE file for copying conditions. There is"
	@echo "NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
	@echo ""
