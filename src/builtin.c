/*
 * This file implements a few built-in commands.
 */

#include "builtin.h"
#include "history.h"
#include "utils/path_manager.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

struct builtin {
    const char *cmd;

    int (*func)(char *args[MAX_ARG_SIZE], int stdin, int stdout);
};

int handle_history(char *args[MAX_ARG_SIZE], int stdin, int stdout) {
    (void) args;
    print_history(stdout);
    return 0;
}

int handle_clear(char *args[MAX_ARG_SIZE], int stdin, int stdout) {
    (void) args;
    clear_history();
    return 0;
}

/* Define our animation parameters */
//#define LINES 6
//#define NUM_COLORS 7
//#define LOOP_COUNT 29
//#define U_TO_SECOND 1000000
//#define FRAME_DELAY (U_TO_SECOND * 0.1)

///* These follow the ANSI color code format, of which modern shells will
// * support for a wide variety of stylizing */
//const char *colors[] = {
//        "\x1B[31m", // Red
//        "\x1B[33m", // Yellow
//        "\x1B[32m", // Green
//        "\x1B[36m", // Cyan
//        "\x1B[34m", // Blue
//        "\x1B[35m", // Magenta
//        "\x1B[37m"  // White
//};
//
//char text[LINES][268] = {
//        " ██████╗  ██████╗    ████████╗ █████╗ ██████╗    ██╗  ██╗███████╗███████╗██╗     ███████╗ ██╗",
//        "██╔════╝ ██╔═══██╗   ╚══██╔══╝██╔══██╗██╔══██╗   ██║  ██║██╔════╝██╔════╝██║     ██╔════╝ ██║",
//        "██║  ███╗██║   ██║      ██║   ███████║██████╔╝   ███████║█████╗  █████╗  ██║     ███████╗ ██║",
//        "██║   ██║██║   ██║      ██║   ██╔══██║██╔══██╗   ██╔══██║██╔══╝  ██╔══╝  ██║     ╚════██║ ╚═╝",
//        "╚██████╔╝╚██████╔╝      ██║   ██║  ██║██║  ██║   ██║  ██║███████╗███████╗███████╗███████║ ██╗",
//        " ╚═════╝  ╚═════╝       ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝  ╚═╝╚══════╝╚══════╝╚══════╝╚══════╝ ╚═╝"
//};
//
///* Handle a goheels command. */
//int handle_goheels(char *args[MAX_ARG_SIZE], int in, int out) {
//    printf("\n");
//    for (int i = 0; i < LOOP_COUNT; i++) {
//        for (int j = 0; j < LINES; j++) {
//            /* Print the text in the color of the current iteration
//             * and flush stdout to make sure our writes are reliable;
//             * utilizing our \r (carriage) to point the cursor back to the start
//             * of the line, so we can overwrite, giving the illusion of static
//             * animation */
//            printf("%s%s\r\n", colors[(i + j) % NUM_COLORS], text[j]);
//            fflush(stdout);
//        }
//        usleep(FRAME_DELAY);
//        /* Move the cursor up 6 lines, so we can overwrite the previous "frame",
//         * otherwise don't reset cursor position for final run */
//        if (i < LOOP_COUNT - 1) printf("\033[%dA", LINES);
//    }
//    // Reset to default before termination
//    printf("\x1B[0m\n");
//    return 0;
//}

static struct builtin builtins[] = {{"cd",      handle_cd},
                                    {"exit",    handle_exit},
//                                    {"goheels", handle_goheels},
                                    {"history", handle_history},
                                    {"clear",   handle_clear},
                                    {NULL,      NULL}};

/*
 * This function returns an array of builtins.
 */
char **get_builtin_names() {
    int count = 0;
    while (builtins[count].cmd != NULL)
        count++;

    char **command_names = (char **) malloc((count + 1) * sizeof(char *));
    if (!command_names) {
        perror("malloc for command_names failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < count; i++) {
        command_names[i] = strdup(builtins[i].cmd); // make a copy, handle const...
    }

    command_names[count] = NULL;
    return command_names;
}

/* This function checks if the command (args[0]) is a built-in.
 * If so, call the appropriate handler, and return 1.
 * If not, return 0.
 *
 * stdin and stdout are the file handles for standard in and standard out,
 * respectively. These may or may not be used by individual builtin commands.
 *
 * Places the return value of the command in *retval.
 *
 * stdin and stdout should not be closed by this command.
 *
 * In the case of "exit", this function will not return.
 */
int handle_builtin(char *args[MAX_ARG_SIZE], int stdin, int stdout, int *retval) {
    /* Since an array of builtins is already provided, checking is a matter
     * of a looping and checking if the command matches.
     */
    for (int i = 0; builtins[i].cmd != NULL; ++i) {
        if (strcmp(builtins[i].cmd, args[0]) == 0) {
            *retval = builtins[i].func(args, stdin, stdout);
            return 1;
        }
    }
    return 0;
}

/* This function initially prints a default prompt of:
 * thsh>
 *
 * In Lab 2, Exercise 3, you will add the current working
 * directory to the prompt.  As in, if you are in "/home/foo"
 * the prompt should be:
 * [/home/foo] thsh>
 *
 * Returns the number of bytes written
 */
int print_prompt(void) {
    char prompt[PATH_MAX];

    /* Construct the prompt, and prepend output to our shell */
    snprintf(prompt, sizeof(prompt), "\r[%s] thsh> ", get_current_path());
    int ret = write(STDOUT_FILENO, prompt, strlen(prompt));

    if (ret < 0) {
        perror("Error writing the prompt");
        return -errno;
    }

    return ret;
}