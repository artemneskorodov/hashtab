/*============================================================================*/
/**
* @file     ht_utils.cpp
* @author   Artem Neskorodov
* @date     2024-04-18
* @brief    Implementation of functions which are helpful for hashtab.
*/
/*============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*============================================================================*/

#include "hashtab.h"
#include "ht_utils.h"
#include "colors.h"
#include "custom_assert.h"

/*============================================================================*/

enum cont_ctor_type_t {
    CONT_CTOR_DEFAULT   = 1,
    CONT_CTOR_FIRST     = 2,
};

/*============================================================================*/

typedef ht_error_t (*flag_handler_t)(hashtab_t *, int *, int, const char *[]);

/*============================================================================*/
/**
* @brief                Size of buffer to use while writing.
*/
static const size_t BufferWordsNum = 1024;

/*============================================================================*/
/**
* @brief                Supported flag information structure.
*/
struct flag_t {
    const char         *long_name;                  ///< Long name of flag.
    flag_handler_t      handler;                    ///< Function to handle.
};

/*============================================================================*/

static ht_error_t ht_container_add(hashtab_t       *ctx,
                                   cont_ctor_type_t ctor_type);

static ht_error_t handler_test    (hashtab_t       *ctx,
                                   int             *current,
                                   int              argc,
                                   const char      *argv[]);

static ht_error_t handler_parse   (hashtab_t       *ctx,
                                   int             *current,
                                   int              argc,
                                   const char      *argv[]);

static ht_error_t handler_load    (hashtab_t       *ctx,
                                   int             *current,
                                   int              argc,
                                   const char      *argv[]);

static ht_error_t read_input_file (hashtab_t       *ctx,
                                   char           **buffer,
                                   size_t          *input_size);

/*============================================================================*/

static const flag_t SupportedFlags[] = {
    {.long_name = "--load",  .handler = handler_load },
    {.long_name = "--test",  .handler = handler_test },
    {.long_name = "--parse", .handler = handler_parse},
};
static const size_t SupportedFlagsSize = sizeof(SupportedFlags) /
                                         sizeof(SupportedFlags[0]);

