CFLAGS := -Og -g3 -fPIC -shared -Wl,--version-script=link.T -Wl,--no-undefined \
	-I../../inc

all: libretro-init.so
libretro-init.so: libretro-init.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) libretro-init.so

