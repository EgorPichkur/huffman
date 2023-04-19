#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#define FREE(ptr)          \
    do {                   \
        if (ptr != NULL) { \
            free(ptr);     \
            ptr = NULL;    \
        }                  \
    } while (0)

#define CHKPTR(ptr)        \
    do {                   \
        if (ptr == NULL)   \
            printf("Bad ptr in %s:%u\n", __FILE__, __LINE__); \
    } while (0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define error(msg) do { fprintf(stderr, "[ERR] file %s/line %d: %s\n", __FILE__, __LINE__, msg); exit(EXIT_FAILURE); } while(0)
#define ERR_MSG_BUFFER 256

#ifdef DEBUG
    #define DBG_LOG(fmt, ...) do { fprintf(stderr, "[LOG] file %s/line %d: " fmt, __FILE__, __LINE__, __VA_ARGS__); } while(0)
#else
    #define DBG_LOG(fmt, ...) {}
#endif

#endif
