/*
 * This module implements command parsing, following the grammar
 * in the assignment handout.
 */

#include "parse.h"
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Allocate and copy a substring of a string.
 * @param destination Pointer to the destination string.
 * @param source Pointer to the source string.
 * @param start_idx Index of the first character to copy.
 * @param length Number of characters to copy.
 * @return 0 on success, -1 on failure.
 */
int allocate_and_copy_substring(char **destination,
                                char *source,
                                int start_idx,
                                long length) {
    if (destination == NULL) {
        fprintf(stderr, "destination argument is NULL\n");
        return -1;
    }

    if (source == NULL) {
        fprintf(stderr, "source argument is NULL\n");
        return -1;
    }

    if (start_idx < 0) {
        fprintf(stderr, "start_idx argument is negative\n");
        return -1;
    }

    if (length < 0) {
        fprintf(stderr, "length argument is negative\n");
        return -1;
    }

    *destination = malloc((length + 1) * sizeof(char *));
    strncpy(*destination, &source[start_idx], length);
    (*destination)[length] = '\0';
    return 0;
}

/* This function returns one line from input_fd
 *
 * buf is populated with the contents, including the newline, and
 *      including a null terminator.  Must point to a buffer
 *      allocated by the caller, and may not be NULL.
 *
 * size is the size of *buf
 *
 * Return value: the length of the string (not counting the null terminator)
 *               zero indicates the end of the input file.
 *               a negative value indicates an error (e.g., -errno)
 */
int read_one_line(int input_fd, char *buf, size_t size) {
    int count, rv;
    // pointer to next place in cmd to store a character
    char *cursor;
    // the last character that was written into cmd
    char last_char;

    assert(buf);

    /*
     * We want to continue reading characters until:
     *   - read() fails (rv will become 0) OR
     *   - count == MAX_INPUT-1 (we have no buffer space left) OR
     *   - last_char was '\n'
     * so we continue the loop while:
     *   rv is nonzero AND count < MAX_INPUT - 1 AND last_char != '\n'
     *
     * On every iteration, we:
     *   - increment cursor to advance to the next char in the cmd buffer
     *   - increment count to indicate we've stored another char in cmd
     *     (this is done during the loop termination check with "++count")
     *
     * To make the termination check work as intended, the loop starts by:
     *   - setting rv = 1 (so it's initially nonzero)
     *   - setting count = 0 (we've read no characters yet)
     *   - setting cursor = cmd (cursor at start of cmd buffer)
     *   - setting last_char = 1 (so it's initially not '\n')
     *
     * In summary:
     *   - START:
     *      set rv = 1, count = 0, cursor = cmd, last_char = 1
     *   - CONTINUE WHILE:
     *      rv (is nonzero) && count < MAX_INPUT - 1 && last_char != '\n'
     *   - UPDATE PER ITERATION:
     *      increment count and cursor
     */
    for (rv = 1, count = 0, cursor = buf, last_char = 1;
         rv && (last_char != '\n') && (++count < (size - 1)); cursor++) {

        // read one character
        // file descriptor 0 -> reading from stdin
        // writing this one character to cursor (current place in cmd buffer)
        rv = read(input_fd, cursor, 1);
        last_char = *cursor;
    }
    // null terminate cmd buffer (so that it will print correctly)
    *cursor = '\0';

    // Deal with an error from the read call
    if (!rv) {
        count = -errno;
    }

    return count;
}

/* TODO: Check is a file matches a glob.
 *
 * This function takes in a simple file glob (such as '*.c')
 * and a file name, and returns 1 if it matches, and 0 if not.
 */
static int glob_matches(const char *glob, char *name) {
    // preliminary check
    if (glob[0] != '*') {
        return 0;
    }

    // point to the extension part of the glob
    const char *extension = glob + 1;
    size_t ext_len = strlen(extension);
    size_t name_len = strlen(name);

    // check if the name is shorter than the extension
    if (name_len < ext_len) {
        return 0;
    }

    // Compare the end of the name with the extension
    return strcmp(name + name_len - ext_len, extension) == 0;
}

/* TODO: Expand a file glob.
 *
 * This function takes in a simple file glob (such as '*.c')
 * and expands it to all matching files in the current working directory.
 * If the glob does not match anything, pass the glob through to the application as an argument.
 *
 * glob: The glob must be simple - only one asterisk as the first character,
 * with 0+ non-special characters following it.
 *
 * buf: A double pointer to scratch buffer allocated by the caller and good for the life of the command,
 *      which one may place expanded globs in.  The double pointer allows you to update the offset
 *      into the buffer.
 *
 * bufsize: _pointer_ to the remaining space in the buffer.  If too many files match, this space may be exceeded,
 *      in which case, the function returns -ENOSPC
 *
 * commands: Same argument as passed to parse_line
 *
 * pipeline_idx: Index into command array for the current pipeline
 *
 * arg_idx: _Pointer_ to the current argument index.  May be incremented as
 *         a glob is expanded.
 *
 * Returns 0 on success, -errno on error
 */
