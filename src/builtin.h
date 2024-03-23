#define MAX_ARG_SIZE 256

int handle_builtin(char *args[MAX_ARG_SIZE], int stdin, int stdout, int *retval);

int print_prompt(void);

char **get_builtin_names(void);

int handle_cd(char *args[MAX_ARG_SIZE], int stdin, int stdout);

int handle_exit(char *args[MAX_ARG_SIZE], int stdin, int stdout);
