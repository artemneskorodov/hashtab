#include <stdio.h>
#include <stdlib.h>

#include "hashtab.h"
#include "ht_storage.h"
#include "ht_dump.h"
#include "colors.h"

enum cont_ctor_type_t {
    CONT_CTOR_DEFAULT = 1,
    CONT_CTOR_FIRST = 2,
};

static ht_error_t ht_container_add(hashtab_t *ctx, cont_ctor_type_t ctor_type);

ht_error_t ht_storage_ctor(hashtab_t *ctx) {
    ctx->buckets = (bucket_t *)calloc(BucketsNum, sizeof(bucket_t));
    if(ctx->buckets == NULL) {
        return HASHTAB_MEMORY_ERROR;
    }

    _RETURN_IF_ERROR(ht_container_add(ctx, CONT_CTOR_FIRST));
    _HT_DUMP(ctx, "This is dump after initialization of storage");
    return HASHTAB_SUCCESS;
}

ht_error_t ht_storage_get_list(hashtab_t *ctx, list_t **list) {
    if(ctx->free_lists == NULL) {
        _RETURN_IF_ERROR(ht_container_add(ctx, CONT_CTOR_DEFAULT));
        _HT_DUMP(ctx, "Dump after new container");
    }
    list_t *result = ctx->free_lists;

    ctx->free_lists = result->next;
    *list = result;
    return HASHTAB_SUCCESS;
}

ht_error_t ht_storage_free_list(hashtab_t *ctx, list_t *list) {
    list->prev = NULL;
    list->next = ctx->free_lists;
    ctx->free_lists = list;
    _HT_DUMP(ctx, "Dump after list free");
    return HASHTAB_SUCCESS;
}

ht_error_t ht_storage_dtor(hashtab_t *ctx) {
    free(ctx->buckets);
    ctx->buckets = NULL;
    ctx->free_lists = NULL;
    return HASHTAB_SUCCESS;
}

ht_error_t ht_container_add(hashtab_t *ctx, cont_ctor_type_t ctor_type) {
    size_t index = ctx->used_containers;
    if(index >= ListContainersNum) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Unable to allocate new container as "
                     "only %lu containers are supported\n",
                     ListContainersNum);
        return HASHTAB_MEMORY_ERROR;
    }

    list_t *lists = (list_t *)calloc(ListContainerSize, sizeof(list_t));
    if(lists == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating new container\n");
        return HASHTAB_MEMORY_ERROR;
    }

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
            ctx->free_lists = lists + BucketsNum;
            break;
        }
        default: {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unexpected parameter to call '%s'\n",
                         __PRETTY_FUNCTION__);
            return HASHTAB_UNEXPECTED_PARAMS;
        }
    }

    ctx->containers[index] = lists;
    ctx->used_containers++;
    return HASHTAB_SUCCESS;
}
