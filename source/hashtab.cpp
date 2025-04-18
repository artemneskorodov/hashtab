/*============================================================================*/
/**
* @file     hashtab.cpp
* @author   Artem Neskorodov
* @date     2024-04-18
* @brief    File with hashtab access implementation.
*/
/*============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nmmintrin.h>
#include <math.h>

/*============================================================================*/

#include "hashtab.h"
#include "ht_storage.h"
#include "utils.h"
#include "linked_list.h"
#include "ht_dump.h"
#include "colors.h"
#include "parse_flags.h"

/*============================================================================*/
/**
* @brief                Enables optimization of hash function with x86
                        intrinsic. It is fully safe to use this optimization if
                        your processor supports SSE 4.2
*/
#define _OPTIMIZE_HASH

/*----------------------------------------------------------------------------*/
/**
* @brief                Enables checking the result of hashtab_search() while
                        testing with hashtab_run_tests(). Function calls default
                        libc's strcmp() after every search.
*/
#define _CHECK_RESULT

/*============================================================================*/
/**
* @brief                Number of iterations to run while testing.
*/
static const size_t TestsNumber = 100;

/*============================================================================*/

#if defined(_OPTIMIZE_HASH)
    /*------------------------------------------------------------------------*/
    static size_t hash_func_optimized(const char *key);
    /*------------------------------------------------------------------------*/
    #define CALL_HASH_FUNC(_ctx, _key) hash_func_optimized(_key)
    #define CALL_CREATE_CRC_TABLE(_ctx)
    /*------------------------------------------------------------------------*/
#else
    /*------------------------------------------------------------------------*/
    static void create_crc_table(hashtab_t *ctx);
    static size_t hash_func_default(hashtab_t *ctx, const char *key);
    /*------------------------------------------------------------------------*/
    #define CALL_HASH_FUNC(_ctx, _key)  hash_func_default((_ctx), (_key))
    #define CALL_CREATE_CRC_TABLE(_ctx) create_crc_table(_ctx)
    /*------------------------------------------------------------------------*/
#endif

/*============================================================================*/
/**
* @brief                Constructor of hashtab context.

* @param ctx            Pointer to hashtab context.
* @param argc           Number of arguments from console.
* @param argv           Arguments from console.

* @result               ht_error_t - overall error code enum.
*/
ht_error_t hashtab_ctor(hashtab_t *ctx, int argc, const char *argv[]) {
    /*------------------------------------------------------------------------*/
    _RETURN_IF_ERROR(parse_flags(ctx, argc, argv));
    if(ctx->run_mode == HASHTAB_MODE_PARSE_TEXT) {
        return HASHTAB_SUCCESS;
    }
    /*------------------------------------------------------------------------*/
    _HT_DUMP_CTOR(ctx, "dump.log");
    CALL_CREATE_CRC_TABLE(ctx);
    _RETURN_IF_ERROR(ht_storage_ctor(ctx));
    /*------------------------------------------------------------------------*/
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Reading data from file and inserting it in a hashtab.

* @param ctx            Pointer to hashtab context.

* @result               ht_error_t - overall error code enum.

* @note                 Database file format is described in hashtab.h

* @warning              It is expected to call this function only after
                        hashtab_ctor() and only in case when ctx->data_file is
                        set. Don't call this function if
                        ctx->run_mode != HASHTAB_MODE_RUN_TEST
*/
ht_error_t hashtab_read_data(hashtab_t *ctx) {
    /*------------------------------------------------------------------------*/
    FILE *data = fopen(ctx->data_file, "rb");
    if(data == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'\n",
                     ctx->data_file);
        return HASHTAB_OPENING_FILE_ERROR;
    }
    size_t data_size = file_size(data);
    size_t words_num = data_size / KeyWordSize;
    /*------------------------------------------------------------------------*/
    ctx->data = (char *)calloc(data_size, sizeof(char));
    if(ctx->data == NULL) {
        fclose(data);
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory for data.\n");
        return HASHTAB_MEMORY_ERROR;
    }
    /*------------------------------------------------------------------------*/
    if(fread(ctx->data, sizeof(char), data_size, data) != data_size) {
        fclose(data);
        free(ctx->data);
        ctx->data = NULL;
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reading file '%s'\n",
                     ctx->data_file);
        return HASHTAB_READING_FILE_ERROR;
    }
    fclose(data);
    /*------------------------------------------------------------------------*/
    char *position = ctx->data;
    for(size_t i = 0; i < words_num; i++, position += KeyWordSize) {
        _RETURN_IF_ERROR(hashtab_insert(ctx, position, NULL));
    }
    /*------------------------------------------------------------------------*/
    fprintf(stderr, "words = %lu\n", ctx->counter);
    /*------------------------------------------------------------------------*/
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Reading data from test file and running tests.

