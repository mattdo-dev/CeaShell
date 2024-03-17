/*
 *  Handles shell inputs in the terminal.
 */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "utils/trie.h"

/*
 * Initializes the input handler.
 */
void init_input_handler(Trie *root);

/*
 * Reads a line of input from the terminal.
 */
int read_input_line(int input_fd, char *cmd, int *history_idx, Trie *root);

/*
 * Cleans up the input handler.
 */
void cleanup_input_handler(void);

#endif
