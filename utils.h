#include <stdio.h>
#include <stdbool.h>
#include "huffman_data_structures/nodes.h"
#include "huffman_data_structures/priority_queue.h"

typedef struct huffman_code* huffman_code_ptr;

typedef struct huffman_code {
    uint64_t code;
    uint8_t length;
} huffman_code;

void get_frequencies(frequency_node*, FILE*);
uint8_t write_header(FILE*, uint16_t, uintmax_t, uint8_t, uint8_t);
void header_check(const char*, uint16_t, uintmax_t, uint8_t, uint8_t);
void check_fingerprint(uint8_t*);
