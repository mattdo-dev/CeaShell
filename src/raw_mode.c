/*
 * This file handles raw input modes to handle tab, up, down, etc.
 */

#include "util/trie.h"
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios global_termios;

// https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html

/**
 * Exits the program with an error message.
 * Clears the terminal screen and moves the cursor to the
 * home position before printing the error message and exiting.
 *
 * @param s The error message to be displayed.
 */
static void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

/**
 * Disables raw mode for the terminal.
 * Restores the terminal's original settings stored in `global_termios`.
 * If there is an error in setting the attributes, it calls `die`.
 */
void disable_raw_mode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &global_termios) == -1)
        die("tcsetattr");
}

/**
 * Enables raw mode for the terminal.
 * Modifies the terminal settings to disable echoing, canonical mode,
 * and certain control signals.
 * The original terminal settings are saved in `global_termios`.
 * If there is an error in getting or setting the attributes, it calls `die`.
 */
void enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &global_termios) == -1)
        die("tcgetattr");
    atexit(disable_raw_mode);

    struct termios raw = global_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}