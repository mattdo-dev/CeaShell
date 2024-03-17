/* 730406661
 *
 * This file implements functions related to launching
 * jobs and job control, and the implementation of the execution of commands.
 */

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "thsh.h"

static char **path_table;

/* The constant defined here is merely arbitrary to define an initial size
 * for the job table.
 */
#define MAX_PATHS 512

/* Initialize the table of PATH prefixes.
 *
 * Split the result on the parentheses, and
 * remove any trailing '/' characters.
 * The last entry should be a NULL character.
 *
 * For instance, if one's PATH environment variable is:
 *  /bin:/sbin///
 *
 * Then path_table should be:
 *  path_table[0] = "/bin"
 *  path_table[1] = "/sbin"
 *  path_table[2] = '\0'
 *
 * Returns 0 on success, -errno on failure.
 */
int init_path(void) {
    char *path_var = getenv("PATH");
    char *next_colon;
    int index = 0;

    if (!path_var) {
        perror("Failed to get PATH environment variable.");
        return -errno;
    }

    path_table = malloc(MAX_PATHS * sizeof(char *));
    if (!path_table) {
        perror("Failed to allocate memory for path_table.");
        return -errno;
    }

    while (*path_var) {
        next_colon = strchr(path_var, ':') ?: path_var + strlen(path_var);

        int len = (int) (next_colon - path_var);
        path_table[index] = len ? strndup(path_var, len) : strdup(".");

        if (!path_table[index]) {
            perror("Failed to allocate memory for path_table entry.");
            return -errno;
        }

        for (; len > 0 && path_table[index][len - 1] == '/'; len--)
            path_table[index][len - 1] = '\0';

        if (++index >= MAX_PATHS) {
            perror("Exceeded path table size.\n");
            return -errno;
        }

        path_var = *next_colon ? next_colon + 1 : next_colon;
    }

    path_table[index] = NULL;

    return 0;
}


/*
 * Return the path table
 * Returns a NULL-terminated array of strings.
 */
char **get_path_table() {
    return path_table;
}

/* Debug helper function that just prints
 * the path table out.
 */
void print_path_table() {
    if (path_table == NULL) {
        printf("XXXXXXX Path Table Not Initialized XXXXX\n");
        return;
    }

    printf("===== Begin Path Table =====\n");
    for (int i = 0; path_table[i]; i++)
        printf("Prefix %2d: [%s]\n", i, path_table[i]);
    printf("===== End Path Table =====\n");
}

static int job_counter = 0;

struct kiddo {
    int pid;
    struct kiddo *next; // Linked list of sibling processes
};

// A job consists of a unique numeric ID and
// one or more processes
struct job {
    int id;
    struct kiddo *kidlets; // Linked list of child processes
    struct job *next; // Linked list of active jobs
};

// A singly linked list of active jobs.
static struct job *jobbies = NULL;

/* Initialize a job structure
 *
 * Returns an integer ID that represents the job.
 */
int create_job(void) {
    struct job *tmp;
    struct job *j = malloc(sizeof(struct job));
    j->id = ++job_counter;
    j->kidlets = NULL;
    j->next = NULL;
    if (jobbies) {
        for (tmp = jobbies; tmp && tmp->next; tmp = tmp->next);
        assert(tmp != j);
        tmp->next = j;
    } else {
        jobbies = j;
    }
    return j->id;
}

/* Helper function to walk the job list and find
 * a given job.
 *
 * remove: If true, remove this job from the job list.
 *
 * Returns NULL on failure, a job pointer on success.
 */
static struct job *find_job(int job_id, bool remove) {
    struct job *tmp, *last = NULL;
    for (tmp = jobbies; tmp; tmp = tmp->next) {
        if (tmp->id == job_id) {
            if (remove) {
                if (last) {
                    last->next = tmp->next;
                } else {
                    assert(tmp == jobbies);
                    jobbies = NULL;
                }
            }
            return tmp;
        }
        last = tmp;
    }
    return NULL;
}

/* Given the command listed in args,
 * try to execute it and create a job structure.
 *
 * This function does NOT wait on the child to complete,
 * nor does it return an exit code from the child.
 *
 * If the first argument starts with a '.'
 * or a '/', it is an absolute path and can
 * execute as-is.
 *
 * Otherwise, search each prefix in the path_table
 * in order to find the path to the binary.
 *
 * Then fork a child and pass the path and the additional arguments
 * to execve() in the child.  Wait for execution to complete
 * before returning.
 *
 * stdin is a file handle to be used for standard in.
 * stdout is a file handle to be used for standard out.
 *
 * If stdin and stdout are not 0 and 1, respectively, they will be
 * closed in the parent process before this function returns.
 *
 * job_id is the job_id allocated in create_job
 *
 * Returns 0 on success, -errno on failure to create the child.
 *
 */
int run_command(char *args[MAX_ARGS], int stdin, int stdout, int job_id) {
    char *path = NULL;

    if (!args[0]) return 0;

    /* If the first argument starts with a '.' or a '/', it is an absolute path
     * and can execute as-is.
     *
     * Otherwise, search each prefix in the path_table in order to find the path
     * to the binary.
     */
    char pre = '\r';
    write(STDOUT_FILENO, &pre, 1);

    if (*args[0] == '.' || *args[0] == '/') path = args[0];
    else {
        for (int i = 0; path_table[i]; i++) {
            char tmp[strlen(path_table[i]) + strlen(args[0]) + 2];
            sprintf(tmp, "%s/%s", path_table[i], args[0]);
            if (access(tmp, X_OK) == 0) {
                path = strdup(tmp);
                break;
            }
        }
    }

    /* Ensure that our path exists, otherwise we terminate with error */
    if (!path || stat(path, &(struct stat) {}) != 0) return -ENOENT;

    /*
     * This block handles the forking of the current process to execute a command.
     * It ensures proper redirection of stdin and stdout, allowing for
     * command output and input to be directed as needed.
     * The parent process waits for the child to complete, making sure that we
     * don't have any zombie processes.
     */
    pid_t pid = fork();
    if (pid < 0) return -errno;
    if (pid == 0) {
        if (stdin != STDIN_FILENO) {
            dup2(stdin, STDIN_FILENO);
            close(stdin);
        }
        if (stdout != STDOUT_FILENO) {
            dup2(stdout, STDOUT_FILENO);
            close(stdout);
        }
        execve(path, args, __environ);
        perror("execve");
        _exit(errno);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }

    free(path);
    (void) &find_job;
    return 0;
}

/* Wait for the job to complete and free internal bookkeeping
 *
 * job_id is the job_id allocated in create_job
 *
 * exit_code is the exit code from the last child process, if it executed.
 *           This parameter may be NULL, and is only set if the return
 *           value is zero.  This is the same as the wstatus parameter
 *           to waitpid variants, and can be used with functions such
 *           as WIFEXITED.  If this job includes multiple
 *           processes, the exit code will be the last process.
 *
 * Returns zero on success, -errno on error.
 */
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
