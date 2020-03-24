OPT ?= -g3 -Og
CFLAGS := -std=c99 -pedantic $(OPT) -Wall -Wextra -Werror -I./inc

all: parsley
parsley: ./src/parsley.o ./src/load.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test:
	$(MAKE) -C ./test

clean:
	$(RM) ./src/*.o
	$(RM) ./parsley
	$(MAKE) -C ./test clean
