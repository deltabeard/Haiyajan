# This Makefile is for use with GNU make only.
include buildtools.mk

TARGETS := haiyajan
DEBUG := 0
STATIC := 0
CFLAGS =
LDFLAGS =
BUILDDIR = src

define copyright
Copyright (C) 2020 Mahyar Koshkouei
Haiyajan is free software; see the LICENSE file for copying conditions. There is
NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
endef

define help_txt_unix
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

$(copyright)
endef

define help_txt_nt
Available options and their descriptions when enabled:
  ENABLE_WEBP_SCREENSHOTS=$(ENABLE_WEBP_SCREENSHOTS)
          Uses libwebp to encode screenshots instead of BMP.
          This option will be enabled automatically if the linker is able to
          detect the availability of libwebp.
          If this option is disabled, screenshots will be saved as BMP files.

  ENABLE_VIDEO_RECORDING=$(ENABLE_VIDEO_RECORDING)
          Enables video recording of gameplay using libx264 and libwavpack.
          This option will be enabled automatically if the linker is able to
          detect the availability of libx264 and libwavpack.

  SDL_LIB=""
	  Location of compiled SDL2 libraries SDL2-static.lib and SDL2main.lib.
	  
  SDL_INC=""
	  Location of SDL2 headers, such as SDL2.h.
	  
  Compiling for Windows NT $(NT_REV) $(Platform) platforms.

  Example: make -B SDL_LIB="C:\SDL2\libs" SDL_INC="C:\SDL2\include"

$(copyright)
endef

SDL_LIB := $(err SDL_LIB must define the location of the SDL2 libraries)
SDL_INC := $(err SDL_INC must define the location of the SDL2 headers)

# CFLAGS for all Windows NT platforms
NT_CFLAGS = /nologo /GL /O2 /Ob2 /fp:fast /Ot /GF /GT /Oi /MT \
	/I"inc" /I"$(SDL_INC)" \
	/D"_UNICODE" /D"UNICODE"
# 32-bit Windows NT builds require SSE instructions, supported from Pentium III CPUs.
# These builds offer support for ReactOS and Windows XP.
NT32_CFLAGS = $(NT_CFLAGS) /arch:SSE /Fd"$(BUILDDIR)\vc141.pdb"
# 64-bit Windows NT builds already have SSE and SSE2 enabled.
NT64_CFLAGS = $(NT_CFLAGS) /Fd"$(BUILDDIR)\vc142.pdb"

NT_LDFLAGS = /OUT:"Haiyajan-$(Platform).exe" /MANIFEST /LTCG /NXCOMPAT /PDB:"Haiyajan-$(Platform).pdb" /DYNAMICBASE "SDL2main.lib" "SDL2-static.lib" "winmm.lib" "msimg32.lib" "version.lib" "imm32.lib" "setupapi.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" /MACHINE:$(Platform) /INCREMENTAL:NO /SUBSYSTEM:WINDOWS",$(NT_REV)" /LIBPATH:"$(SDL_LIB)"

UNIX_CFLAGS = -std=c99 -pedantic -g3 -fPIE -Wall -Wextra -pipe $(OPT) -flto

# Check build system
ifeq ($(OS),Windows_NT)
	ifeq ($(DEBUG),1)
		err = $(error Debug builds are not supported on Windows NT builds)
	else ifneq ($(CC),cl)
		warn = $(warning You should use MSVC cl compiler to avoid issues with OpenGL on Windows NT systems)
	else ifeq ($(VSCMD_VER),)
		err = $(error You must execute GNU make within either Native Tools Command Prompt for VS 2019, or MinGW64)
	else ifeq ($(Platform),x86)
		info = $(info Compiling for Win32 platforms)
		CFLAGS += $(NT32_CFLAGS)
		NT_REV = 5.01
		ICON_FILE = icon_lo.ico
	else ifeq ($(Platform),x64)
		info = $(info Compiling for Win64 platforms)
		CFLAGS += $(NT64_CFLAGS)
		NT_REV = 6.01
		# Use high quality EXE icon for newer platforms
		ICON_FILE = icon_hi.ico
	endif
	
	help_txt = $(help_txt_nt)
else
	help_txt = $(help_txt_unix)
endif

ifeq ($(CC),cl)
	LDFLAGS = /link $(NT_LDFLAGS)
else
	CFLAGS = $(UNIX_CFLAGS) $(shell sdl2-config --cflags)
	LDFLAGS = $(shell sdl2-config --libs)
endif

# Function substitutes variables depending on the value set in $(CC)
ccparam = $(if $(subst cl,,$(CC)),$(1),$(2))

ifeq ($(DEBUG),1)
	# cl is already filtered here.
	CFLAGS += -DDEBUG=1 -DSDL_ASSERT_LEVEL=3
	OPT = -Og
else
	# I don't want any warnings in release builds
	CFLAGS += -DSDL_ASSERT_LEVEL=1 $(call ccparam, -Werror -O2 -ffast-math,)
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
override CFLAGS += -Iinc $(SDL_CFLAGS)
override LDFLAGS += $(SDL_LIBS)

.PHONY: test

all: $(TARGETS)
haiyajan: $(OBJS) $(LDLIBS)
	$(CC) $(CFLAGS) $(EXEOUT)$@ $^ $(LDFLAGS)

%.obj: %.c
	$(CC) $(CFLAGS) /Fo$@ /c /TC $^

%.res: %.rc
	rc /nologo /c65001 /DEXE_VER=$(EXE_VERSION) /DGIT_VER="$(GIT_FULL_VERSION)" /DICON_FILE="$(ICON_FILE)" $^

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
	$(RM) $(BUILDDIR) $(SRCS:.c=.gcda)
	$(RM) ./haiyajan ./haiyajan.exe ./haiyajan.sym
	$(MAKE) -C ./test clean

help:
	$(info $(help_txt))
	@cd 1>$(NULL)
