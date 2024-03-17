#ifndef THSH_H
#define THSH_H

#include "src/utils/constants.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>

// Disallow exec*p* variants, lest we spoil the fun
#pragma GCC poison execlp execvp execvpe

// Helper functions

// In jobs.c:
int init_path(void);

char **get_path_table();

void print_path_table(void);

int create_job(void);

int run_command(char *args[MAX_ARGS], int stdin, int stdout, int job_id);

int wait_on_job(int job_id, int *exit_code);

#endif // THSH_H