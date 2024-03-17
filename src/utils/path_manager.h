/*
 * Path utils
 */

#ifndef PATH_MANAGER_H
#define PATH_MANAGER_H

#define MAX_INPUT 256

/* This function needs to be called once at start-up to initialize
 * the current path.  This should populate cur_path.
 *
 * Returns zero on success, -errno on failure.
 */
int init_cwd(void);

// Get the current working directory.
char* get_current_path(void);

// Get the previous working directory.
char* get_old_path(void);

// Set the current working directory.
void set_current_path(const char* path);

// Set the previous working directory.
void set_old_path(const char* path);

#endif //PATH_MANAGER_H