static int expand_glob(char *glob,
                       char **buf,
                       size_t *bufsize,
                       char *commands[MAX_PIPELINE][MAX_ARGS],
                       int pipeline_idx,
                       int *arg_idx) {
    // Check for valid glob pattern
    if (glob[0] != '*' || strlen(glob) < 2) {
        return -EINVAL;
    }

    DIR *dirp;
    struct dirent *dp;

    // open current directory
    if ((dirp = opendir(".")) == NULL) {
        return -errno;
    }

    while ((dp = readdir(dirp)) != NULL) {
        // check files if they match the glob pattern
        if (glob_matches(glob, dp->d_name)) {
            size_t len = strlen(dp->d_name);
            if (*bufsize < len + 1) { // need to check buffer space
                closedir(dirp);
                return -ENOSPC;
            }

            // Copy file name to buffer and update commands array
            strcpy(*buf, dp->d_name);
            commands[pipeline_idx][*arg_idx] = *buf;
            (*buf) += len + 1;     // move buffer pointer
            (*bufsize) -= len + 1; // decrement remaining buffer size
            (*arg_idx)++;          // increment argument index
        }
    }

    closedir(dirp);
    return 0;
}

/* Parse one line of input.
 *
 * This function should populate a two-dimensional array of commands
 * and tokens.  The array itself should be pre-allocated by the
 * caller.
 *
 * The first level of the array is each stage in a pipeline, at most MAX_PIPELINE long.
 * The second level of the array is each argument to a given command, at most MAX_ARGS entries.
 * In each command buffer, the entry after the last valid entry should be NULL.
 * After the last valid pipeline buffer, there should be one command entry with just a NULL.
 *
 * For instance, a simple command like "cd" should parse as:
 *  commands[0] = ["cd", '\0']
 *  commands[1] = ['\0']
 *
 * The first "special" character to consider is the vertical bar, or "pipe" ('|').
 * This splits a single line into multiple sub-commands that form the pipeline.
 * We will implement pipelines in lab 2, but for now, just use this character to delimit
 * commands.
 *
 * For instance, the command: "ls | grep foo\n" should be broken into:
 *
 * commands[0] = ["ls", '\0']
 * commands[1] = ["grep", "foo", '\0']
 * commands[2] = ['\0']
 *
 * Hint: Make sure to remove the newline at the end
 *
 * Hint: Make sure the implementation is robust to extra whitespace, like: "grep      foo"
 *       should still be parsed as:
 *
 * commands[0] = ["grep", "foo", '\0']
 * commands[1] = ['\0']
 *
 * This function should ignore anything after the '#' character, as
 * this is a comment.
 *
 * Finally, the command should identify file redirection characters ('<' and '>').
 * The string right after these tokens should be returned using the special output
 * parameters "infile" and "outfile".  You can assume there is at most one '<' and one
 * '>' character in the entire inbuf.
 *
 * For example, in input: "ls > out.txt", the return should be:
 *   commands[0] = ["ls", '\0']
 *   commands[1] = ['\0']
 *   outfile = "out.txt"
 *
 * Hint: Be sure your implementation is robust to arbitrary (or no) space before or after
 *       the '<' or '>' characters.  For instance, "ls>out.txt" and "ls      >      out.txt"
 *       are both syntactically valid, and should parse identically to "ls > out.txt".
 *       Similarly, "ls|grep foo" is also syntactically valid.
 *
 * You do not need to handle redirection of other handles (e.g., "foo 2>&1 out.txt").
 *
 * inbuf: a NULL-terminated buffer of input.
 *        This buffer may be changed by the function
 *        (e.g., changing some characters to \0).
 *
 * length: the length of the string in inbuf.  Should be
 *         less than the size of inbuf.
 *
 * commands: a two-dimensional array of character pointers, allocated by the caller, which
 *           this function populates.
 *
 * scratch: A caller-allocated buffer that can be used for scratch space, such as
 *          expanding globs in the challenge problems.  You may not need to use this
 *          for the core assignment.
 *
 * scratch_len: Size of the scratch buffer
 *
 * return value: Number of entries populated in commands (1+, not counting the NULL),
 *               or -errno on failure.
 *
 *               In the case of a line with no actual commands (e.g.,
 *               a line with just comments), return 0.
 */

#define DELIM " #|><\n"

