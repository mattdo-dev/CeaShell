/*
 * This file implements functions related to launching
 * jobs and job control, and the implementation of the execution of commands.
 */

#include "jobs.h"
#include "utils/constants.h"
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

static char **path_table;
static int job_counter = 0;
static struct job *jobbies = NULL;

int init_path(void) {
    char *path_var = getenv("PATH");
    char *next_colon;
    int index = 0;

    if (!path_var) {
        perror("Failed to get PATH environment variable");
        return -errno;
    }

    path_table = malloc(MAX_PATHS * sizeof(char *));
    if (!path_table) {
        perror("Failed to allocate memory for path_table");
        return -errno;
    }

    while (*path_var) {
        next_colon = strchr(path_var, ':') ?: path_var + strlen(path_var);
        int len = next_colon - path_var;
        path_table[index] = strndup(path_var, len > 0 ? len : 1);

        if (!path_table[index]) {
            perror("Failed to allocate memory for path_table entry");
            return -errno;
        }

        path_table[index][strcspn(path_table[index], "/")] = '\0';
        index++;
        path_var = *next_colon ? next_colon + 1 : next_colon;
    }

    path_table[index] = NULL;
    return 0;
}

char **get_path_table(void) {
    return path_table;
}

void print_path_table(void) {
    if (!path_table) {
        printf("Path Table Not Initialized\n");
        return;
    }

    printf("Begin Path Table\n");
    for (int i = 0; path_table[i]; i++) {
        printf("Prefix %2d: [%s]\n", i, path_table[i]);
    }
    printf("End Path Table\n");
}

int create_job(void) {
    struct job *new_job = malloc(sizeof(struct job));
    if (!new_job) {
        perror("Failed to allocate memory for new job");
        return -errno;
    }

    new_job->id = ++job_counter;
    new_job->kidlets = NULL;
    new_job->next = NULL;

    if (jobbies) {
        struct job *last_job = jobbies;
        while (last_job->next) {
            last_job = last_job->next;
        }
        last_job->next = new_job;
    } else {
        jobbies = new_job;
    }

    return new_job->id;
}

static struct job *find_job(int job_id, bool remove) {
    struct job *prev = NULL;
    for (struct job *job = jobbies; job; job = job->next) {
        if (job->id == job_id) {
            if (remove) {
                if (prev) {
                    prev->next = job->next;
                } else {
                    jobbies = job->next;
                }
            }
            return job;
        }
        prev = job;
    }
    return NULL;
}

int run_command(char *args[MAX_ARGS], int stdin, int stdout, int job_id) {
    if (!args[0]) return -EINVAL;

    char *cmd = args[0];
    char *path = NULL;
    if (cmd[0] == '.' || cmd[0] == '/') {
        path = cmd;
    } else {
        for (int i = 0; path_table[i]; i++) {
            char tmp[PATH_MAX];
            snprintf(tmp, sizeof(tmp), "%s/%s", path_table[i], cmd);
            if (access(tmp, X_OK) == 0) {
                path = strdup(tmp);
                break;
            }
        }
    }

    if (!path) {
        fprintf(stderr, "Command not found: %s\n", cmd);
        return -ENOENT;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -errno;
    }

    if (pid == 0) {
        if (stdin != STDIN_FILENO) dup2(stdin, STDIN_FILENO);
        if (stdout != STDOUT_FILENO) dup2(stdout, STDOUT_FILENO);

        execve(path, args, __environ);
        perror("execve");
        _exit(errno);
    } else {
        waitpid(pid, NULL, 0);
    }

    free(path);
    return 0;
}

int wait_on_job(int job_id, int *exit_code) {
    struct job *j = find_job(job_id, false);
    if (!j) return -ENOENT;

    int status;
    struct kiddo *k = j->kidlets;
    while (k) {
        waitpid(k->pid, &status, 0);
        if (exit_code) *exit_code = status;

        struct kiddo *next_kid = k->next;
        free(k);
        k = next_kid;
    }

    // Remove job from jobbies list
    find_job(job_id, true);
    return 0;
}
