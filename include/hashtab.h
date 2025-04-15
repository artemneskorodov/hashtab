#ifndef HASHTAB_H
#define HASHTAB_H

#include <stdio.h>
#include <stdint.h>

enum ht_error_t {
    HASHTAB_SUCCESS = 0,
    HASHTAB_UNEXPECTED_PARAMS = 1,
    HASHTAB_OPENING_FILE_ERROR = 2,
    HASHTAB_READING_FILE_ERROR = 3,
    HASHTAB_FOUND_WRONG = 4,
    HASHTAB_MEMORY_ERROR = 5,
    HASHTAB_INVALID_CONTAINER_SIZE = 6,
};

static const size_t KeyWordSize = 32;
static const size_t BucketsNum = 1979;
static const size_t ListContainerSize = 2048;
static const size_t ListContainersNum = 20;

struct data_t {
    int value;
    const char *key;
};

struct list_t {
    list_t *prev;
    list_t *next;
    data_t data;
};

struct bucket_t {
    //TODO add elements counter
    // size_t elements;
    list_t *head;
};

struct hashtab_t {
    bucket_t *buckets;
    const char *data_file;
    const char *test_file;
    const char *output_file;

    list_t *free_lists;

    list_t *containers[ListContainersNum];
    size_t used_containers;

    char *data;
    FILE *dump_file;
    bool parse_flag;
    uint32_t crc_table[256];
    size_t counter;
};

ht_error_t hashtab_ctor(hashtab_t *ctx, int argc, const char *argv[]);
ht_error_t hashtab_read_data(hashtab_t *ctx);
ht_error_t hashtab_run_tests(hashtab_t *ctx);
ht_error_t hashtab_insert(hashtab_t *ctx, const char *key, data_t *data);
ht_error_t hashtab_search(hashtab_t *ctx, const char *key, data_t **result);
ht_error_t hashtab_remove(hashtab_t *ctx, const char *key);
ht_error_t hashtab_dtor(hashtab_t *ctx);

#define _RETURN_IF_ERROR(...) {             \
    ht_error_t _error_code = (__VA_ARGS__); \
    if(_error_code != HASHTAB_SUCCESS) {    \
        return _error_code;                 \
    }                                       \
}

#endif
