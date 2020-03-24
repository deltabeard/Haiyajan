CFLAGS := -std=c99 -pedantic -g3 -Og -Wall -Wextra -Werror -I../inc \
	$(shell sdl2-config --cflags)
LDLIBS := $(shell sdl2-config --libs)

all: test run
test: test.o ../src/load.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

libretro-init:
	$(MAKE) -C ./libretro_init

run: libretro-init test
	@./test

clean:
	$(RM) ./test
	$(RM) ./*.o
	$(RM) ../src/*.o
	$(MAKE) -C ./libretro_init clean
