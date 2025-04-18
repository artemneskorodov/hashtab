#ifndef HASHTAB_H
#define HASHTAB_H
/**
* @mainpage         Hashtab

* @section          Introduction
Implementation of hashtab with 4.14 times optimization with assembly inlines,
assembly code and intrinsics for x86-64 processors with SSE 4.2.

* @section          Features
- Can be compiled for every processor with optimization turned off.
- Can store custom data, but only one per build. See data_t.

* @section          Usage
\code{.cpp}
#include <library.h>

const char key1[32] = {};
const char key2[32] = {};

int main(int argc, const char *argv[]) {
    // Writing keyword to 32 bytes arrays.
    snprintf(key1, 32, "First keyword");
    snprintf(key2, 32, "Second keyword");

    // Calling hashtab constructor
    hashtab_t hashtab = {};
    hashtab_ctor(&hashtab);

    // Inserting elements in hashtab
    data_t data = {.value = 10};
    hashtab_insert(&hashtab, key1, data);
    hashtab_insert(&hashtab, key2, NULL);

    // Searching element in hashtab
    data_t *result = NULL;
    hashtab_search(&hashtab, "First keyword", &result);
    printf("%d", result->value);

    // Removing element from hashtab
    hashtab_remove(&hashtab, "Second keyword");

    // Calling hashtab destructor
    hashtab_dtor(&hashtab);
    return 0;
}
\endcode

* @section          Database format
- Every keyword in database file has to be stored in blocks of 32 bytes.
- Every symbol of 32 bytes is treated as a part of keyword.
- It is expected that database size is divisible by 32.

* @section          Links
- [Репозиторий GitHub](https://github.com/artemneskorodov/hashtab)
*/
/*============================================================================*/
/**
* @file     hashtab.h
* @author   Artem Neskorodov
* @date     2024-04-18
* @brief    Header file for hashtab library.
*/
/*============================================================================*/

#include <stdio.h>
#include <stdint.h>

/*============================================================================*/

enum ht_error_t {
    HASHTAB_SUCCESS                     = 0,
    HASHTAB_UNEXPECTED_PARAMS           = 1,
    HASHTAB_OPENING_FILE_ERROR          = 2,
    HASHTAB_READING_FILE_ERROR          = 3,
    HASHTAB_FOUND_WRONG                 = 4,
    HASHTAB_MEMORY_ERROR                = 5,
    HASHTAB_INVALID_CONTAINER_SIZE      = 6,
};

/*============================================================================*/

enum run_mode_t {
    HASHTAB_MODE_RUN_TEST               = 1,
    HASHTAB_MODE_TEST_LOAD              = 2,
    HASHTAB_MODE_PARSE_TEXT             = 3,
};

/*============================================================================*/

static const size_t KeyWordSize         = 32;
static const size_t BucketsNum          = 1979;
static const size_t ListContainerSize   = 2048;
static const size_t ListContsNum        = 20;

/*============================================================================*/
/**
* @brief                Structure of data stored in hashtab.

* @note                 The only needed element is key, so structure can be
                        changed to store anything.
*/
struct data_t {
    int                 value;                      ///< Some values.
    const char         *key;                        ///< Key of element.
};

/*============================================================================*/
/**
* @brief                List element structure.
*/
struct list_t {
    list_t             *prev;                       ///< Pointer to previous.
    list_t             *next;                       ///< Pointer to next.
    data_t              data;                       ///< Stored data.
};

/*============================================================================*/
/**
* @brief                Hashtable bucket structure.
*/
struct bucket_t {
    list_t             *head;                       ///< Zero element of list.
    size_t              elements;                   ///< Number of elements.
};

/*============================================================================*/
/**
* @brief                Hashtab context structure.
*/
struct hashtab_t {
    bucket_t           *buckets;                    ///< Array of buckets.
    const char         *data_file;                  ///< Name of database file.
    const char         *test_file;                  ///< Name of test file.
    const char         *output_file;                ///< Name of output file.
    list_t             *free_lists;                 ///< Free linked list.
    list_t             *containers[ListContsNum];   ///< Storage containers.
    size_t              used_containers;            ///< Number of used conts.
    char               *data;                       ///< Buffer for database.
    FILE               *dump_file;                  ///< File to write dump.
    run_mode_t          run_mode;                   ///< Enum with run mode.
    uint32_t            crc_table[256];             ///< Table for CRC32.
    size_t              counter;                    ///< Number of elements.
};

/*============================================================================*/

ht_error_t hashtab_ctor        (hashtab_t      *ctx,
                                int             argc,
                                const char     *argv[]);

ht_error_t hashtab_read_data   (hashtab_t      *ctx);

ht_error_t hashtab_run_tests   (hashtab_t      *ctx);

ht_error_t hashtab_insert      (hashtab_t      *ctx,
                                const char     *key,
                                data_t         *data);

ht_error_t hashtab_search      (hashtab_t      *ctx,
                                const char     *key,
                                data_t        **result);

ht_error_t hashtab_remove      (hashtab_t      *ctx,
                                const char     *key);

ht_error_t hashtab_dtor        (hashtab_t      *ctx);

ht_error_t hashtab_test_load   (hashtab_t      *ctx);

/*============================================================================*/

#define _RETURN_IF_ERROR(...) {                                                \
    ht_error_t _error_code = (__VA_ARGS__);                                    \
    if(_error_code != HASHTAB_SUCCESS) {                                       \
        return _error_code;                                                    \
    }                                                                          \
}

/*============================================================================*/
#endif
