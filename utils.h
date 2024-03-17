#include <stdlib.h>

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