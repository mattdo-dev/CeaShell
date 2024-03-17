/* 730406661
 *
 * This file implements a few built-in commands, such as cd and exit, and
 * our custom goheels command.
 */
#include "thsh.h"
#include <stdlib.h>

struct builtin {
    const char *cmd;

    int (*func)(char *args[MAX_ARGS], int stdin, int stdout);
};

static char old_path[MAX_INPUT];
static char cur_path[MAX_INPUT];

/* This function needs to be called once at start-up to initialize
 * the current path.  This should populate cur_path.
 *
 * Returns zero on success, -errno on failure.
 */
int init_cwd(void) {
    if (getcwd(cur_path, sizeof(cur_path)) == NULL) {
        perror("thsh: init_cwd / getcwd");
        return -errno;
    }
    return 0;
}

/* Handle a cd command.  */
int handle_cd(char *args[MAX_INPUT], int stdin, int stdout) {
    int rv = 0;

    if (!args[1]) {
        const char *home = getenv("HOME");
        if (home == NULL) {
            perror("thsh: cd: HOME not set\n");
            return -errno;
        }
        strcpy(old_path, cur_path);
        if ((rv = chdir(home)) != 0) return rv;
        init_cwd();
        return rv;
    }

    if (args[1] && strcmp(args[1], "-") == 0) {
        /* cd - : go to previous directory.
         * directory should loop back and forth on repetitive,
         * consecutive calls */
        if ((rv = chdir(old_path) != 0)) return rv;
        strcpy(old_path, cur_path);
        init_cwd();
        fprintf(stderr, "%s\n", cur_path);
    } else if (args[1]) {
        /* otherwise, cd to the specified directory, if possible */
        if ((rv = chdir(args[1])) != 0) return rv;
        strcpy(old_path, cur_path);
        init_cwd();
        return rv;
    }

    return rv;
}

/* Handle an exit command. */
int handle_exit(char *args[MAX_ARGS], int stdin, int stdout) {
    (void) args;
    save_history();
    char pre = '\r';
    write(STDOUT_FILENO, &pre, 1);
    exit(0);
}

int handle_history(char *args[MAX_ARGS], int stdin, int stdout) {
    print_history(stdout);
    return 0;
}

int handle_clear(char *args[MAX_ARGS], int stdin, int stdout) {
    clear_history();
    return 0;
}

/* Define our animation parameters */
#define LINES 6
#define NUM_COLORS 7
#define LOOP_COUNT 29
#define U_TO_SECOND 1000000
#define FRAME_DELAY (U_TO_SECOND * 0.1)

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
//int handle_goheels(char *args[MAX_ARGS], int in, int out) {
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
char** get_builtin_names() {
    int count = 0;
    while (builtins[count].cmd != NULL)
        count++;

    char** command_names = (char**) malloc((count + 1) * sizeof(char*));
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
int handle_builtin(char *args[MAX_ARGS], int stdin, int stdout, int *retval) {
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
    snprintf(prompt, sizeof(prompt), "\r[%s] thsh> ", cur_path);
    int ret = write(STDOUT_FILENO, prompt, strlen(prompt));

    if (ret < 0) {
        perror("Error writing the prompt");
        return -errno;
    }

    return ret;
}