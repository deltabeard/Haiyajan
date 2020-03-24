OPT ?= -g3 -Og
CFLAGS := -std=c99 -pedantic $(OPT) -Wall -Wextra -Werror -I./inc

all: jafari
jafari: ./src/jafari.o ./src/load.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test:
	$(MAKE) -C ./test

clean:
	$(RM) ./jafari
	$(MAKE) -C ./test clean