/*============================================================================*/
/**
* @brief                Constructor for hashtab storage data.

* @param ctx            Pointer to hashtab context.

* @result               ht_error_t - overall error code enum.

* @warning              It is expected to call this function only from
                        hashtab_ctor().
*/
ht_error_t ht_storage_ctor(hashtab_t *ctx) {
    _C_ASSERT(ctx          != NULL, return HASHTAB_CTX_NULL_PTR);
    _C_ASSERT(!ctx->constructed,    return HASHTAB_DOUBLE_CTOR );
    _C_ASSERT(ctx->buckets == NULL, return HASHTAB_DOUBLE_CTOR );
    for(size_t cont = 0; cont < ListContsNum; cont++) {
        _C_ASSERT(ctx->containers[cont] == NULL, return HASHTAB_DOUBLE_CTOR);
    }
    /*------------------------------------------------------------------------*/
    ctx->buckets = (bucket_t *)calloc(BucketsNum, sizeof(bucket_t));
    if(ctx->buckets == NULL) {
        return HASHTAB_MEMORY_ERROR;
    }
    _RETURN_IF_ERROR(ht_container_add(ctx, CONT_CTOR_FIRST));
    /*------------------------------------------------------------------------*/
    _HT_DUMP(ctx, "This is dump after initialization of storage");
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Allocator of new node for list.

* @param ctx            Pointer to hashtab context.
* @param list           Pointer to write a pointer to allocated list.

* @result               ht_error_t - overall error code enum.

* @warning              It is expected to call this function only from
                        list_insert().
*/
ht_error_t ht_storage_get_list(hashtab_t *ctx, list_t **list) {
    _C_ASSERT(ctx                  != NULL,         return HASHTAB_CTX_NULL_PTR);
    _C_ASSERT(ctx->constructed,                     return HASHTAB_DOUBLE_CTOR );
    _C_ASSERT(ctx->buckets         != NULL,         return HASHTAB_BROKEN_DATA );
    _C_ASSERT(ctx->used_containers <  ListContsNum, return HASHTAB_BROKEN_DATA );
    /*------------------------------------------------------------------------*/
    if(ctx->free_lists == NULL) {
        _RETURN_IF_ERROR(ht_container_add(ctx, CONT_CTOR_DEFAULT));
        _HT_DUMP(ctx, "Dump after new container");
    }
    list_t *result = ctx->free_lists;
    ctx->free_lists = result->next;
    *list = result;
    /*------------------------------------------------------------------------*/
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Function adds element to list of frees.

* @param ctx            Pointer to hashtab context.
* @param list           Pointer to list element to free.

* @result               ht_error_t - overall error code enum.

* @warning              It is expected to call this function only from
                        list_remove().
*/
ht_error_t ht_storage_free_list(hashtab_t *ctx, list_t *list) {
    _C_ASSERT(ctx  != NULL,     return HASHTAB_CTX_NULL_PTR   );
    _C_ASSERT(list != NULL,     return HASHTAB_NULL_LIST_PTR  );
    _C_ASSERT(ctx->constructed, return HASHTAB_NOT_CONSTRUCTED);
    /*------------------------------------------------------------------------*/
    list->prev = NULL;
    list->next = ctx->free_lists;
    ctx->free_lists = list;
    /*------------------------------------------------------------------------*/
    _HT_DUMP(ctx, "Dump after list free");
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Destructor of hashtab storage.

* @param ctx            Pointer to hashtab context.

* @result               ht_error_t - overall error code enum.

* @todo                 Check that context was constructed.
*/
ht_error_t ht_storage_dtor(hashtab_t *ctx) {
    _C_ASSERT(ctx != NULL,      return HASHTAB_CTX_NULL_PTR   );
    _C_ASSERT(ctx->constructed, return HASHTAB_NOT_CONSTRUCTED);
    /*------------------------------------------------------------------------*/
    free(ctx->buckets);
    for(size_t i = 0; i < ctx->used_containers; i++) {
        free(ctx->containers[i]);
        ctx->containers[i] = NULL;
    }
    ctx->buckets    = NULL;
    ctx->free_lists = NULL;
    /*------------------------------------------------------------------------*/
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Adds container to storage.

* @param ctx            Pointer to hashtab context.
* @param ctor_type      Type of initialization: a list of frees, or adding zeros
                        elements to buckets.

* @result               ht_error_t - overall error code enum.

* @warning              ctor_type == CONT_CTOR_FIRST must be used only once.
*/
ht_error_t ht_container_add(hashtab_t *ctx, cont_ctor_type_t ctor_type) {
    _C_ASSERT(ctx                  != NULL,
              return HASHTAB_CTX_NULL_PTR);
    _C_ASSERT(ctx->used_containers <  ListContsNum,
              return HASHTAB_BROKEN_DATA);
    _C_ASSERT(ctx->buckets         != NULL,
              return HASHTAB_BROKEN_DATA);
    _C_ASSERT(ListContainerSize >= BucketsNum,
              return HASHTAB_INVALID_CONTAINER_SIZE);
    /*------------------------------------------------------------------------*/
    size_t index = ctx->used_containers;
    if(index >= ListContsNum) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Unable to allocate new container as "
                     "only %lu containers are supported\n",
                     ListContsNum);
        return HASHTAB_MEMORY_ERROR;
    }
    /*------------------------------------------------------------------------*/
    list_t *lists = (list_t *)calloc(ListContainerSize, sizeof(list_t));
    if(lists == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating new container\n");
        return HASHTAB_MEMORY_ERROR;
    }
    /*------------------------------------------------------------------------*/
    switch(ctor_type) {
        case CONT_CTOR_DEFAULT: {
            for(size_t i = 0; i + 1 < ListContainerSize; i++) {
                lists[i].next = &lists[i] + 1;
            }
            lists[ListContainerSize - 1].next = ctx->free_lists;
            ctx->free_lists = lists;
            break;
        }
        case CONT_CTOR_FIRST: {
            for(size_t i = 0; i < BucketsNum; i++) {
                ctx->buckets[i].head = &lists[i];
                lists[i].prev = &lists[i];
                lists[i].next = &lists[i];
            }
            for(size_t i = BucketsNum; i + 1 < ListContainerSize; i++) {
                lists[i].next = &lists[i] + 1;
            }
            if(ListContainerSize == BucketsNum) {
                ctx->free_lists = NULL;
            }
            else {
                ctx->free_lists = lists + BucketsNum;
            }
            break;
        }
        default: {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unexpected parameter to call '%s'\n",
                         __PRETTY_FUNCTION__);
            return HASHTAB_UNEXPECTED_PARAMS;
        }
    }
    /*------------------------------------------------------------------------*/
    ctx->containers[index] = lists;
    ctx->used_containers++;
    /*------------------------------------------------------------------------*/
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Constructor of dump information.

* @param ctx            Pointer to hashtab context.
* @param filename       Name of file to write dump.

* @result               ht_error_t - overall error code enum.

* @deprecated           This code cannot be helpful when hashtab size is big.
                        Use macro _HT_DUMP_CTOR(_ctx, _filename) to call this
                        function.
*/
ht_error_t hashtab_dump_ctor(hashtab_t *ctx, const char *filename) {
    _C_ASSERT(ctx            != NULL, return HASHTAB_CTX_NULL_PTR );
    _C_ASSERT(filename       != NULL, return HASHTAB_NULL_FILENAME);
    _C_ASSERT(ctx->dump_file == NULL, return HASHTAB_DOUBLE_CTOR  );
    _C_ASSERT(!ctx->constructed,      return HASHTAB_DOUBLE_CTOR  );
    /*------------------------------------------------------------------------*/
    ctx->dump_file = fopen(filename, "w");
    /*------------------------------------------------------------------------*/
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Function dumps linked list.

* @param ctx            Pointer to hashtab context.
* @param label          Label for dump.

* @result               ht_error_t - overall error code enum.

* @note                 Use macro _HT_DUMP() to pass parameters.

* @deprecated           This code cannot be helpful when hashtab size is big.
                        Use macro _HT_DUMP(_ctx, _label) to call this
                        function.
*/
ht_error_t hashtab_dump(hashtab_t  *ctx,
                        const char *label,
                        const char *file,
                        const char *func,
                        int         line) {
    _C_ASSERT(ctx            != NULL, return HASHTAB_CTX_NULL_PTR   );
    _C_ASSERT(ctx->dump_file != NULL, return HASHTAB_NOT_CONSTRUCTED);
    _C_ASSERT(ctx->buckets   != NULL, return HASHTAB_NOT_CONSTRUCTED);
    _C_ASSERT(ctx->constructed,       return HASHTAB_NOT_CONSTRUCTED);
    /*------------------------------------------------------------------------*/
    fprintf(ctx->dump_file,
            "========================================"
            "========================================\n"
            "This dump is called from line %d in "
            "file '%s'\n"
            "From function: '%s'\n"
            "Dump comment: %s\n"
            "----------------------------------------"
            "----------------------------------------\n"
            "Free: %p\n"
            "----------------------------------------"
            "----------------------------------------\n"
            "BUCKETS:\n"
            "\t[index]:    head_prev   |   head_next\n",
            line, file, func, label, ctx->free_lists);
    /*------------------------------------------------------------------------*/
    for(size_t i = 0; i < BucketsNum; i++) {
        fprintf(ctx->dump_file,
                "\t[  %03lu]: %14p | %14p |",
                i,
                ctx->buckets[i].head->prev,
                ctx->buckets[i].head->next);
        if(ctx->buckets[i].head->next == ctx->buckets[i].head) {
            fprintf(ctx->dump_file, " FREE\n");
        }
        else {
            fprintf(ctx->dump_file, " NOT FREE\n");
        }
    }
    /*------------------------------------------------------------------------*/
    for(size_t cont = 0; cont < ctx->used_containers; cont++) {
        fprintf(ctx->dump_file,
                "----------------------------------------"
                "----------------------------------------\n"
                "LIST ARRAY %lu:\n"
                "\t[      ptr     ]:      prev      |"
                "      next      |     key\n", cont);
        list_t *lists = ctx->containers[cont];
        for(size_t i = 0; i < ListContainerSize; i++) {
            fprintf(ctx->dump_file,
                    "\t[%11p]: %14p | %14p | %s\n",
                    &lists[i],
                    lists[i].prev,
                    lists[i].next,
                    lists[i].data.key);
        }
    }
    /*------------------------------------------------------------------------*/
    fprintf(ctx->dump_file,
            "========================================"
            "========================================\n\n\n");
    fflush(ctx->dump_file);
    /*------------------------------------------------------------------------*/
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Destructor for dump information of hashtab.

* @param ctx            Pointer to hashtab context.

* @result               ht_error_t - overall error code enum.

* @deprecated           This code cannot be helpful when hashtab size is big.
                        Use macro _HT_DUMP_DTOR(_ctx) to call this
                        function.
*/
ht_error_t hashtab_dump_dtor(hashtab_t *ctx) {
    _C_ASSERT(ctx            != NULL, return HASHTAB_CTX_NULL_PTR);
    _C_ASSERT(ctx->dump_file != NULL, return HASHTAB_DOUBLE_DTOR );
    _C_ASSERT(ctx->constructed,       return HASHTAB_DOUBLE_DTOR );
    /*------------------------------------------------------------------------*/
    fclose(ctx->dump_file);
    ctx->dump_file = NULL;
    /*------------------------------------------------------------------------*/
    return HASHTAB_SUCCESS;
}

/*============================================================================*/

size_t file_size(FILE *file) {
    _C_ASSERT(file != NULL, return HASHTAB_NULL_FILE);
    /*------------------------------------------------------------------------*/
    long position = ftell(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, position, SEEK_SET);
    /*------------------------------------------------------------------------*/
    return (size_t)size;
}

/*============================================================================*/
/**
* @brief                Parsing flags from console input.

* @param ctx            Pointer to hashtab context.
* @param argc           Number of flags from console.
* @param argv           Flags from console.

* @result               ht_error_t - overall error code enum.
*/
ht_error_t parse_flags(hashtab_t *ctx, int argc, const char *argv[]) {
    int current = 1;
    while(current < argc) {
        bool flag_found = false;
        for(size_t i = 0; i < SupportedFlagsSize; i++) {
            if(strcmp(argv[current], SupportedFlags[i].long_name) == 0) {
                current++;
                flag_found = true;
                _RETURN_IF_ERROR(SupportedFlags[i].handler(ctx,
                                                           &current,
                                                           argc,
                                                           argv));
                break;
            }
        }
        if(!flag_found) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unknown flag '%s'\n", argv[current]);
            return HASHTAB_UNEXPECTED_PARAMS;
        }
    }
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Handling '--test' flag.

* @param ctx            Pointer to hashtab context.
* @param current        Pointer to index of current flag.
* @param argc           Number of flags from console.
* @param argv           Flags from console.

* @result               ht_error_t - overall error code enum.
*/
ht_error_t handler_test(hashtab_t  *ctx,
                        int        *current,
                        int         argc,
                        const char *argv[]) {
    if(*current + 1 >= argc) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Flag '%s' expected to have two parameters: "
                     "names of data and test databases.\n",
                     argv[*current - 1]);
        return HASHTAB_UNEXPECTED_PARAMS;
    }

    ctx->data_file = argv[*current    ];
    ctx->test_file = argv[*current + 1];

    ctx->run_mode = HASHTAB_MODE_RUN_TEST;
    (*current) +=  2;
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Handling '--parse' flag.

* @param ctx            Pointer to hashtab context.
* @param current        Pointer to index of current flag.
* @param argc           Number of flags from console.
* @param argv           Flags from console.

* @result               ht_error_t - overall error code enum.
*/
ht_error_t handler_parse(hashtab_t  *ctx,
                         int        *current,
                         int         argc,
                         const char *argv[]) {
    if(*current + 1 >= argc) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Flag '%s' expected to have two parameters: "
                     "name of file to parse and output file.\n",
                     argv[*current - 1]);
        return HASHTAB_UNEXPECTED_PARAMS;
    }

    ctx->data_file   = argv[*current    ];
    ctx->output_file = argv[*current + 1];

    ctx->run_mode = HASHTAB_MODE_PARSE_TEXT;
    (*current) += 2;
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Handling '--load' flag.

* @param ctx            Pointer to hashtab context.
* @param current        Pointer to index of current flag.
* @param argc           Number of flags from console.
* @param argv           Flags from console.

* @result               ht_error_t - overall error code enum.
*/
ht_error_t handler_load(hashtab_t  *ctx,
                        int        *current,
                        int         argc,
                        const char *argv[]) {
    if(*current + 1 >= argc) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Flag '%s' expected to have two parameters: "
                     "name of data and output files.\n",
                     argv[*current - 1]);
        return HASHTAB_UNEXPECTED_PARAMS;
    }

    ctx->data_file   = argv[*current    ];
    ctx->output_file = argv[*current + 1];

    ctx->run_mode = HASHTAB_MODE_TEST_LOAD;
    (*current) += 2;
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Parsing text to database format.

* @param ctx            Pointer to hashtab context.

* @result               ht_error_t - overall error code enum.

* @warning              It is not expected to call this function after
                        hashtab_ctor().
*/
ht_error_t parse_text(hashtab_t *ctx) {
    char *buffer = NULL;
    size_t input_size = 0;
    _RETURN_IF_ERROR(read_input_file(ctx, &buffer, &input_size));

    char *words_buffer = buffer + input_size;
    size_t words_buffer_pos = 0;
    FILE *output = fopen(ctx->output_file, "w");
    if(output == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'\n",
                     ctx->output_file);
        free(buffer);
        return HASHTAB_OPENING_FILE_ERROR;
    }

    size_t pos = 0;
    while(!isalpha(buffer[pos])) {
        pos++;
    }

    while(pos < input_size) {
        size_t end = pos;
        while(isalpha(buffer[end]) && pos != input_size) {
            end++;
        }
        for(size_t i = 0; i < end - pos; i++) {
            words_buffer[words_buffer_pos + i] = buffer[pos + i];
        }
        words_buffer_pos += KeyWordSize;
        if(words_buffer_pos == BufferWordsNum * (KeyWordSize)) {
            fwrite(words_buffer,
                   sizeof(char) * (KeyWordSize),
                   BufferWordsNum,
                   output);
            memset(words_buffer,
                   0,
                   sizeof(char) * (KeyWordSize) * BufferWordsNum);
            words_buffer_pos = 0;
        }
        pos = end;
        while(!isalpha(buffer[pos]) && pos != input_size) {
            pos++;
        }
    }

    fwrite(words_buffer, sizeof(char), words_buffer_pos, output);
    fclose(output);
    free(buffer);
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Inserting element in linked list.

* @param ctx            Pointer to hashtab context.
* @param buffer         Place to write read buffer.
* @param input_size     Place to write buffer size.

* @result               ht_error_t - overall error code enum.

* @warning              It is not expected to call this function after
                        hashtab_ctor().
*/
ht_error_t read_input_file(hashtab_t *ctx, char **buffer, size_t *input_size) {
    FILE *input = fopen(ctx->data_file, "rb");
    if(input == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'\n",
                     ctx->data_file);
        return HASHTAB_OPENING_FILE_ERROR;
    }

    *input_size = file_size(input);
    *buffer = (char *)calloc(*input_size + BufferWordsNum * KeyWordSize,
                             sizeof(char));
    if(*buffer == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory for buffer\n");
        fclose(input);
        return HASHTAB_MEMORY_ERROR;
    }

    if(fread(*buffer, sizeof(char), *input_size, input) != *input_size) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reading input file.\n");
        fclose(input);
        free(*buffer);
        return HASHTAB_READING_FILE_ERROR;
    }
    fclose(input);
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
