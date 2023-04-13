#include "utils.h"
#include "pointers.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define error(msg) do { fprintf(stderr, "[ERR] file %s/line %d: %s\n", __FILE__, __LINE__, msg); exit(EXIT_FAILURE); } while(0)

extern bool DEBUG;

#define ERR_MSG_BUFFER 256

void get_frequencies(frequency_node *nodes, FILE *ifile_fd) {
    uint8_t buffer[1024];
    size_t ret = 0;
    while (!feof(ifile_fd)) {
        ret = fread(buffer, sizeof(*buffer), ARRAY_SIZE(buffer), ifile_fd);
        if (ret != ARRAY_SIZE(buffer)) {
            if (ferror(ifile_fd)) {
                uint8_t err_msg[ERR_MSG_BUFFER];
                sprintf((char*)err_msg, "fread() failed: %zu\n", ret);
                error(err_msg);
            }
        }
        for (uint16_t i = 0; i < ret; i++) {
            uint8_t c = buffer[i];
            nodes[c].frequency++;
            if (c && !nodes[c].symbol_code) nodes[c].symbol_code = c;
        }
    }
}

uint8_t write_header(FILE *ofile, uint16_t tree_size, uintmax_t data_size, uint8_t tree_last_bits, uint8_t data_last_bits) {
    const uint8_t *fingerprint = (const uint8_t*)"HUFF";
    size_t fingerprint_size = 4;
    uint8_t metadata_size = (uint8_t)(sizeof(tree_size) + sizeof(data_size));
    uint8_t buffer_size = (uint8_t)(fingerprint_size + sizeof(buffer_size)) + metadata_size;
    uint8_t buffer[buffer_size];
    uint8_t written = 0;
    tree_size |= tree_last_bits << 13;
    tree_size |= data_last_bits << 10;
    memcpy(buffer, fingerprint, fingerprint_size);
    memcpy(&buffer[fingerprint_size], &metadata_size, sizeof(metadata_size));
    memcpy(&buffer[fingerprint_size + sizeof(metadata_size)], &tree_size, sizeof(tree_size));
    memcpy(&buffer[fingerprint_size + sizeof(metadata_size) + sizeof(tree_size)], &data_size, sizeof(data_size));
    written = fwrite(buffer, buffer_size, 1, ofile);
    if (written != 1) {
        uint8_t err_msg[ERR_MSG_BUFFER];
        sprintf((char*)err_msg, "fwrite header failed! Expected 1 got %hu\n", written);
        error(err_msg);
    }
    return buffer_size;
}

void header_check(const char *ofile_name, uint16_t tree_size, uintmax_t data_size, uint8_t tree_last_bits, uint8_t data_last_bits) {
    FILE *ofile = fopen(ofile_name, "rb");
    uint8_t err_msg[ERR_MSG_BUFFER];
    if (!ofile) {
        sprintf((char*)err_msg, "fopen failed! Cannot open the file %s\n", ofile_name);
        error(err_msg);
    }

    uint16_t tree_size_from_header;
    uintmax_t data_size_from_header;
    uint8_t tree_last_bits_from_header;
    uint8_t data_last_bits_from_header;
    uint16_t tree_size_buf;

    uint8_t buffer[5];
    size_t read = 0;
    read = fread(buffer, ARRAY_SIZE(buffer), sizeof(*buffer), ofile);
    if (read != sizeof(*buffer)) {
        if (ferror(ofile)) {
            sprintf((char*)err_msg, "fread failed! Expected %zu bytes, got %zu bytes\n", sizeof(*buffer), read);
            error(err_msg);
        }
    }
    check_fingerprint(buffer);

    uint8_t metadata_size = buffer[4];
    uint8_t metadata_buffer[metadata_size];

    read = fread(metadata_buffer, ARRAY_SIZE(metadata_buffer), sizeof(*metadata_buffer), ofile);
    if (read != sizeof(*metadata_buffer)) {
        if (ferror(ofile)) {
            sprintf((char*)err_msg, "fread failed! Expected %zu bytes, got %zu bytes\n", sizeof(*buffer), read);
            error(err_msg);
        }
    }
    memcpy(&tree_size_buf, metadata_buffer, sizeof(tree_size_buf));
    memcpy(&data_size_from_header, metadata_buffer + sizeof(tree_size_buf), metadata_size - sizeof(tree_size_buf));
    data_last_bits_from_header = (tree_size_buf >> 10) & 7;
    tree_last_bits_from_header = (tree_size_buf >> 13) & 7;
    tree_size_from_header = tree_size_buf & 0x1FF;

    assert(data_size == data_size_from_header);
    assert(tree_size == tree_size_from_header);
    assert(tree_last_bits_from_header == tree_last_bits);
    assert(data_last_bits_from_header == data_last_bits);

    fclose(ofile);
}

void check_fingerprint(uint8_t *buf) {
    uint8_t fingerprint[5];
    memcpy(fingerprint, buf, 4);
    fingerprint[4] = '\0';
    if (strcmp("HUFF", (const char *)fingerprint)) {
        uint8_t err_msg[ERR_MSG_BUFFER];
        sprintf((char*)err_msg, "First 4 bytes should be HUFF, got %s\n", fingerprint);
        error(err_msg);
    }
}

