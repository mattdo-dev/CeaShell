## Do not change this file
TARGETS=thsh

# Find all .c files in the current directory and src directory
SRC=$(wildcard *.c) $(wildcard src/*.c) $(wildcard src/util/*.c)

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
