#include "../builtin.h"
#include "../history.h"
#include <stdlib.h>
#include <unistd.h>

int handle_exit(char *args[MAX_ARG_SIZE], int stdin, int stdout) {
    (void) args;
    save_history();
    char pre = '\r';
    write(STDOUT_FILENO, &pre, 1);
    exit(0);
}
