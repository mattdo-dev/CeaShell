#include "path_manager.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

static char old_path[MAX_INPUT_LEN];
static char cur_path[MAX_INPUT_LEN];

int init_cwd(void) {
    if (getcwd(cur_path, sizeof(cur_path)) == NULL) {
        perror("thsh: init_cwd / getcwd");
        return -errno;
    }
    // Optionally, set old_path to cur_path on initialization
    strcpy(old_path, cur_path);
    return 0;
}

char *get_current_path(void) {
    return cur_path;
}

char *get_old_path(void) {
    return old_path;
}

void set_current_path(const char *path) {
    if (path != NULL) {
        strncpy(cur_path, path, MAX_INPUT_LEN);
        cur_path[MAX_INPUT_LEN - 1] = '\0';  // Ensure null termination
    }
}

void set_old_path(const char *path) {
    if (path != NULL) {
        strncpy(old_path, path, MAX_INPUT_LEN);
        old_path[MAX_INPUT_LEN - 1] = '\0';  // Ensure null termination
    }
}