* @param ctx            Pointer to hashtab context.

* @result               ht_error_t - overall error code enum.

* @note                 Test file format is same as database file format.

* @warning              It is expected to call this function only after
                        hashtab_ctor() and reading database from file.
                        ctx->test_file must be set by user. Don't call this
                        function if ctx->run_mode != HASHTAB_MODE_RUN_TEST
*/
ht_error_t hashtab_run_tests(hashtab_t *ctx) {
    /*------------------------------------------------------------------------*/
    FILE *test = fopen(ctx->test_file, "rb");
    if(test == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'\n",
                     ctx->test_file);
        return HASHTAB_OPENING_FILE_ERROR;
    }
    size_t test_size = file_size(test);
    size_t words_num = test_size / KeyWordSize;
    /*------------------------------------------------------------------------*/
    if(fread(ctx->data, sizeof(char), test_size, test) != test_size) {
        fclose(test);
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reading file '%s'\n",
                     ctx->test_file);
        return HASHTAB_READING_FILE_ERROR;
    }
    fclose(test);
    /*------------------------------------------------------------------------*/
    for(size_t test_num = 0; test_num < TestsNumber; test_num++) {
        /*--------------------------------------------------------------------*/
        char *position = ctx->data;
        for(size_t i = 0; i < words_num; i++, position += KeyWordSize) {
            data_t *result = NULL;
            _RETURN_IF_ERROR(hashtab_search(ctx, position, &result));
            /*----------------------------------------------------------------*/
            #if defined(_CHECK_RESULT)
            if(strcmp(result->key, position) != 0) {
                color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                             "[FAIL] ");
                color_printf(DEFAULT_TEXT, NORMAL_TEXT, DEFAULT_BACKGROUND,
                             "%s\n", position);
                return HASHTAB_FOUND_WRONG;
            }
            #endif
            /*----------------------------------------------------------------*/
        }
        /*--------------------------------------------------------------------*/
    }
    /*------------------------------------------------------------------------*/
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Inserting element to hashtab.

* @param ctx            Pointer to hashtab context.
* @param key            Pointer to 32 bytes array of chars.
* @param data           Pointer to structure with data value. May be NULL.

* @result               ht_error_t - overall error code enum.

* @warning              It is expected to call this function only after
                        hashtab_ctor().
*/
ht_error_t hashtab_insert(hashtab_t *ctx, const char *key, data_t *data) {
    /*------------------------------------------------------------------------*/
    size_t bucket_index = CALL_HASH_FUNC(ctx, key);
    return list_insert(ctx, &ctx->buckets[bucket_index], key, data);
    /*------------------------------------------------------------------------*/
}

/*============================================================================*/
/**
* @brief                Searching element in hashtab

* @param ctx            Pointer to hashtab context.
* @param key            Pointer to 32 bytes array of chars.
* @param result         Pointer to data_t structure to write a result.

* @result               ht_error_t - overall error code enum.

* @warning              It is expected to call this function only after
                        hashtab_ctor().
*/
ht_error_t hashtab_search(hashtab_t *ctx, const char *key, data_t **result) {
    /*------------------------------------------------------------------------*/
    size_t bucket_index = CALL_HASH_FUNC(ctx, key);
    return list_search(&ctx->buckets[bucket_index], key, result);
    /*------------------------------------------------------------------------*/
}

/*============================================================================*/
/**
* @brief                Inserting element to hashtab.

* @param ctx            Pointer to hashtab context.
* @param key            Pointer to 32 bytes array of chars.

* @result               ht_error_t - overall error code enum.

* @warning              It is expected to call this function only after
                        hashtab_ctor().

* @todo                 Carefully check this function with unit tests.
*/
ht_error_t hashtab_remove(hashtab_t *ctx, const char *key) {
    /*------------------------------------------------------------------------*/
    size_t bucket_index = CALL_HASH_FUNC(ctx, key);
    return list_remove(ctx, &ctx->buckets[bucket_index], key);
    /*------------------------------------------------------------------------*/
}

