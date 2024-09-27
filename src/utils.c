#include "utils.h"
#include <stdlib.h>
#include <stdio.h>

uint8_t *read_file(const char *filename, size_t *len) {
    FILE *file = fopen(filename, "rb");
    if(!file) {
        perror(filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(file);
    fseek(file, SEEK_SET, 0);

    uint8_t *buffer = malloc(sizeof(uint8_t) * file_size);
    size_t bytes_read = fread(buffer, sizeof(uint8_t), file_size, file);
    if(bytes_read != file_size) {
        perror(filename);
        return NULL;
    }

    *len = file_size;
    return buffer;
}