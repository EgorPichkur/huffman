#ifndef NODES_H
#define NODES_H

#include "pointers.h"
#include <stdint.h>

typedef struct frequency_node* frequency_node_ptr;

/* TODO: check with packed struct */
typedef struct frequency_node {
    frequency_node_ptr left;
    frequency_node_ptr right;
    uint64_t frequency;
    uint8_t symbol_code;
} frequency_node;

void init_node(frequency_node_ptr,
	       frequency_node_ptr,
               frequency_node_ptr,
               uint64_t,
               uint8_t);
/* void free_node(frequency_node_ptr*); */
void make_new_node(frequency_node_ptr, frequency_node_ptr, frequency_node_ptr);
void free_nodes(frequency_node_ptr*);

#endif
