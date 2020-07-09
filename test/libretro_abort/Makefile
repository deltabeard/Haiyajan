CFLAGS := -Og -g3
override CFLAGS += -fPIC -shared -Wl,--version-script=link.T -Wl,--no-undefined -I../../inc

all: libretro-abort.so
libretro-abort.so: libretro-abort.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) libretro-abort.so

