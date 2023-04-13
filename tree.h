#include "utils.h"

frequency_node_ptr build_huffman_tree(binary_heap_ptr);
bool find_symbol_in_tree(frequency_node_ptr, huffman_code_ptr, uint8_t, bool);
void write_tree(FILE*, frequency_node_ptr, uint16_t);
void prepare_tree_to_write(frequency_node_ptr, uint8_t*);
void tree_check(const char*, frequency_node_ptr, uint16_t, size_t, frequency_node_ptr);
frequency_node_ptr restore_tree(frequency_node_ptr, uint8_t*);
