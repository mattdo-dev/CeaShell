TARGETS=thsh
BUILD_DIR=build

# Build directories
SRC_DIRS= . src src/builtins src/utils
SRC=$(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))

# Object files will be located in the build directory
OBJECTS=$(SRC:%.c=$(BUILD_DIR)/%.o)

CFLAGS= -Wall -Werror -g

.PHONY: all clean

all: $(TARGETS)

# Create the build directory if it doesn't exist and compile the .c to .o
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	gcc $(CFLAGS) -c $< -o $@

thsh: $(OBJECTS)
	gcc $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGETS)
	rm -rf $(BUILD_DIR)
