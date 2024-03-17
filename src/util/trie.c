/*
 * Implementation of trie.h.
 */

#include "trie.h"

Trie* get_node(void) {
    Trie *p_node = (Trie*) malloc(sizeof(Trie));
    p_node->end = false;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        p_node->children[i] = NULL;
    }
    return p_node;
}

void insert(Trie *root, const char *key) {
    Trie *p_crawl = root;
    for (int level = 0; level < strlen(key); level++) {
        int index = (int) key[level];
        if (!p_crawl->children[index]) {
            p_crawl->children[index] = get_node();
        }
        p_crawl = p_crawl->children[index];
    }
    p_crawl->end = true;
}

bool is_child_node(Trie* root) {
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (root->children[i]) {
            return false;
        }
    }
    return true;
}

void recommend_suggestion(Trie* root, char* curr_prefix, char** suggestions, int* count) {
    if (root->end) {
        suggestions[*count] = strdup(curr_prefix);
        (*count)++;
    }

    if (is_child_node(root)) {
        return;
    }

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (root->children[i]) {
            char next_prefix[256];  // Adjust size as needed
            snprintf(next_prefix, sizeof(next_prefix), "%s%c", curr_prefix, i);
            recommend_suggestion(root->children[i], next_prefix, suggestions, count);
        }
    }
}

char** find_suggestion(Trie* root, const char* query, int* count) {
    Trie* p_crawl = root;
    *count = 0;

    for (int level = 0; level < strlen(query); level++) {
        int index = (int) query[level];
        if (!p_crawl->children[index]) {
            return NULL;
        }
        p_crawl = p_crawl->children[index];
    }

    bool is_word = (p_crawl->end && is_child_node(p_crawl));
    char** suggestions = (char**)malloc(100 * sizeof(char*));

    if (!is_word) {
        recommend_suggestion(p_crawl, (char*) query, suggestions, count);
    }

    return suggestions;
}

void populate_trie(Trie* root, char** paths) {
    DIR *dir;
    struct dirent *ent;

    for (int i = 0; paths[i] != NULL; i++) {
        dir = opendir(paths[i]);
        if (dir != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                // is file exec?
                char path[PATH_MAX];
                snprintf(path, sizeof(path), "%s/%s", paths[i], ent->d_name);

                if (access(path, X_OK) == 0) {
                    insert(root, ent->d_name);
                }
            }
            closedir(dir);
        }
    }
}