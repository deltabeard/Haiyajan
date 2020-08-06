# This Makefile is for use with GNU make only.

#              Unix           Mac OS     Windows
.LIBPATTERNS = lib%.a lib%.so lib%.dylib lib%.lib

ifneq ($(OS),Windows_NT)
	NULL := /dev/null
else
	NULL := nul
	RM := del
endif

ifeq ($(CC),cl)
	OBJEXT := obj
	EXEOUT := /Fe
else
	OBJEXT := o
	EXEOUT := -o
endif

# Returns 0 if library exists.
fn_chklib = $(shell $(MAKE) -f buildtools.mk CHKLIB=$(strip $(1)) rp_chklib >$(NULL) 2>&1)$(.SHELLSTATUS)
ifdef CHKLIB
rp_chklib: -l$(CHKLIB)
	@cd
endif
