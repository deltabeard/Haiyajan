# This Makefile is for use with GNU make only.
# For Windows, set CC to "cl" to compile with MSVC build tools, for everything
# else, gcc compatible flags will be used.
USE_BUILDTOOLS := 1
include ./buildtools/inc.mk

TARGETS := haiyajan
DEBUG := 0

# Function substitutes variables depending on the value set in $(CC)
ccparam = $(if $(findstring cl,$(CC)), $(2), $(1))

# Set default flags
CFLAGS := $(call ccparam, -std=c99 -g3 -fPIE -Wall -Wextra -pipe, /W2)
LDLIBS :=

ifeq ($(DEBUG),1)
	CFLAGS += -DDEBUG=1 -DSDL_ASSERT_LEVEL=3
	OPT := -Og
else
	# I don't want any warnings in release builds
	CFLAGS += -DSDL_ASSERT_LEVEL=1
	CFLAGS += $(call ccparam, -Werror -flto, /GL)
	OPT := -O2
	TARGETS += haiyajan.sym
endif
CFLAGS += $(OPT)

# Enable static build by default on Windows.
ifeq ($(OS),Windows_NT)
	STATIC := 1
	NEWLN := echo.
	PIPE_SINK := nul
else
	STATIC := 0
	NEWLN := echo
	PIPE_SINK := /dev/null
endif

GIT_VERSION := $(shell git describe --dirty --always --tags 2> $(PIPE_SINK))
ifneq ($(GIT_VERSION),)
	GIT_VERSION := LOCAL
endif
CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"

ifeq ($(STATIC),1)
	STATIC_CFLAG := -static
	PKGSTC := --static
endif

ifneq ($(call fn_chklib, SDL2),0)
	err := $(error Unable to find any of $(subst %,SDL2,$(.LIBPATTERNS) in library paths). SDL2 is required)
endif
LDLIBS += $(shell sdl2-config --libs)
CFLAGS += $(shell sdl2-config --cflags)

# Check if WEBP is available. Otherwise use BMP for screenshots.
CHECK_LIB := libwebp
ENABLE_WEBP_SCREENSHOTS := $(IS_LIB_AVAIL)
ifeq ($(ENABLE_WEBP_SCREENSHOTS),1)
	LDLIBS += $(PKGLIB)
	CFLAGS += $(PKGFLG) -D ENABLE_WEBP_SCREENSHOTS=1
endif

VIDLIBS :=
VIDFLGS :=
CHECK_LIB := x264
USE_X264 := $(IS_LIB_AVAIL)
ifeq ($(USE_X264),1)
	VIDLIBS := $(PKGLIB)
	VIDFLGS := $(PKGFLG)
endif

CHECK_LIB := wavpack
USE_WAVPACK := $(IS_LIB_AVAIL)
ifeq ($(USE_WAVPACK),1)
	VIDLIBS += $(PKGLIB)
	VIDFLGS += $(PKGFLG)
endif

ifeq ($(USE_X264)$(USE_WAVPACK),11)
	ENABLE_VIDEO_RECORDING := 1
endif
ifeq ($(ENABLE_VIDEO_RECORDING),1)
	LDLIBS += $(VIDLIBS)
	CFLAGS += $(LIBFLGS) -D ENABLE_VIDEO_RECORDING=1
endif

SRC_DIR	:= ./src
INC_DIR	:= ./inc
SRCS	:= $(wildcard ./src/*.c)
HDRS	:= $(wildcard ./inc/*.h)
OBJS	:= $(SRCS:.c=.o)
DEPS	:= Makefile.depend
override CFLAGS += -I$(INC_DIR) $(STATIC_CFLAG)

.PHONY: test

all: $(TARGETS)
haiyajan: $(OBJS)
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
	$(RM) $(OBJS) $(SRCS:.c=.gcda) *.obj
	$(RM) ./haiyajan ./haiyajan.exe ./haiyajan.sym
	$(MAKE) -C ./test clean

# 80char      |-------------------------------------------------------------------------------|
help:
	@echo "Available options and their descriptions when enabled:"
	@echo "  DEBUG=$(DEBUG)"
	@echo "          Enables all asserts and reduces optimisation."
	@$(NEWLN)
	@echo "  STATIC=$(STATIC)"
	@echo "          Enables static build."
	@$(NEWLN)
	@echo "  ENABLE_WEBP_SCREENSHOTS=$(ENABLE_WEBP_SCREENSHOTS)"
	@echo "          Uses libwebp to encode screenshots instead of BMP."
	@echo "          This option will be enabled automatically if the linker is able to"
	@echo "          detect the availability of libwebp."
	@echo "          If this option is disabled, screenshots will be saved as BMP files."
	@$(NEWLN)
	@echo "  ENABLE_VIDEO_RECORDING=$(ENABLE_VIDEO_RECORDING)"
	@echo "          Enables video recording of gameplay using libx264 and libwebpack."
	@echo "          This option will be enabled automatically if the linker is able to"
	@echo "          detect the availability of libx264 and libwebpack."
	@$(NEWLN)
	@echo "  OPT=\"$(OPT)\"   Set custom optimisation options."
	@$(NEWLN)
	@echo "  Example: make DEBUG=1 OPT=\"-Ofast -march=native\""
	@$(NEWLN)
	@$(NEWLN)
	@echo "Copyright (C) 2020 Mahyar Koshkouei"
	@echo "Haiyajan is free software; see the LICENSE file for copying conditions. There is"
	@echo "NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
	@$(NEWLN)
