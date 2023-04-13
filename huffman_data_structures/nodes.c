#include "nodes.h"

void init_node(frequency_node_ptr source,
               frequency_node_ptr left,
               frequency_node_ptr right,
               uint64_t frequency,
               uint8_t symbol_code) {
    CHKPTR(source);
    source->left = left;
    source->right = right;
    source->frequency = frequency;
    source->symbol_code = symbol_code;
}

/* void free_node(frequency_node_ptr *nptr) {
    CHKPTR(nptr);
    FREE((*nptr)->left);
    FREE((*nptr)->right);
    FREE(*nptr);
} */

void make_new_node(frequency_node_ptr new, frequency_node_ptr left, frequency_node_ptr right) {
    CHKPTR(left);
    CHKPTR(right);
    uint64_t frequency_sum = left->frequency + right->frequency;
    init_node(new, left, right, frequency_sum, 0);
}

void free_nodes(frequency_node_ptr *nptr) {
    CHKPTR(nptr);
    if (!((*nptr)->left) && (!(*nptr)->right)) { /* leaf - allocated in stack */
        *nptr = NULL;
        return;
    }
    if ((*nptr)->left) free_nodes(&((*nptr)->left));
    if ((*nptr)->right) free_nodes(&((*nptr)->right));
    FREE(*nptr);
}
