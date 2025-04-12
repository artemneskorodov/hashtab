#include <stdio.h>

#include "utils.h"

size_t file_size(FILE *file) {
    long position = ftell(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, position, SEEK_SET);
    return (size_t)size;
}
