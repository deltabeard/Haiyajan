CFLAGS := -Og -g3 -fPIC -shared -Wl,--version-script=link.T -Wl,--no-undefined \
	-I../../inc -D SDL_ASSERT_LEVEL=3 $(shell sdl2-config --cflags)
LDLIBS := $(shell sdl2-config --libs)

all: libretro-av.so
libretro-av.so: libretro-av.c ../../src/timer.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) libretro-av.so

