TARGETS=thsh

# Main build directories
SRC=$(wildcard *.c) \
    $(wildcard src/*.c) \
    $(wildcard src/builtins/*.c) \
    $(wildcard src/utils/*.c)

# Replace the .c extension with .o to get the list of object files
OBJECTS=$(SRC:.c=.o)

CFLAGS= -Wall -Werror -g

.PHONY: all clean

all: $(TARGETS)

# Rule to compile .c to .o; handles dependencies automatically
%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

thsh: $(OBJECTS)
	gcc $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGETS) $(OBJECTS)
