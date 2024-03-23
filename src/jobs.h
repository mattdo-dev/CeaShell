#ifndef JOBS_H
#define JOBS_H

#include "utils/constants.h"
#include <stdbool.h>

/**
 * Maximum number of PATH prefixes that can be stored.
 */
#define MAX_PATHS 512

struct kiddo {
    int pid;
    struct kiddo *next; // Linked list of sibling processes
};

struct job {
    int id;
    struct kiddo *kidlets; // Linked list of child processes
    struct job *next; // Linked list of active jobs
};


/**
 * Initializes the table of PATH prefixes.
 * Parses the PATH environment variable to populate the global path_table.
 * Trims any trailing '/' characters and ensures the table ends with a NULL.
 *
 * @return 0 on success, or negative errno on failure.
 */
int init_path(void);

/**
 * Retrieves the current table of PATH prefixes.
 *
 * @return A NULL-terminated array of strings containing the PATH prefixes.
 */
char **get_path_table(void);

/**
 * Prints the current path table to standard output.
 * Useful for debugging purposes, showing the list of directories
 * where executables are searched.
 */
void print_path_table(void);

/**
 * Creates a new job and adds it to the list of active jobs.
 * Each job is assigned a unique job ID.
 *
 * @return The job ID of the newly created job.
 */
int create_job(void);

/**
 * Executes a command in a new process, associates it with a job ID, and
 * does not wait for the command to complete before returning.
 * If the command's first argument is not an absolute path, it searches
 * each prefix in the path_table to find the executable.
 * The command's input and output can be redirected by specifying file
 * descriptors other than the standard input (0) and output (1).
 *
 * @param args Array of strings representing the command and its arguments.
 * @param stdin File descriptor for standard input.
 * @param stdout File descriptor for standard output.
 * @param job_id The ID of the job to which this command belongs.
 * @return 0 on success, or negative errno on failure to execute the command.
 */
int run_command(char *args[MAX_ARGS], int stdin, int stdout, int job_id);

/**
 * Waits for all processes in the job to complete, then frees associated resources.
 * Captures and returns the exit code of the last child process in the job.
 * If the job consists of multiple processes, the exit code returned is that
 * of the last process to finish.
 *
 * @param job_id The ID of the job to wait on.
 * @param exit_code Pointer to store the exit code of the last process of the job.
 * @return 0 on success, or negative errno on failure.
 */
int wait_on_job(int job_id, int *exit_code);

#endif
