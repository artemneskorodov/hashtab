/*============================================================================*/
/**
* @file     ht_dump.cpp
* @author   Artem Neskorodov
* @date     2024-04-18
* @brief    File with tools to dump hashtab. It is almost always unused.
*/
/*============================================================================*/

#include <stdio.h>

/*============================================================================*/

#include "hashtab.h"
#include "ht_dump.h"

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
    ctx->dump_file = fopen(filename, "w");
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

    for(size_t cont = 0; cont < ctx->used_containers; cont++) {
        fprintf(ctx->dump_file,
                "----------------------------------------"
                "----------------------------------------\n"
                "LIST ARRAY %lu:\n"
                "\t[      ptr     ]:      prev      |      next      |     key\n", cont);
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

    fprintf(ctx->dump_file,
            "========================================"
            "========================================\n\n\n");
    fflush(ctx->dump_file);
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
    fclose(ctx->dump_file);
    ctx->dump_file = NULL;
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
