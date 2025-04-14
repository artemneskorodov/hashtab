#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nmmintrin.h>

#include "hashtab.h"
#include "ht_storage.h"
#include "utils.h"
#include "linked_list.h"
#include "ht_dump.h"
#include "colors.h"
#include "parse_flags.h"

static void create_crc_table(hashtab_t *ctx);
static size_t hash_func(hashtab_t *ctx, const char *key);

ht_error_t hashtab_ctor(hashtab_t *ctx, int argc, const char *argv[]) {
    _RETURN_IF_ERROR(parse_flags(ctx, argc, argv));
    if(ctx->parse_flag) {
        return HASHTAB_SUCCESS;
    }
    _HT_DUMP_CTOR(ctx, "dump.log");

    create_crc_table(ctx);
    _RETURN_IF_ERROR(ht_storage_ctor(ctx));
    return HASHTAB_SUCCESS;
}

ht_error_t hashtab_read_data(hashtab_t *ctx) {
    FILE *data = fopen(ctx->data_file, "rb");
    if(data == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'\n",
                     ctx->data_file);
        return HASHTAB_OPENING_FILE_ERROR;
    }

    size_t data_size = file_size(data);
    ctx->data = (char *)calloc(data_size, sizeof(char));

    if(fread(ctx->data, sizeof(char), data_size, data) != data_size) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reading file '%s'\n",
                     ctx->data_file);
        return HASHTAB_READING_FILE_ERROR;
    }
    fclose(data);


    char *position = ctx->data;
    size_t words_num = data_size / (KeyWordSize + 1);
    for(size_t i = 0; i < words_num; i++, position += KeyWordSize + 1) {
        _RETURN_IF_ERROR(hashtab_insert(ctx, position, NULL));
    }
    return HASHTAB_SUCCESS;
}

ht_error_t hashtab_run_tests(hashtab_t *ctx) {
    FILE *test = fopen(ctx->test_file, "rb");
    if(test == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'\n",
                     ctx->test_file);
        return HASHTAB_OPENING_FILE_ERROR;
    }

    size_t test_size = file_size(test);

    if(fread(ctx->data, sizeof(char), test_size, test) != test_size) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reading file '%s'\n",
                     ctx->test_file);
        return HASHTAB_READING_FILE_ERROR;
    }
    fclose(test);

    size_t words_num = test_size / (KeyWordSize + 1);

    for(size_t test_iter = 0; test_iter < 100; test_iter++) {
        char *position = ctx->data;

        for(size_t i = 0; i < words_num; i++, position += KeyWordSize + 1) {
            data_t *result = NULL;
            _RETURN_IF_ERROR(hashtab_search(ctx, position, &result));
            if(strcmp(result->key, position) != 0) {
                color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                             "[FAIL] ");
                color_printf(DEFAULT_TEXT, NORMAL_TEXT, DEFAULT_BACKGROUND,
                             "%s\n", position);
                return HASHTAB_FOUND_WRONG;
            }
        }
    }
    return HASHTAB_SUCCESS;
}

ht_error_t hashtab_insert(hashtab_t *ctx, const char *key, data_t *data) {
    size_t bucket_index = hash_func(ctx, key);
    _RETURN_IF_ERROR(list_insert(ctx, &ctx->buckets[bucket_index], key, data));
    return HASHTAB_SUCCESS;
}


ht_error_t hashtab_search(hashtab_t *ctx, const char *key, data_t **result) {
    size_t bucket_index = hash_func(ctx, key);
    _RETURN_IF_ERROR(list_search(ctx, &ctx->buckets[bucket_index], key, result));
    return HASHTAB_SUCCESS;
}
ht_error_t hashtab_remove(hashtab_t *ctx, const char *key) {
    size_t bucket_index = hash_func(ctx, key);
    _RETURN_IF_ERROR(list_remove(ctx, &ctx->buckets[bucket_index], key));
    return HASHTAB_SUCCESS;
}

ht_error_t hashtab_dtor(hashtab_t *ctx) {
    _HT_DUMP_DTOR(ctx);
    _RETURN_IF_ERROR(ht_storage_dtor(ctx));
    free(ctx->data);
    if(memset(ctx, 0, sizeof(*ctx)) != ctx) {
        return HASHTAB_MEMORY_ERROR;
    }
    return HASHTAB_SUCCESS;
}

// size_t hash_func(hashtab_t *ctx, const char *key) {
//     uint32_t crc = 0xFFFFFFFF;
//     for(size_t i = 0; i < KeyWordSize; i++) {
//         crc = ctx->crc_table[(crc ^ *key++) & 0xFF] ^ (crc >> 8);
//     }
//     return (crc ^ 0xFFFFFFFF) % BucketsNum;
// }

size_t hash_func(hashtab_t *ctx, const char *key) {
    uint64_t crc = 0xFFFFFFFF;
    uint64_t *key_uint = (uint64_t *)key;
    for(size_t i = 0; i < KeyWordSize / sizeof(uint64_t); i++) {
        crc = _mm_crc32_u64(crc, key_uint[i]);
    }
    return (crc ^ 0xFFFFFFFF) % BucketsNum;
}

void create_crc_table(hashtab_t *ctx) {
    for(uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for(uint32_t j = 0; j < 8; j++) {
            crc = crc & 1 ? (crc >> 1) ^ 0x82F63B78 : crc >> 1;
        }
        ctx->crc_table[i] = crc;
    }
}
