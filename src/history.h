#ifndef HISTORY_H
#define HISTORY_H

/**
 * Adds a line to the history.
 * Removes the trailing newline from the line, if present,
 * and stores it in the history array.
 *
 * @param line The command line to be added to the history.
 */
void add_history_line(char *line);

/**
 * Clears the command history.
 * Frees memory for all stored history lines and resets the history count.
 */
void clear_history(void);

/**
 * Prints the command history to the specified output.
 *
 * @param stdout The file descriptor to which the history will be printed.
 */
void print_history(int stdout);

/**
 * Saves the command history to a file in the user's home directory.
 * The history is saved to a file named `.thsh_history`.
 *
 * @return 0 on success, -1 on failure.
 */
int save_history(void);

/**
 * Loads the command history from a file in the user's home directory.
 * The history is loaded from a file named `.thsh_history`.
 *
 * @return 0 on success, -1 on failure.
 */
int load_history(void);

/**
 * Retrieves the previous command from the history based on the current index.
 * Decrements the current index to move backwards in the history array.
 *
 * @param current_index A pointer to the current index in the history.
 * @return A pointer to the previous command, or NULL if there are no more previous commands.
 */
char *get_prev_history_command(int *current_index);

/**
 * Retrieves the next command from the history based on the current index.
 * Increments the current index to move forward in the history array.
 *
 * @param current_index A pointer to the current index in the history.
 * @return A pointer to the next command, or NULL if there are no more next commands.
 */
char *get_next_history_command(int *current_index);

/**
 * Gets the total number of commands stored in the history.
 *
 * @return The number of commands in the history.
 */
int get_history_length(void);

#endif // HISTORY_H