/*============================================================================*/
/**
* @brief                Testing load factor.

* @param ctx            Pointer to hashtab context.

* @result               ht_error_t - overall error code enum.

* @note                 Test file format is same as database file format.

* @warning              It is expected to call this function only after
                        hashtab_ctor(). Don't call it if
                        ctx->run_mode != HASHTAB_MODE_TEST_LOAD.
*/
ht_error_t hashtab_test_load(hashtab_t *ctx) {
    _RETURN_IF_ERROR(hashtab_read_data(ctx));
    double avarage = (double)ctx->counter / (double)BucketsNum;
    double standard_deviation_sq = 0;

    FILE *output = fopen(ctx->output_file, "w");
    if(output == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'.\n",
                     ctx->output_file);
        return HASHTAB_OPENING_FILE_ERROR;
    }

    for(size_t i = 0; i < BucketsNum; i++) {
        fprintf(output, "%lu\n", ctx->buckets[i].elements);
        double current = (double)ctx->buckets[i].elements;
        standard_deviation_sq += (current - avarage) * (current - avarage);
    }

    fclose(output);

    double dispersion = sqrt(standard_deviation_sq / (double)BucketsNum);

    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Load Factor = ");
    color_printf(WHITE_TEXT, NORMAL_TEXT, DEFAULT_BACKGROUND,
                 "%f ± %f\n", avarage, dispersion);

    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Destructor of hashtab context.

* @param ctx            Pointer to hashtab context.

* @result               ht_error_t - overall error code enum.

* @todo                 Check that context was constructed first.
*/
ht_error_t hashtab_dtor(hashtab_t *ctx) {
    /*------------------------------------------------------------------------*/
    _HT_DUMP_DTOR(ctx);
    _RETURN_IF_ERROR(ht_storage_dtor(ctx));
    /*------------------------------------------------------------------------*/
    free(ctx->data);
    if(memset(ctx, 0, sizeof(*ctx)) != ctx) {
        return HASHTAB_MEMORY_ERROR;
    }
    /*------------------------------------------------------------------------*/
    return HASHTAB_SUCCESS;
}

/*============================================================================*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if defined(_OPTIMIZE_HASH)
/*============================================================================*/
    /**
    * @brief                CRC32 function with x86 intrinsics optimization.

    * @param key            Pointer to 32 bytes array of chars.

    * @result               Index in buckets array.
    */
    size_t hash_func_optimized(const char *key) {
        /*--------------------------------------------------------------------*/
        const uint64_t *key_uint = (const uint64_t *)key;
        uint64_t crc = 0xFFFFFFFF;
        /*--------------------------------------------------------------------*/
        for(size_t i = 0; i < KeyWordSize / sizeof(uint64_t); i++) {
            crc = _mm_crc32_u64(crc, key_uint[i]);
        }
        /*--------------------------------------------------------------------*/
        return (crc ^ 0xFFFFFFFF) % BucketsNum;
    }

/*============================================================================*/
#else /* defined(_OPTIMIZE_HASH)                                              */
/*============================================================================*/
    /**
    * @brief                CRC32 hash function

    * @param ctx            Pointer to hashtab context.
    * @param key            Pointer to 32 bytes array of chars.

    * @result               Index in buckets array.

    * @warning              It is expected to call this function only
                            hashtab_ctor(). Don't call it if
                            ctx->run_mode != HASHTAB_MODE_TEST_LOAD.
    */
    size_t hash_func_default(hashtab_t *ctx, const char *key) {
        /*--------------------------------------------------------------------*/
        uint32_t crc = 0xFFFFFFFF;
        /*--------------------------------------------------------------------*/
        for(size_t i = 0; i < KeyWordSize; i++) {
            crc = ctx->crc_table[(crc ^ (uint32_t)*key) & 0xFF] ^ (crc >> 8);
            key++;
        }
        /*--------------------------------------------------------------------*/
        return (crc ^ 0xFFFFFFFF) % BucketsNum;
    }

    /*========================================================================*/
    /**
    * @brief                Creating table for CRC32 hash.

    * @param ctx            Pointer to hashtab context.
    */
    void create_crc_table(hashtab_t *ctx) {
        /*--------------------------------------------------------------------*/
        for(uint32_t i = 0; i < 256; i++) {
            uint32_t crc = i;
            for(uint32_t j = 0; j < 8; j++) {
                crc = crc & 1 ? (crc >> 1) ^ 0x82F63B78 : crc >> 1;
            }
            ctx->crc_table[i] = crc;
        }
        /*--------------------------------------------------------------------*/
    }

/*============================================================================*/
#endif /* defined(_OPTIMIZE_HASH)                                             */
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

