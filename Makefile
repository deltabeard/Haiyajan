# This Makefile is for use with GNU make only.
include buildtools.mk

ifeq ($(CC)$(CFLAGS),cl)
	err = $(error cl does not use GCC or Clang compatible options; use \
		  		build_win.bat or specify CFLAGS and LDFLAGS)
endif

PKG_CONFIG := pkg-config
TARGETS := haiyajan
DEBUG := 0
STATIC := 0
CFLAGS = -std=c99 -pedantic -g3 -fPIE -Wall -Wextra -pipe -flto
LDFLAGS =
STRIP := strip

define help_txt
Available options and their descriptions when enabled:
  DEBUG=$(DEBUG)
          Enables all asserts and reduces optimisation.

  STATIC=$(STATIC)
          Enables static build.

  ENABLE_WEBP_SCREENSHOTS=$(ENABLE_WEBP_SCREENSHOTS)
          Uses libwebp to encode screenshots instead of BMP.
          This option will be enabled automatically if the linker is able to
          detect the availability of libwebp.
          If this option is disabled, screenshots will be saved as BMP files.

  ENABLE_VIDEO_RECORDING=$(ENABLE_VIDEO_RECORDING)
          Enables video recording of gameplay using libx264 and libwavpack.
          This option will be enabled automatically if the linker is able to
          detect the availability of libx264 and libwavpack.

  OPT="$(OPT)"
          Set custom optimisation options.

  Example: make DEBUG=1 OPT="-Ofast -march=native"

Copyright (C) 2020 Mahyar Koshkouei
Haiyajan is free software; see the LICENSE file for copying conditions. There is
NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
endef

ifeq ($(STATIC),1)
	CFLAGS += -static
	PKG_CONFIG += --static
endif

# Function substitutes variables depending on the value set in $(CC)
ccparam = $(if $(subst cl,,$(CC)),$(1),$(2))

chklib_fn = $(shell $(PKG_CONFIG) --exists $(1))$(.SHELLSTATUS)
getlibs_fn = $(shell $(PKG_CONFIG) --libs $(1))
getcflags_fn = $(shell $(PKG_CONFIG) --cflags $(1))

ifeq ($(DEBUG),1)
	CFLAGS += -DDEBUG=1 -DSDL_ASSERT_LEVEL=3
	OPT = -Og
else
	# I don't want any warnings in release builds
	CFLAGS += -DSDL_ASSERT_LEVEL=1 -Werror -O2 -ffast-math
	TARGETS += $(call ccparam, haiyajan.sym,)
endif

GIT_VERSION := $(shell git describe --abbrev=0 --tags 2>$(NULL))
GIT_FULL_VERSION := $(shell git describe --dirty --always --tags --long 2>$(NULL))
ifeq ($(GIT_VERSION),)
	GIT_VERSION := 0,0
endif
ifeq ($(GIT_FULL_VERSION),)
	GIT_FULL_VERSION := LOCAL
endif

COMMA := ,
FULLSTOP := .
EXE_VERSION = $(subst $(FULLSTOP),$(COMMA),$(GIT_VERSION))
CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\" -DGIT_FULL_VERSION=\"$(GIT_FULL_VERSION)\"

USE_SDL2 = $(call chklib_fn,sdl2)
ifneq ($(USE_SDL2),0)
	$(error SDL2 is required but $(PKG_CONFIG) could not locate it)
endif
SDL2_CFLAGS += $(call getcflags_fn,sdl2)
SDL2_LDLIBS += $(call getlibs_fn,sdl2)

# Check if WEBP is available. Otherwise use BMP for screenshots.
USE_WEBP = $(call chklib_fn,libwebp)
ifeq ($(USE_WEBP),0)
	ENABLE_WEBP_SCREENSHOTS = 1
else
	ENABLE_WEBP_SCREENSHOTS = 0
endif
ifeq ($(ENABLE_WEBP_SCREENSHOTS),1)
	WEBP_CFLAGS += $(call getcflags_fn,libwebp)
	WEBP_LDLIBS += $(call getlibs_fn,libwebp)
endif

USE_VIDEOREC = $(call chklib_fn,x264 wavpack)
ifeq ($(USE_VIDEOREC),0)
	ENABLE_VIDEO_RECORDING = 1
else
	ENABLE_VIDEO_RECORDING = 0
endif
ifeq ($(ENABLE_VIDEO_RECORDING),1)
	VID_CFLAGS += $(call getcflags_fn,x264 wavpack) -DENABLE_VIDEO_RECORDING=1
	VID_LDLIBS += $(call getlibs_fn,x264 wavpack)
endif

override CFLAGS += -Iinc $(SDL2_CFLAGS) $(WEBP_CFLAGS) $(VID_CFLAGS)
override LDLIBS += -Iinc $(SDL2_LDLIBS) $(WEBP_LDLIBS) $(VID_LDLIBS)

SRCS := $(wildcard src/*.c)
HDRS := $(wildcard inc/*.h)
OBJS := $(SRCS:.c=.$(OBJEXT))
DEPS := Makefile.depend

ifeq ($(CC),cl)
	OBJS += meta/Haiyajan.res
endif

.PHONY: test

all: $(TARGETS)
haiyajan: $(OBJS)
	$(CC) $(CFLAGS) $(EXEOUT)$@ $^ $(LDFLAGS) $(LDLIBS)

%.obj: %.c
	$(CC) $(CFLAGS) /Fo$@ /c /TC $^

%.res: %.rc
	rc /nologo /c65001 /DEXE_VER=$(EXE_VERSION) /DGIT_VER="$(GIT_FULL_VERSION)" /DICON_FILE="$(ICON_FILE)" $^

include Makefile.depend

# Saves debug symbols in a separate file, and strips the main executable.
# To get information from stack trace: `addr2line -e haiyajan.debug addr`
haiyajan.sym: haiyajan
	$(STRIP) --only-keep-debug -o $@ $<
	$(STRIP) -s $<
	@chmod -x $@

test: haiyajan
	$(MAKE) -C ./test run

clean:
	$(RM) $(OBJS) $(SRCS:.c=.gcda)
	$(RM) ./haiyajan ./haiyajan.exe ./haiyajan.sym
	$(MAKE) -C ./test clean

help:
	$(info $(help_txt))
	@cd 1>$(NULL)
