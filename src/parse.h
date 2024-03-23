#include "utils/constants.h"
#include <stddef.h>

int read_one_line(int input_fd, char *buf, size_t size);

int parse_line(char *inbuf, size_t length, char *commands[MAX_PIPELINE][MAX_ARGS],
               char **infile, char **outfile,
               char *scratch, size_t scratch_len);