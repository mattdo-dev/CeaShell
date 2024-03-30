/*
 * This file is the main entry point for the ceashell.
 * It is responsible for reading input, parsing it, and
 * executing commands.
 */

#include "src/jobs.h"
#include "src/parse.h"
#include "src/utils/constants.h"
#include "src/utils/trie.h"
#include "src/utils/path_manager.h"
#include "src/builtin.h"
#include "src/history.h"
#include "src/raw_mode.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <ctype.h>


int main(int argc, char **argv, char **envp) {
    // flag that the program should end
    bool finished = 0;
    int input_fd = 0; // Default to stdin
    int ret = 0;
    int debug = 0;
    int time_counting = 0;
    Trie *root = get_node();

    load_history();

    /* Argument support:
     * currently handles debug -d, and input file for non-interactive mode,
     * which can be used to run scripts.
     */
    if (argc > 1) {
        if (strcmp(argv[1], "-d") == 0) {
            debug = 1;
        } else if (strcmp(argv[1], "-t") == 0) {
            time_counting = 1;
        } else {
            /* If a file is specified, open it in place for stdin. */
            input_fd = open(argv[1], O_RDONLY);
            if (input_fd < 0) {
                dprintf(2, "Failed to open %s\n", argv[1]);
                return input_fd;
            }
        }
    }

    ret = init_cwd();
    if (ret) {
        dprintf(2, "Error initializing the current working directory: %d\n", ret);
        return ret;
    }

    ret = init_path();
    if (ret) {
        dprintf(2, "Error initializing the path table: %d\n", ret);
        return ret;
    }

    char **paths = get_path_table();
    char **builtins = get_builtin_names();
    populate_trie(root, paths);
    populate_trie(root, builtins);

    enable_raw_mode();

    while (!finished) {
        // Buffer to hold input
        char cmd[MAX_INPUT] = {0};
        int cmd_len = 0;
        char scratch[MAX_INPUT];
        char *parsed_commands[MAX_PIPELINE][MAX_ARGS];
        char *infile = NULL;
        char *outfile = NULL;
        int pipeline_steps;
        int history_idx = get_history_length();

        if (!input_fd) {
            ret = print_prompt();
            if (ret <= 0) {
                // if we printed 0 bytes, this call failed and the program
                // should end -- this will likely never occur.
                finished = true;
                break;
            }
        }

        // Reset memory from the last iteration
        for (int i = 0; i < MAX_PIPELINE; i++) {
            for (int j = 0; j < MAX_ARGS; j++) {
                parsed_commands[i][j] = NULL;
            }
        }

        // Read a line of input
        char c;
        /*
         *
         * RAW INPUT HANDLING
         *
         */
        while (read(input_fd, &c, 1) == 1) {
            if (c == '\x1b') {
                /*
                 *  ESC handling -- for arrow keys
                 */
                char seq[3];
                if (read(STDIN_FILENO, &seq[0], 1) != 1) break;
                if (read(STDIN_FILENO, &seq[1], 1) != 1) break;

                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'A': {  // UP arrow key
                            char *prev_cmd = get_prev_history_command(&history_idx);
                            if (prev_cmd) {
                                // clear current command
                                while (cmd_len > 0) {
                                    write(STDOUT_FILENO, "\b \b", 3);
                                    cmd_len--;
                                }
                                // load previous command into cmd buffer
                                strcpy(cmd, prev_cmd);
                                cmd_len = strlen(cmd);
                                write(STDOUT_FILENO, cmd, cmd_len);
                            }
                            break;
                        }
                        case 'B': {  // DOWN arrow key
                            char *next_cmd = get_next_history_command(&history_idx);
                            if (next_cmd) {
                                // clear current command
                                while (cmd_len > 0) {
                                    write(STDOUT_FILENO, "\b \b", 3);
                                    cmd_len--;
                                }
                                // load previous command into cmd buffer
                                strcpy(cmd, next_cmd);
                                cmd_len = strlen(cmd);
                                write(STDOUT_FILENO, cmd, cmd_len);
                            }
                            break;
                        }
                    }
                }
                continue;
            } else if (c == '\n' || c == '\r') {
                /*
                 * NEWLINE HANDLING
                 */
                printf("\n");
                break;
            } else if (c == '\t') {
                /*
                 * TAB HANDLING
                 */
                static int tab_count = 0;
                tab_count++;

                if (cmd_len <= 0) {
                    tab_count = 0;
                    continue; // No suggestions if no input
                }
                cmd[cmd_len] = '\0'; // Temporarily null-terminate current input

                int num_suggestions = 0;
                int suggestion_len;
                char **suggestions = find_suggestion(root, cmd, &num_suggestions);

                /* If there is only one suggestion, auto-complete the command.
                 * If there are multiple suggestions, print them all out after
                 * multiple tab spaces (ubuntu behavior)
                 */
                if (num_suggestions == 1) {
                    suggestion_len = strlen(suggestions[0]);

                    // Move cursor back and clear the current input
                    for (int i = 0; i < cmd_len; i++) {
                        write(STDOUT_FILENO, "\b \b", 3);
                    }

                    write(STDOUT_FILENO, suggestions[0], suggestion_len);

                    // Update cmd and cmd_len to reflect the new command
                    strcpy(cmd, suggestions[0]);
                    cmd_len = suggestion_len;
                } else if (num_suggestions > 1) {
                    if (tab_count >= 2) {
                        // Display all options
                        printf("\n");
                        for (int i = 0; i < num_suggestions; i++) {
                            printf("%s\n", suggestions[i]);
                        }
                        printf("\n");
                        ret = print_prompt();
                        write(STDOUT_FILENO, cmd, cmd_len);
                    }
                }
                free(suggestions);
                continue;
            } else if (c == '\x7f' || c == '\b') {
                /*
                 * BACKSPACE HANDLING
                 */
                if (cmd_len > 0) {
                    cmd[--cmd_len] = '\0'; // Remove the last character from the buffer
                    write(STDOUT_FILENO, "\b \b", 3); // Move back, write space, move back again
                }
                continue;
            } else if (isprint(c)) {
                /*
                 * PRINTABLE CHARACTER HANDLING
                 */
                if (cmd_len < MAX_INPUT - 1) {
                    cmd[cmd_len++] = c;
                    write(STDOUT_FILENO, &c, 1); // Echo back the character
                }
            }
        }

        cmd[cmd_len] = '\0'; // Null-terminate the command

        if (cmd[0] == '#') continue;

        // Add it to the history
        add_history_line(cmd);

        // Pass it to the parser
        pipeline_steps = parse_line(cmd,
                                    cmd_len,
                                    parsed_commands,
                                    &infile,
                                    &outfile,
                                    scratch,
                                    MAX_INPUT);

        if (pipeline_steps < 0) {
            dprintf(2,
                    "Parsing error.  Cannot execute command. %d\n",
                    -pipeline_steps);
            continue;
        }

        ret = 0;
        int fd[2];
        int in_fd = STDIN_FILENO;
        int out_fd = STDOUT_FILENO;

        /* Notes on the `open` function, for "<" redirection:
         * `O_RDONLY`: This flag opens the file for reading only.
         *
         * Ensures that our input file is readable for workable input.
         * */
        if (infile) {
            in_fd = open(infile, O_RDONLY);
            if (in_fd < 0) {
                perror("in_fd: error opening file");
                exit(EXIT_FAILURE);
            }
        }

        /* Notes on the `open` function, for ">" redirection:
         * `O_CREAT`: This flag tells the `open` function to create the file if
         *  it does not already exist.
         * `O_WRONLY`: This flag opens the file for writing only.
         * `S_IRUSR`: This flag gives the owner of the file read permission.
         * `S_IWUSR`: This flag gives the owner of the file write permission.
         * `S_IRGRP`: This flag gives the group of the file read permission.
         * `S_IROTH`: This flag gives others read permission.
         *
         * Ensures that our output file has the correct permissions for
         * workable output.
        */
        if (outfile) {
            out_fd = open(outfile,
                          O_CREAT | O_WRONLY,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (out_fd < 0) {
                perror("out_fd: error opening file");
                exit(EXIT_FAILURE);
            }
        }

        /*
         * This loop is responsible for executing a series of parsed commands.
         * It sets up piping between consecutive commands, ensuring the output
         * of one command is passed as input to the next, enabling the chaining
         * of commands.
         *
         * If it's the last command and an outfile is specified, the output is
         * redirected to the given outfile.
         */
        for (int i = 0; parsed_commands[i][0] != NULL; i++) {
            pipe(fd);

            if (debug)
                fprintf(stderr, "RUNNING: [%s]\n",
                        parsed_commands[i][0]);

            int next_in = fd[0];
            int next_out = fd[1];

            /* If it's the last command and outfile is specified, use out_fd as
             * output, otherwise use STDOUT_FILENO == 1 */
            if (parsed_commands[i + 1][0] == NULL) {
                next_out = (outfile) ? out_fd : STDOUT_FILENO;
            }

            if (handle_builtin(parsed_commands[i], in_fd, next_out, &ret)) {
                if (in_fd != STDIN_FILENO) {
                    close(in_fd);
                }
            } else {
                struct timeval start_time, end_time;
                struct rusage usage_start, usage_end;

                if (time_counting) {
                    gettimeofday(&start_time, NULL);
                    getrusage(RUSAGE_CHILDREN, &usage_start);
                }

                ret = run_command(parsed_commands[i], in_fd, next_out, 0);

                if (time_counting) {
                    gettimeofday(&end_time, NULL);
                    getrusage(RUSAGE_CHILDREN, &usage_end);

                    // Calculate elapsed time, then print it
                    long real_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                                     (end_time.tv_usec - start_time.tv_usec) / 1000;
                    long user_time = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) * 1000 +
                                     (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1000;
                    long sys_time = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) * 1000 +
                                    (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1000;

                    printf("TIMES: real=%.1fs user=%.1fs sys=%.1fs\n",
                           real_time / 1000.0, user_time / 1000.0, sys_time / 1000.0);
                }
            }

            if (debug) {
                fprintf(stderr, "ENDED: [%s] (ret=%d)\n",
                        parsed_commands[i][0], ret);
            }

            close(fd[1]);
            in_fd = next_in;
        }

        /* We need to make sure to close any file descriptors that we opened
         * for input and output redirection, if not already set to STDIN_FILENO
         * and STDOUT_FILENO, respectively.
         */
        if (in_fd != STDIN_FILENO) close(in_fd);
        if (out_fd != STDOUT_FILENO) close(out_fd);


        // Do NOT change this if/printf - it is used by the autograder.
        if (ret) {
            char buf[100];
            int rv = snprintf(buf, 100, "Failed to run command - error %d\n", ret);
            if (rv > 0)
                write(1, buf, strlen(buf));
            else
                dprintf(2,
                        "Failed to format the output (%d).  This shouldn't happen...\n",
                        rv);
        }
    }

    save_history();
    // Only return a non-zero value from main() if the shell itself
    // has a bug.  Do not use this to indicate a failed command.
    return 0;
}