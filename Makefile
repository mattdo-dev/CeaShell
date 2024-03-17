## Do not change this file
TARGETS=thsh

HEADERS=thsh.h utils.h src/input_handler.h src/raw_mode.h src/util/trie.h
OBJECTS=parse.o builtin.o jobs.o history.o src/input_handler.o src/raw_mode.o src/util/trie.o

CFLAGS= -Wall -Werror -g

.PHONY: all clean

all: $(TARGETS)

%.o: %.c $(HEADERS)
	gcc $(CFLAGS) -c -o $@ $<

src/%.o: src/%.c $(HEADERS)
	gcc $(CFLAGS) -c -o $@ $<

thsh: thsh.c $(OBJECTS)
	gcc $(CFLAGS) thsh.c $(OBJECTS) -o thsh

clean:
	rm -f $(TARGETS) $(OBJECTS)