#include <string.h>
#include <assert.h>
#include <common.h>
#include "tree.h"

frequency_node_ptr build_huffman_tree(binary_heap_ptr priority_queue) {
    frequency_node_ptr left = NULL;
    frequency_node_ptr right = NULL;
    while ((left = extract_min(priority_queue))) {
        right = extract_min(priority_queue);
        if (!right) break;
        frequency_node_ptr new = malloc(sizeof(*new));
        CHKPTR(new);
        make_new_node(new, left, right);
        insert(priority_queue, new);
    }
    CHKPTR(left);
    return left;
}

/* symbol_data must be initialized before the call */
bool find_symbol_in_tree(frequency_node_ptr root, huffman_code_ptr symbol_data, uint8_t symbol, bool found) {
    if (symbol_data->length > 64) {
        uint8_t err_msg[120];
        sprintf((char*)err_msg, "Huffman code for the symbol %x is more than 64 bits! Aborting...\n", symbol);
        error(err_msg);
    }

    if (found) return found;

    if (root->left) {
        symbol_data->length++;
        symbol_data->code <<= 1;
        found = find_symbol_in_tree(root->left, symbol_data, symbol, found);
        if (found) return found;
    }

    if (root->right) {
        symbol_data->length++;
        symbol_data->code <<= 1;
        symbol_data->code |= 1;
        found = find_symbol_in_tree(root->right, symbol_data, symbol, found);
        if (found) return found;
    }

    if (!(root->left) && !(root->right)) {
        if (root->symbol_code == symbol) {
            DBG_LOG("Found symbol %x, CODE: %lX, LENGTH %hu\n", symbol, symbol_data->code, symbol_data->length);
            found = true;
        }
        else {
            found = false;
        }
    }

    if (!found) {
        symbol_data->length--;
        symbol_data->code >>= 1;
    }

    return found;
}

void write_tree(FILE* ofile, frequency_node_ptr tree, uint16_t tree_size) {
    uint8_t buf[tree_size];
    memset(buf, 0, tree_size);
    prepare_tree_to_write(tree, buf);
    size_t written = 0;
    written = fwrite(buf, ARRAY_SIZE(buf), sizeof(*buf), ofile);
    if (written != sizeof(*buf)) {
        uint8_t err_msg[120];
        sprintf((char*)err_msg, "fwrite() failed! Expected %zu bytes, but got %zu bytes\n", sizeof(*buf), written);
        error(err_msg);
    }
}

void prepare_tree_to_write(frequency_node_ptr root, uint8_t *buffer) {
    static uint16_t current_byte = 0;
    static uint8_t bits_left_in_byte = 8;
    static uint8_t bits_left_in_symbol = 8;

    uint8_t temp = 0;
    if (root->left || root->right) {
        temp = 0;
        bits_left_in_byte--;
        temp <<= bits_left_in_byte;
        buffer[current_byte] |= temp;
        if (!bits_left_in_byte) {
            bits_left_in_byte = 8;
            current_byte++;
        }
        if (root->left) prepare_tree_to_write(root->left, buffer);
        if (root->right) prepare_tree_to_write(root->right, buffer);
    }
    else {
        temp = 1;
        bits_left_in_byte--;
        temp <<= bits_left_in_byte;
        buffer[current_byte] |= temp;
        if (!bits_left_in_byte) {
            bits_left_in_byte = 8;
            current_byte++;
        }
        while (bits_left_in_symbol) {
            temp = 0;
            if (bits_left_in_byte < bits_left_in_symbol) {
                temp = root->symbol_code >> (8 - bits_left_in_byte);
                bits_left_in_symbol -= bits_left_in_byte;
                bits_left_in_byte = 0;
            }
            else {
                temp = root->symbol_code << (8 - bits_left_in_symbol);
                bits_left_in_byte -= bits_left_in_symbol;
                bits_left_in_symbol = 0;
            }
            buffer[current_byte] |= temp;
            if (!bits_left_in_byte) {
                bits_left_in_byte = 8;
                current_byte++;
            }
        }
        bits_left_in_symbol = 8;
    }
}

void tree_check(const char* ofile_name, frequency_node_ptr tree_root, uint16_t tree_size, size_t offset, frequency_node_ptr nodes) {
    FILE *ofile = fopen(ofile_name, "rb");
    uint8_t err_msg[ERR_MSG_BUFFER];
    if (!ofile) {
        sprintf((char*)err_msg, "fopen failed! Cannot open the file %s\n", ofile_name);
        error(err_msg);
    }
    fseek(ofile, offset, SEEK_SET);

    frequency_node_ptr current_root = NULL;
    uint8_t buf[tree_size];
    size_t ret = fread(buf, ARRAY_SIZE(buf), sizeof(*buf), ofile);
    if (ret != sizeof(*buf)) {
        if (ferror(ofile)) {
            sprintf((char*)err_msg, "fread() failed! Expected %zu bytes, got %zu bytes\n", sizeof(*buf), ret);
            error(err_msg);
        }
    }
    current_root = restore_tree(current_root, buf);
    huffman_code hfmncd1, hfmncd2;
    for (uint16_t i = 0; i < 256; i++) {
        if (!nodes[i].frequency) continue;
        hfmncd1.code = 0;
        hfmncd1.length = 0;
        hfmncd2.code = 0;
        hfmncd2.length = 0;
        find_symbol_in_tree(tree_root, &hfmncd1, nodes[i].symbol_code, false);
        find_symbol_in_tree(current_root, &hfmncd2, nodes[i].symbol_code, false);
        assert(hfmncd1.code == hfmncd2.code);
        assert(hfmncd1.length == hfmncd2.length);
    }
    free_nodes(&current_root);
    fclose(ofile);
}

frequency_node_ptr restore_tree(frequency_node_ptr root, uint8_t *buffer) {
    static uint16_t current_byte = 0;
    static uint8_t bits_left_in_byte = 8;
    static uint8_t bits_left_in_symbol = 8;
    static uint8_t symbol = 0;

    uint8_t bit = (buffer[current_byte] >> --bits_left_in_byte) & 1;
    if (!bits_left_in_byte) {
        bits_left_in_byte = 8;
        current_byte++;
    }
    if (!bit) {
        root = malloc(sizeof(*root));
        CHKPTR(root);
        root->left = restore_tree(root->left, buffer);
        CHKPTR(root->left);
        root->right = restore_tree(root->right, buffer);
        CHKPTR(root->right);
        return root;
    }
    else {
        symbol = 0;
        root = malloc(sizeof(*root));
        CHKPTR(root);
        root->left = NULL;
        root->right = NULL;
        while (bits_left_in_symbol) {
            if (bits_left_in_byte < bits_left_in_symbol) {
                symbol |= buffer[current_byte] << (8 - bits_left_in_byte);
                bits_left_in_symbol -= bits_left_in_byte;
                bits_left_in_byte = 0;
            }
            else {
                symbol |= buffer[current_byte] >> (8 - bits_left_in_symbol);
                bits_left_in_byte -= bits_left_in_symbol;
                bits_left_in_symbol = 0;
            }
            if (!bits_left_in_byte) {
                bits_left_in_byte = 8;
                current_byte++;
            }
        }
        bits_left_in_symbol = 8;
        root->symbol_code = symbol;
        return root;
    }
}
