#ifndef JOBS_H
#define JOBS_H

int init_path(void);

char **get_path_table();

void print_path_table(void);

int create_job(void);

int run_command(char *args[MAX_ARGS], int stdin, int stdout, int job_id);

int wait_on_job(int job_id, int *exit_code);

#endif
