OPT ?= -g3 -Og
CFLAGS ?= -Wall -Wextra
CFLAGS += $(OPT) -fPIC -shared -Wl,--version-script=link.T -Wl,--no-undefined \
	  -I../../inc
LDLIBS := -lm

all: libretro-init.so
libretro-init.so: libretro-init.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) libretro-init.so

