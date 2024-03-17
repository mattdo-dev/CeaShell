/* 730406661
 *
 * Implement the trie structure for tabbing completion
 */
#ifndef TRIE_H
#define TRIE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#define ALPHABET_SIZE 128

/**
 * Struct representing a trie node.
 *
 * Each node contains `ALPHABET_SIZE` pointers to its
 * children and a boolean `end` to mark the end of a word.
 */
typedef struct Trie {
    struct Trie *children[ALPHABET_SIZE];
    bool end;
} Trie;

/**
 * Allocates and returns a new trie node.
 * Initializes all children to NULL and sets the end flag to false.
 *
 * @return A pointer to the newly created Trie node.
 */
Trie* get_node(void);

/**
 * Inserts a key into the trie.
 * Iterates over each character in the key and
 * creates new nodes as necessary.
 * Marks the last node as the end of the key.
 *
 * @param root A pointer to the root of the Trie.
 * @param key The string to be inserted into the Trie.
 */
void insert(Trie *root, const char *key);

/**
 * Checks if a Trie node has any children.
 *
 * @param root A pointer to a Trie node.
 * @return `true` if the node has no children, `false` otherwise.
 */
bool is_child_node(Trie* root);

/**
 * Recursively generates suggestions for the current prefix.
 * If the current Trie node marks the end of a word, it adds
 * the prefix to the suggestions array.
 *
 * @param root A pointer to the current Trie node.
 * @param curr_prefix The current prefix being constructed.
 * @param suggestions Array of strings to store the suggestions.
 * @param count Pointer to an integer tracking the number of suggestions.
 */
void recommend_suggestion(Trie* root, char* curr_prefix, char** suggestions, int* count);

/**
 * Finds suggestions based on a given query string.
 * Traverses the Trie based on the query. If query is a part of
 * the Trie, it calls `recommend_suggestion` to find completions.
 *
 * @param root A pointer to the root of the Trie.
 * @param query The query string for which suggestions are to be found.
 * @param count Pointer to an integer to store the number of suggestions.
 * @return An array of string suggestions.
 */
char** find_suggestion(Trie* root, const char* query, int* count);


/**
 * Populates the Trie with executable file names found in given directories.
 * Iterates through each directory and inserts executable file names into the Trie.
 *
 * @param root A pointer to the root of the Trie.
 * @param paths Array of strings representing directory paths.
 */
void populate_trie(Trie* root, char** paths);

#endif //TRIE_H
