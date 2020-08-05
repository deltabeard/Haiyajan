# Pre-define variables
CROSS_COMPILE ?=
CC ?= $(CROSS_COMPILE)cc
HOST_CC ?= $(CC)

# Quickly compiles $(1).c for use on the host system.
fn_hostcc = $(shell $(HOST_CC) -O1 -o $(1:.c=) $(1))

# Initialises and compiles build tools.
tools_src := echo.c
fn_init = $(foreach tlc,tools_src,$(call fn_hostcc,$(tlc)))

fn_println = $(shell ./echo $(1))
