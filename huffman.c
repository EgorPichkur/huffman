#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "pointers.h"
#include "tree.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define error(msg) do { fprintf(stderr, "[ERR] file %s/line %d: %s\n", __FILE__, __LINE__, msg); exit(EXIT_FAILURE); } while(0)
#define ERR_MSG_BUFFER 256

bool DEBUG = false;

uintmax_t write_data(FILE*, FILE*, size_t, huffman_code_ptr);
uint8_t write_symbol(uint64_t*, uintmax_t*, uint8_t*, huffman_code, uint16_t);
uint64_t swap_bytes_order_64(uint64_t num);
void write_decompressed_data(FILE*, FILE*, frequency_node_ptr, size_t, uint8_t);

int main(int argc, char *argv[]) {

    int opt;
    bool compress = false;
    bool decompress = false;
    uint8_t *ofile_name = NULL;
    uint8_t *ifile_name = NULL;
    frequency_node nodes[256] = {0};
    FILE *ifile_fd = NULL;
    FILE *ofile_fd = NULL;
    huffman_code codes[256] = {0};
    uint8_t err_msg[ERR_MSG_BUFFER];

    /* huffman ifile [-c|-x] ofile */
    while ((opt = getopt(argc, argv, "c:x:")) != -1) {
        switch(opt) {
            case 'c': compress = true; ofile_name = (uint8_t*)optarg; break;
            case 'x': decompress = true; ofile_name = (uint8_t*)optarg; break;
            default:
                fprintf(stderr, "Usage: %s ifile [-c|-x] ofile\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if ((compress == decompress) || (argc - optind != 1) || (ofile_name[0] == '-')) {
        fprintf(stderr, "Usage: %s ifile [-c|-x] ofile\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (DEBUG) printf("ofile: %s\n", ofile_name);
    CHKPTR(ofile_name);

    ifile_name = (uint8_t*)argv[optind];
    if (DEBUG) printf("ifile: %s\n", ifile_name);
    CHKPTR(ifile_name);

    if (compress) {
        ifile_fd = fopen((const char*)ifile_name, "rb");
        if (!ifile_fd) {
            sprintf((char*)err_msg, "fopen() failed! Cannot open %s file\n", ifile_name);
            error(err_msg);
        }

        get_frequencies(nodes, ifile_fd);

        uint16_t number_of_symbols = 0;
        for (uint16_t i = 0; i < 256; i++) {
            if (!nodes[i].frequency) continue;
            number_of_symbols++;
            if (DEBUG) printf("FREQ: %ld, SYMBOL: %x\n", nodes[i].frequency, nodes[i].symbol_code);
        }

        binary_heap_ptr pr_queue = malloc(sizeof(*pr_queue));
        CHKPTR(pr_queue);
        init_heap(pr_queue, number_of_symbols);
        for (uint16_t i = 0; i < 256; i++) {
            if (!nodes[i].frequency) continue;
            insert(pr_queue, &nodes[i]);
        }
        frequency_node_ptr tree_root = build_huffman_tree(pr_queue);
        huffman_code hfmncd;
        size_t compressed_data_size = 0;
        uint16_t tree_size = 0;
        uint8_t significant_bits_last_tree_byte = 0;
        uint8_t significant_bits_last_data_byte = 0;
        for (uint16_t i = 0; i < 256; i++) {
            if (!nodes[i].frequency) continue;
            hfmncd.code = 0;
            hfmncd.length = 0;
            find_symbol_in_tree(tree_root, &hfmncd, nodes[i].symbol_code, false);
            codes[i].code = hfmncd.code;
            codes[i].length = hfmncd.length;
            compressed_data_size += hfmncd.length * nodes[i].frequency;
        }
        significant_bits_last_data_byte = compressed_data_size % 8;
        compressed_data_size /= 8; /* bits to bytes */
        if (significant_bits_last_data_byte) compressed_data_size++;
        tree_size = 10 * number_of_symbols - 1;
        significant_bits_last_tree_byte = tree_size % 8;
        tree_size /= 8; /* bits to bytes */
        tree_size++;
        if (DEBUG) {
            printf("COMPRESSED DATA SIZE (BYTES): %ld\n", compressed_data_size);
            printf("SIGNIFICANT BITS IN LAST DATA BYTE: %hu\n", significant_bits_last_data_byte);
            printf("TREE SIZE: %hu\n", tree_size);
            printf("SIGNIFICANT BITS IN LAST TREE BYTE: %hu\n", significant_bits_last_tree_byte);
        }

        ofile_fd = fopen((const char*)ofile_name, "wb");
        if (!ofile_fd) {
            sprintf((char*)err_msg, "fopen() failed! Cannot open %s file\n", ifile_name);
            error(err_msg);
        }

        size_t offset = 0;
        offset = write_header(ofile_fd, tree_size, compressed_data_size, significant_bits_last_tree_byte, significant_bits_last_data_byte);
        if (DEBUG) {
            fclose(ofile_fd); /* force write */
            header_check((const char*)ofile_name, tree_size, compressed_data_size, significant_bits_last_tree_byte, significant_bits_last_data_byte);
            ofile_fd = fopen((const char*)ofile_name, "ab");
            if (!ofile_fd) {
                sprintf((char*)err_msg, "fopen() failed! Cannot open %s file\n", ifile_name);
                error(err_msg);
            }
        }

        write_tree(ofile_fd, tree_root, tree_size);
        offset += tree_size;

        if (DEBUG) {
            fclose(ofile_fd);
            header_check((const char*)ofile_name, tree_size, compressed_data_size, significant_bits_last_tree_byte, significant_bits_last_data_byte);
            tree_check((const char*)ofile_name, tree_root, tree_size, offset - tree_size, nodes);
            ofile_fd = fopen((const char*)ofile_name, "ab");
            if (!ofile_fd) {
                sprintf((char*)err_msg, "fopen() failed! Cannot open %s file\n", ifile_name);
                error(err_msg);
            }
        }
        fseek(ifile_fd, 0, SEEK_SET);
        size_t data_size = offset;
        offset += write_data(ifile_fd, ofile_fd, offset, codes);
        fclose(ifile_fd);
        fclose(ofile_fd);
        if (DEBUG) printf("COMPRESSED DATA WRITTEN: %zu\n", offset - data_size);
        free_heap(&pr_queue);
        free_nodes(&tree_root);
    }

    if (decompress) {
        ifile_fd = fopen((const char*)ifile_name, "rb");
        if (!ifile_fd) {
            sprintf((char*)err_msg, "fopen() failed! Cannot open %s file\n", ifile_name);
            error(err_msg);
        }
        ofile_fd = fopen((const char*)ofile_name, "wb");
        if (!ofile_fd) {
            sprintf((char*)err_msg, "fopen() failed! Cannot open %s file\n", ifile_name);
            error(err_msg);
        }
        uint16_t tree_size_from_header;
        uintmax_t data_size_from_header;
        // uint8_t tree_last_bits_from_header;
        uint8_t data_last_bits_from_header;

        uint8_t buffer[5];
        size_t read = 0;
        read = fread(buffer, ARRAY_SIZE(buffer), sizeof(*buffer), ifile_fd);
        if (read != sizeof(*buffer)) {
            if (ferror(ifile_fd)) {
                fprintf(stderr, "fread() failed (header reading): %zu\n", read);
                exit(EXIT_FAILURE);
            }
        }

        check_fingerprint(buffer);

        uint8_t metadata_size = buffer[4];
        uint8_t metadata_buffer[metadata_size];
        read = fread(metadata_buffer, ARRAY_SIZE(metadata_buffer), sizeof(*metadata_buffer), ifile_fd);
        if (read != sizeof(*metadata_buffer)) {
            if (ferror(ifile_fd)) {
                sprintf((char*)err_msg, "fread() failed! Expected %zu, got %zu\n", sizeof(*metadata_buffer), read);
                error(err_msg);
            }
        }
        uint16_t tree_size_buf;
        memcpy(&tree_size_buf, metadata_buffer, sizeof(tree_size_buf));
        memcpy(&data_size_from_header, metadata_buffer + sizeof(tree_size_buf), metadata_size - sizeof(tree_size_buf));
        data_last_bits_from_header = (tree_size_buf >> 10) & 7;
        // tree_last_bits_from_header = (tree_size_buf >> 13) & 7;
        tree_size_from_header = tree_size_buf & 0x1FF;

        frequency_node_ptr current_root = NULL;
        uint8_t buf[tree_size_from_header];
        size_t ret = fread(buf, ARRAY_SIZE(buf), sizeof(*buf), ifile_fd);
        if (ret != sizeof(*buf)) {
            if (ferror(ifile_fd)) {
                sprintf((char*)err_msg, "fread() failed! Expected %zu, got %zu\n", sizeof(*buf), ret);
                error(err_msg);
            }
        }
        current_root = restore_tree(current_root, buf);
        write_decompressed_data(ifile_fd, ofile_fd, current_root, data_size_from_header, data_last_bits_from_header);
        free_nodes(&current_root);
        fclose(ifile_fd);
        fclose(ofile_fd);
    }

    exit(EXIT_SUCCESS);
}

uintmax_t write_data(FILE* ifile, FILE* ofile, size_t offset, huffman_code_ptr tree) {
    uintmax_t written = 0;
    size_t read = 0;
    uint8_t read_buffer[1024] = {0};
    uint64_t write_buffer[1024] = {0};
    uintmax_t current_frame = 0;
    uint8_t bits_left_in_current_frame = 64;
    huffman_code hfmncd;
    bool chunk_written = false;
    fseek(ofile, offset, SEEK_SET);
    while (!feof(ifile)) {
        read = fread(read_buffer, sizeof(*read_buffer), ARRAY_SIZE(read_buffer), ifile);
        if (read != ARRAY_SIZE(read_buffer)) {
            if (ferror(ifile)) {
                fprintf(stderr, "fread() failed: %zu\n", read);
                exit(EXIT_FAILURE);
            }
        }
        chunk_written = false;
        for (uint16_t i = 0; i < read; i++) {
            hfmncd.code = 0;
            hfmncd.length = 0;
            uint8_t written_for_one_symbol = 0;
            hfmncd.code = tree[read_buffer[i]].code;
            hfmncd.length = tree[read_buffer[i]].length;
            if (DEBUG) printf("Found symbol %x, CODE: %lX, LENGTH %hu\n", read_buffer[i], tree[read_buffer[i]].code, tree[read_buffer[i]].length);
            written_for_one_symbol = write_symbol(write_buffer, &current_frame, &bits_left_in_current_frame, hfmncd, ARRAY_SIZE(write_buffer));
            if ((hfmncd.length > written_for_one_symbol) || (current_frame == ARRAY_SIZE(write_buffer))) {
                fwrite(write_buffer, sizeof(*write_buffer), ARRAY_SIZE(write_buffer), ofile);
                written += ARRAY_SIZE(write_buffer) * sizeof(*write_buffer);
                chunk_written = true;
                if (current_frame) current_frame = 0;
                if (bits_left_in_current_frame != 64) bits_left_in_current_frame = 64;
                memset(write_buffer, 0, sizeof(write_buffer));
                while (hfmncd.length != written_for_one_symbol) {
                    hfmncd.length -= written_for_one_symbol;
                    written_for_one_symbol = write_symbol(write_buffer, &current_frame, &bits_left_in_current_frame, hfmncd, ARRAY_SIZE(write_buffer));
                }
            }
            else chunk_written = false;
        }
    }
    if (!chunk_written) {
        /* little-endian only? */
        size_t a = 0;
        a = fwrite(write_buffer, sizeof(*write_buffer), current_frame, ofile);
        written += a * sizeof(*write_buffer);
        uint8_t *ptr;
        uint64_t swapped = swap_bytes_order_64(write_buffer[current_frame]);
        write_buffer[current_frame] = swapped;
        ptr = (uint8_t*)(&write_buffer[current_frame]);
        int8_t significant_bits_in_frame = 64 - bits_left_in_current_frame;
        while (significant_bits_in_frame > 0) {
            fwrite(ptr, sizeof(*ptr), 1, ofile);
            written += 1;
            ptr++;
            significant_bits_in_frame -= 8;
        }
        if (DEBUG) printf("DATA WRITTEN: %ld\n", written);
    }
    return written;
}

/* buffer size is number of elements, not bytes */
uint8_t write_symbol(uint64_t *buffer, uintmax_t *current_frame, uint8_t *bits_left_in_cur_frame, huffman_code hfmncd, uint16_t buffer_size) {
    uint8_t length = hfmncd.length;
    while (length) {
        uint64_t mask = 0;
        uint64_t temp = 0;
        if (*current_frame < buffer_size) {
            if (*bits_left_in_cur_frame < length) {
                mask = hfmncd.code;
                mask = ~((mask >> length) << length);
                temp = hfmncd.code;
                temp &= mask;
                temp >>= (length - *bits_left_in_cur_frame);
                buffer[*current_frame] |= temp;
                length -= *bits_left_in_cur_frame;
                *bits_left_in_cur_frame = 0;
            }
            else {
                temp = hfmncd.code;
                temp <<= (*bits_left_in_cur_frame - length);
                buffer[*current_frame] |= temp;
                *bits_left_in_cur_frame -= length;
                length = 0;
            }
        }
        else {
            *current_frame = 0;
            *bits_left_in_cur_frame = 64;
            break;
        }

        if (!(*bits_left_in_cur_frame)) {
            (*current_frame)++;
            *bits_left_in_cur_frame = 64;
        }
    }
    return hfmncd.length - length;
}

uint64_t swap_bytes_order_64(uint64_t num) {
    uint64_t swapped = (
            ((num >> 56) & 0xff) | // 1 to 8
            ((num >> 40) & 0xff00) | // 2 to 7
            ((num >> 24) & 0xff0000) | // 3 to 6
            ((num >> 8) & 0xff000000) | // 4 to 5
            ((num << 8) & 0xff00000000) | // 5 to 4
            ((num << 24) & 0xff0000000000) | // 6 to 3
            ((num << 40) & 0xff000000000000) | // 7 to 2
            ((num << 56) & 0xff00000000000000) // 8 to 1
    );
    return swapped;
}

void write_decompressed_data(FILE* ifile, FILE* ofile, frequency_node_ptr tree, size_t data_size, uint8_t bits_in_last_byte) {
    frequency_node_ptr current_root = tree;
    uint64_t read_buffer[1024] = {0};
    uint8_t write_buffer[1024] = {0};
    uint16_t write_pos = 0;
    size_t read_counter = 0;
    size_t processed_data = 0;
    uint8_t bits_to_process, stop_bits;
    uint8_t last_frame_size = 0;
    uint8_t last_frame_buf[8] = {0};
    uint8_t bit;
    while (processed_data < data_size) {
        read_counter = fread(read_buffer, sizeof(*read_buffer), ARRAY_SIZE(read_buffer), ifile);
        processed_data += read_counter * sizeof(*read_buffer);
        if (data_size - processed_data < sizeof(*read_buffer)) {
            last_frame_size = data_size - processed_data;
            processed_data = data_size;
        }
        for (uint16_t i = 0; i < read_counter; i++) {
            bits_to_process = 64;
            stop_bits = 0;
            while (bits_to_process-- > stop_bits) {
                bit = (read_buffer[i] >> bits_to_process) & 1;
                if (!bit) current_root = current_root->left;
                else current_root = current_root->right;
                if (!(current_root->left) && !(current_root->right)) {
                    write_buffer[write_pos++] = current_root->symbol_code;
                    if (write_pos == 1024) {
                        fwrite(write_buffer, ARRAY_SIZE(write_buffer), sizeof(*write_buffer), ofile);
                        write_pos = 0;
                        memset(write_buffer, 0, ARRAY_SIZE(write_buffer));
                    }
                    current_root = tree;
                }
            }
        }
    }
    if (feof(ifile)) fseek(ifile, -last_frame_size, SEEK_END);
    read_counter = fread(last_frame_buf, sizeof(*last_frame_buf), ARRAY_SIZE(last_frame_buf), ifile);
    for (uint16_t i = 0; i < read_counter; i++) {
        if (i == read_counter - 1) {
            stop_bits = 8 - bits_in_last_byte;
            bits_to_process = 8;
        }
        else {
            bits_to_process = 8;
            stop_bits = 0;
        }
        while (bits_to_process-- > stop_bits) {
            bit = (last_frame_buf[i] >> bits_to_process) & 1;
            if (!bit) current_root = current_root->left;
            else current_root = current_root->right;
            if (!(current_root->left) && !(current_root->right)) {
                write_buffer[write_pos++] = current_root->symbol_code;
                if (write_pos == 1024) {
                    fwrite(write_buffer, ARRAY_SIZE(write_buffer), sizeof(*write_buffer), ofile);
                    write_pos = 0;
                    memset(write_buffer, 0, ARRAY_SIZE(write_buffer));
                }
                current_root = tree;
            }
        }
    }
    if (write_pos) fwrite(write_buffer, sizeof(*write_buffer), write_pos, ofile);
}
