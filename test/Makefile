CFLAGS := -std=c99 -g3 -Og -Wall -Wextra -Werror -I../inc \
	$(shell sdl2-config --cflags) -D ENABLE_WEBP_SCREENSHOTS=0 \
	-D ENABLE_VIDEO_RECORDING=0
LDLIBS := $(shell sdl2-config --libs)

SRC_DIR	:= ../src
INC_DIR	:= ../inc
SRCS	:= $(addprefix $(SRC_DIR)/, font.c gl.c input.c load.c \
	menu.c play.c sig.c timer.c tinflate.c ui.c util.c)
HDRS	:= $(wildcard $(INC_DIR)/*.h)
OBJS	:= $(SRCS:.c=.o)

all: test run
test: test.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

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
