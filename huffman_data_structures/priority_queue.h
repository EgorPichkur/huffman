#ifndef PRIORITY_QUEUE_H 
#define PRIORITY_QUEUE_H 

#include "nodes.h"

#define SWAP(x, y)               \
    do {                         \
        frequency_node_ptr temp; \
        temp = x;                \
        x = y;                   \
        y = temp;                \
    } while (0)                  \

#define PARENT(x) (x-1)/2

typedef struct binary_heap* binary_heap_ptr;

struct binary_heap {
    frequency_node_ptr *heap_array;
    uint16_t capacity;
    uint16_t size;
};

void init_heap(binary_heap_ptr, uint16_t);
void free_heap(binary_heap_ptr*);
int8_t insert(binary_heap_ptr, frequency_node_ptr);
frequency_node_ptr extract_min(binary_heap_ptr);
void min_heapify(binary_heap_ptr, uint16_t);

#endif
