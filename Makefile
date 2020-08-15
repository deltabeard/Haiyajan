# This Makefile is for use with GNU make only.
include buildtools.mk

TARGETS := haiyajan
DEBUG := 0
STATIC := 0

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

# Function substitutes variables depending on the value set in $(CC)
ccparam = $(if $(subst cl,,$(CC)),$(1),$(2))

# Set default flags
CFLAGS := $(call ccparam, -std=c99 -pedantic -g3 -fPIE -Wall -Wextra -pipe, /W3)
LDLIBS := -lSDL2 -lSDL2main

ifeq ($(DEBUG),1)
	CFLAGS += -DDEBUG=1 -DSDL_ASSERT_LEVEL=3
	OPT := -Og
else
	# I don't want any warnings in release builds
	CFLAGS += -DSDL_ASSERT_LEVEL=1 $(call ccparam, -Werror, /nologo /Drestrict=  )
	OPT := $(call ccparam,-O2 -flto -ffast-math,/O2 /Ob2 /fp:fast /GL /GS- /Zc:inline)
	TARGETS += $(call ccparam, haiyajan.sym,)
endif
CFLAGS += $(OPT)

GIT_VERSION := $(shell git describe --dirty --always --tags 2>$(NULL))
ifeq ($(GIT_VERSION),)
	GIT_VERSION := LOCAL
endif
CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"

ifneq ($(call fn_chklib, SDL2), 0)
	err := $(error Unable to find any of $(subst %,SDL2,$(.LIBPATTERNS) in library paths). SDL2 is required)
endif

ifeq ($(CC),cl)
	LDFLAGS += /link /LTCG /NODEFAULTLIB:MSVCRT /SUBSYSTEM:WINDOWS user32.lib gdi32.lib winmm.lib imm32.lib ole32.lib oleaut32.lib version.lib uuid.lib advapi32.lib setupapi.lib shell32.lib dinput8.lib
	SDL_LIBS = $(error SDL_LIBS must be defined to the location of the SDL2 include folder)
else
	SDL_LIBS = $(shell sdl2-config --libs)
	SDL_CFLAGS = $(shell sdl2-config --cflags)
endif

# Check if WEBP is available. Otherwise use BMP for screenshots.
USE_WEBP := $(call fn_chklib, webp)
ifeq ($(USE_WEBP), 0)
	ENABLE_WEBP_SCREENSHOTS := 1
else
	ENABLE_WEBP_SCREENSHOTS := 0
endif
ifeq ($(ENABLE_WEBP_SCREENSHOTS), 1)
	LDLIBS += -lwebp
	CFLAGS += -DENABLE_WEBP_SCREENSHOTS=1
endif

USE_X264 := $(call fn_chklib, x264)
ifeq ($(USE_X264), 0)
	VIDLIBS := -lx264
endif

USE_WAVPACK := $(call fn_chklib, wavpack)
ifeq ($(USE_WAVPACK), 0)
	VIDLIBS += -lwavpack
endif

ifeq ($(USE_X264)$(USE_WAVPACK),00)
	ENABLE_VIDEO_RECORDING := 1
else
	ENABLE_VIDEO_RECORDING := 0
endif
ifeq ($(ENABLE_VIDEO_RECORDING),1)
	LDLIBS += $(VIDLIBS)
	CFLAGS += $(LIBFLGS) -DENABLE_VIDEO_RECORDING=1
endif

SRCS	:= $(wildcard src/*.c)
HDRS	:= $(wildcard inc/*.h)
OBJS	:= $(SRCS:.c=.$(OBJEXT)) $(call ccparam,,meta/Haiyajan.res)
DEPS	:= Makefile.depend
override CFLAGS += -Iinc $(SDL_LIBS) $(SDL_CFLAGS)

.PHONY: test

all: $(TARGETS)
haiyajan: $(OBJS) $(LDLIBS)
	$(info LINK $@ $^)
	@$(CC) $(CFLAGS) $(EXEOUT)$@ $^ $(LDFLAGS) 1>$(NULL)

%.obj: %.c
	$(info CC $^)
	@$(CC) $(CFLAGS) /Fo$@ /c /TC $^ 1>$(NULL)

%.res: %.rc
	$(info RC $^)
	@rc /nologo /c65001 $^

include Makefile.depend

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

help:
	$(info $(help_txt))
	@cd 1>$(NULL)
