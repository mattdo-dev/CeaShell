#include "../builtin.h"
#include "../utils/path_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* Handle a cd command. */
int handle_cd(char *args[MAX_ARG_SIZE], int stdin, int stdout) {
    int rv = 0;
    char* old_path = get_old_path();
    char* cur_path = get_current_path();

    if (!args[1]) {
        const char *home = getenv("HOME");
        if (home == NULL) {
            perror("thsh: cd: HOME not set");
            return -errno;
        }
        set_old_path(cur_path);
        if ((rv = chdir(home)) != 0) return rv;
        init_cwd();  // This will update cur_path in path_manager
        return rv;
    }

    if (args[1] && strcmp(args[1], "-") == 0) {
        /* cd - : go to previous directory.
         * directory should loop back and forth on repetitive,
         * consecutive calls */
        set_old_path(cur_path);  // Update old path before changing directory
        if ((rv = chdir(old_path)) != 0) return rv;
        init_cwd();  // This will update cur_path in path_manager
        fprintf(stderr, "%s\n", get_current_path());  // Use updated current path
    } else if (args[1]) {
        /* otherwise, cd to the specified directory, if possible */
        set_old_path(cur_path);  // Update old path before changing directory
        if ((rv = chdir(args[1])) != 0) return rv;
        init_cwd();  // This will update cur_path in path_manager
        return rv;
    }

    return rv;
}
