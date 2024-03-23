/*
 * This module implements tracking, saving, clearing, and restoring command history.
 */

#include "history.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_HISTORY 50

const char *DELIM = "\1\2";
char *history[MAX_HISTORY];
int history_count = 0;

/* Add a line to the history
 */
void add_history_line(char *line) {
    // Remove trailing newline, if present
    line[strcspn(line, "\n")] = '\0';

    if (history_count < MAX_HISTORY) {
        history[history_count++] = strdup(line);
    } else {
        // If history is full, remove the oldest entry
        free(history[0]);
        memmove(history, history + 1, (MAX_HISTORY - 1) * sizeof(char *));
        history[MAX_HISTORY - 1] = strdup(line);
    }
}

void clear_history(void) {
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
        history[i] = NULL;
    }
    history_count = 0;
}

void print_history(int stdout) {
    char pre = '\r';
    write(STDOUT_FILENO, &pre, 1);
    for (int i = 0; i < history_count; i++) {
        dprintf(stdout, "%d: %s\n", i + 1, history[i]);
    }
}

int save_history(void) {
    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        home_dir = "/tmp";  // Fallback directory
    }
    char history_file[PATH_MAX];
    snprintf(history_file, sizeof(history_file), "%s/.thsh_history", home_dir);

    FILE *file = fopen(history_file, "w");
    if (!file) return -1;

    for (int i = 0; i < history_count; i++) {
        fprintf(file, "%s%s", history[i], DELIM);
    }

    fclose(file);
    return 0;
}

int load_history(void) {
    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        home_dir = "/tmp";  // Fallback directory
    }
    char history_file[PATH_MAX];
    snprintf(history_file, sizeof(history_file), "%s/.thsh_history", home_dir);

    FILE *file = fopen(history_file, "r");
    if (!file) return -1;

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(fsize + 1);
    fread(buffer, 1, fsize, file);
    fclose(file);
    buffer[fsize] = 0;

    char *token = strtok(buffer, DELIM);
    while (token) {
        add_history_line(strdup(token));
        token = strtok(NULL, DELIM);
    }

    free(buffer);
    return 0;
}

// Function to get the previous command
char *get_prev_history_command(int *current_index) {
    if (*current_index > 0) {
        return history[--(*current_index) - 1];
    }
    return NULL;
}

// Function to get the next command
char *get_next_history_command(int *current_index) {
    if (*current_index < history_count - 1) {
        return history[++(*current_index)];
    }
    return NULL;
}

int get_history_length() {
    return history_count;
}