int parse_line(char *inbuf, size_t length,
               char *commands[MAX_PIPELINE][MAX_ARGS], char **infile,
               char **outfile, char *scratch, size_t scratch_len) {

    (void) scratch; // Unused, suppress warning
    (void) scratch_len; // Unused, suppress warning
    (void) &expand_glob; // Unused, suppress warning

    /* Handle the trivial cases that would be easy to handle immediately that
     * are valid */
    if (inbuf == NULL || length == 0 || inbuf[0] == '#') {
        return 0;
    }

    /* We are approaching this problem with use of strpbrk() to find the next
     * delimiter in the string.
     *
     * As strpbrk() returns a pointer to the first occurence of the
     * outlined delimiters, we can receive the index of the next delimiter,
     * and on a case by case basis, handle the different special operations,
     * of which are detailed per switch case. Trivially we increment the pointer
     * to the next character after the delimiter, and continue parsing the
     * input until null termination.
     * */
    int pipe_idx = 0, arg_idx = 0;
    char *current = inbuf;
    char *end = inbuf + length;
    char *next_delim;

    while (current < end) {
        /* Per iteration, we find the index of the next delimiter, and then
         * switch/case on the character to handle them per their discrete spec */
        next_delim = strpbrk(current, DELIM);
        if (!next_delim) break;
        switch (*next_delim) {
            case '#':
                /* The comment delimiter is a trivial case, as we can simply
                 * null terminate the string at the comment, and return the
                 * number of commands parsed and terminate the operation
                 * */
                return pipe_idx >= 0 ? pipe_idx + 1 : -errno;

            case '|':
                /* The pipe delimiter is a straightforward case; we allocate the
                 * substring to the commands table, and then increment the pipe_idx
                 * and reset the arg_idx to 0 to build up the rows of piped
                 * commands
                 * */
                if (next_delim - current > 0) {
                    allocate_and_copy_substring(&commands[pipe_idx][arg_idx],
                                                current,
                                                0,
                                                next_delim - current);
                }
                pipe_idx++;
                arg_idx = 0;
                break;

            case '>':
            case '<':
                /* The following redirections are slightly more finicky because its
                 * dependency on a following argument, and the interaction
                 * thereafter with any possible other special character.
                 *
                 * Extraction of the initial argument is a given, but the following
                 * steps require a bit more attention. I approach this by first
                 * detecting the following delimiter, and then extracting the
                 * following argument to the respective infile or outfile; if the next
                 * delimiter is a pipe, we handle the special case by preemptively
                 * setting up the next pipe_idx and arg_idx, and then continuing
                 * parsing the input.
                 * */
                char redirection_char = *next_delim;

                // skip the redirection character and any following whitespace
                current = next_delim + 1;
                while (current < end && isspace((unsigned char) *current)) {
                    current++;
                }

                // find the end of the filename (next delimiter or end of input)
                next_delim = strpbrk(current, DELIM);
                if (!next_delim) {
                    next_delim = end; // if no more delimiters, use end of input
                }

                // extract the filename for redirection
                char **redirection_target = (redirection_char == '>') ? outfile : infile;
                allocate_and_copy_substring(redirection_target, current,
                                            0, next_delim - current);

                // adjust current to continue parsing after the filename
                current = next_delim;
                break;

            case ' ':
            case '\n':
                /* Spaces and newlines (rare to find newlines) all function
                 * more or less the same for our limited use case; every successive
                 * argument is an additional argument, which will be allocated to
                 * the commands table
                 * */
                if (next_delim - current > 0) {
                    allocate_and_copy_substring(&commands[pipe_idx][arg_idx],
                                                current,
                                                0,
                                                next_delim - current);
                    arg_idx++;
                }
                break;
        }

        current = next_delim + 1;
    }

    /* For redundancy, we check if the current pointer is less than the end
     * pointer, and if so, we allocate the remaining string to the commands
     * table */
    if (current < end) {
        allocate_and_copy_substring(&commands[pipe_idx][arg_idx],
                                    current,
                                    0,
                                    end - current);
        arg_idx++;
    }

    /* We null terminate the commands table, and return the number of commands
     * parsed */
    commands[pipe_idx][arg_idx] = NULL;

    // this could be integrated into the above loop,
    // but if it works, it works...
    for (int p = 0; p <= pipe_idx; p++) {
        for (int a = 0; commands[p][a] != NULL; a++) {
            // Check if the argument is a glob pattern
            if (strchr(commands[p][a], '*') != NULL) {
                char *glob = commands[p][a];
                size_t bufsize = scratch_len;

                // Call expand_glob to expand the glob pattern
                int result = expand_glob(glob, &scratch, &bufsize,
                                         commands, p, &a);
                if (result < 0) {
                    return -errno;
                }
            }
        }
    }

    return pipe_idx >= 0 ? pipe_idx + 1 : -errno;
}