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

# This Makefile is for use with GNU make only and is not compatible with
# Microsoft Visual C compilers.
ifeq ($(CC),cl)
	err = $(error cl is not supported. \
	      On Windows, compile within a Mingw-w64 distribution or use cmake.)
endif

DEBUG := 0
STATIC := 0
ENABLE_WEBP_SCREENSHOTS := 0
ENABLE_VIDEO_RECORDING := 0

PKGCONFIG := pkg-config
CFLAGS := $(shell $(PKGCONFIG) sdl2 --cflags)

ifeq ($(DEBUG),1)
	CFLAGS += -DDEBUG=1 -DSDL_ASSERT_LEVEL=3 -O0 \
		  -Wconversion -Wdouble-promotion -Wno-unused-parameter \
		  -Wno-unused-function -Wno-sign-conversion \
		  -fsanitize=undefined -fsanitize-trap
else
	# I don't want any warnings in release builds
	CFLAGS += -DSDL_ASSERT_LEVEL=1 -Werror -O2
endif

ifeq ($(STATIC),1)
	CFLAGS += -static
	LDLIBS += $(shell $(PKGCONFIG) sdl2 --libs --static)
else
	LDLIBS += $(shell $(PKGCONFIG) sdl2 --libs)
endif

# Obtain program version from git
GIT_VERSION := $(shell git describe --abbrev=0 --tags 2> /dev/null)
GIT_FULL_VERSION := $(shell git describe --dirty --always --tags --long 2> /dev/null)
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

# Set compiler flags for optional features.
ifeq ($(ENABLE_WEBP_SCREENSHOTS),1)
	LDLIBS += -lwebp
	CFLAGS += -DENABLE_WEBP_SCREENSHOTS=1
endif
ifeq ($(ENABLE_VIDEO_RECORDING),1)
	LDLIBS += -lwavpack -lx264
	CFLAGS += -DENABLE_VIDEO_RECORDING=1
endif

# Mandatory CFLAGS
override CFLAGS += -std=c99 -pedantic -g3 -Wall -Wextra -pipe -Iinc \
		 -Wshadow -fno-common -Wformat=2 -Wformat-truncation \
		 -Wformat-overflow -Wno-error=format -ffunction-sections \
		 -fdata-sections -Wl,--gc-sections

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)
#DEPS := Makefile.depend

.PHONY: test

all: haiyajan
haiyajan: $(OBJS)
	$(CC) $(CFLAGS) -o$@ $^ $(LDLIBS) $(LDFLAGS)

#%.res: %.rc
#	rc /nologo /c65001 /DEXE_VER=$(EXE_VERSION) /DGIT_VER="$(GIT_FULL_VERSION)" /DICON_FILE="$(ICON_FILE)" $^

-include Makefile.depend

test: haiyajan
	$(MAKE) -C ./test run

clean:
	$(RM) $(OBJS) $(SRCS:.c=.gcda)
	$(RM) ./haiyajan ./haiyajan.exe ./haiyajan.sym
	$(MAKE) -C ./test clean

help:
	$(info $(help_txt))
	@cd 1>$(NULL)
