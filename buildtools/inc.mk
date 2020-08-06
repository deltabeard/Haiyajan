# Pre-define variables
CROSS_COMPILE ?=
CC ?= $(CROSS_COMPILE)cc
HOST_CC ?= $(CC)
STATIC :=

#              Unix           Mac OS     Windows
.LIBPATTERNS = lib%.a lib%.so lib%.dylib lib%.lib

# Quickly compiles $(1).c for use on the host system.
fn_hostcc = $(shell $(HOST_CC) -O1 -o $(1:.c=) $(1))

# Initialises and compiles build tools.
tools_src := echo.c
fn_init = $(foreach tlc,tools_src,$(call fn_hostcc,$(tlc)))

# Print a line to stdout. Uses own echo program to mitigate cross-platform
# compatibility issues (namely Windows echo).
fn_println = $(shell ./echo $(1))

# Returns 0 if library exists.
fn_chklib = $(shell $(MAKE) -f inc.mk CHKLIB=$(strip $(1)) rp_chklib >nul 2>&1)$(.SHELLSTATUS)
ifndef USE_BUILDTOOLS
rp_chklib: -l$(CHKLIB)
	@:
endif
