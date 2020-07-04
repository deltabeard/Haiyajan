CFLAGS := -std=c99 -g3 -Og -Wall -Wextra -Werror -I../inc \
	$(shell sdl2-config --cflags)
LDLIBS := $(shell sdl2-config --libs)

all: test run
test: test.o ../src/load.o ../src/timer.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

../src/load.o: ../src/load.c ../inc/load.h ../inc/haiyajan.h ../inc/libretro.h
../src/timer.o: ../src/timer.c ../inc/timer.h

libretro-init:
	$(MAKE) -C ./libretro_init

libretro-abort:
	$(MAKE) -C ./libretro_abort

libretro-av:
	$(MAKE) -C ./libretro_av

run: libretro-init libretro_abort libretro-av test
	@./test

clean:
	$(RM) ./test
	$(RM) ./*.o
	$(RM) ../src/*.o
	$(MAKE) -C ./libretro_init clean
