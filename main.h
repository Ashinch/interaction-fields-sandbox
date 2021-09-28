//
// Created by Ashinch on 2021/9/26.
//

#ifndef INTERACTION_FIELDS_SANDBOX_MAIN_H
#define INTERACTION_FIELDS_SANDBOX_MAIN_H

#include <string.h>
#include <stdlib.h>
#include <assert.h>

enum {
    ARGUMENTS_ERROR         = -1,
    INIT_SECCOMP_ERROR      = -2,
    ADD_BLACK_LIST_ERROR    = -3,
    LOAD_BLACK_LIST_ERROR   = -4,
};

char **split(char *a_str, const char a_delim) {
    char **result = 0;
    size_t count = 0;
    char *tmp = a_str;
    char *last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char *) * count);

    if (result) {
        size_t idx = 0;
        char *token = strtok(a_str, delim);

        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

#endif //INTERACTION_FIELDS_SANDBOX_MAIN_H
