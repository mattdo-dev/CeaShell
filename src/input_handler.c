#include "input_handler.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// Internal function declarations
static void handle_key_press(char c, char *cmd, int *cmd_len, int *history_idx, Trie* root);

void init_input_handler(Trie* root) {
    // Initialize anything needed for input handling, if necessary
}

int read_input_line(int input_fd, char *cmd, int *history_idx, Trie* root) {
    int cmd_len = 0;
    char c;

    while (read(input_fd, &c, 1) == 1) {
        handle_key_press(c, cmd, &cmd_len, history_idx, root);
        if (c == '\n' || c == '\r') {
            printf("\n");
            break;
        }
    }

    cmd[cmd_len] = '\0';  // Null-terminate the command
    return cmd_len;
}

void cleanup_input_handler() {
    // Clean up resources if necessary
}

static void handle_key_press(char c, char *cmd, int *cmd_len, int *history_idx, Trie* root) {
    // Handle key press events, like arrow keys, backspace, tab completion, etc.
    // This function will modify `cmd` and `cmd_len` as needed.
    if (isprint(c)) {
        if (*cmd_len < MAX_INPUT - 1) {
            cmd[(*cmd_len)++] = c;
            write(STDOUT_FILENO, &c, 1); // Echo back the character
        }
    }
    // Further implementation goes here...
}
