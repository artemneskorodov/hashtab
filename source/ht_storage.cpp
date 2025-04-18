/*============================================================================*/
/**
* @file     ht_storage.cpp
* @author   Artem Neskorodov
* @date     2024-04-18
* @brief    File which allows to easily free and allocate new nodes for list.
*/
/*============================================================================*/

#include <stdio.h>
#include <stdlib.h>

/*============================================================================*/

#include "hashtab.h"
#include "ht_storage.h"
#include "ht_dump.h"
#include "colors.h"
#include "custom_assert.h"

/*============================================================================*/

enum cont_ctor_type_t {
    CONT_CTOR_DEFAULT   = 1,
    CONT_CTOR_FIRST     = 2,
};

/*============================================================================*/

static ht_error_t ht_container_add(hashtab_t *ctx, cont_ctor_type_t ctor_type);

/*============================================================================*/
/**
* @brief                Constructor for hashtab storage data.

* @param ctx            Pointer to hashtab context.

* @result               ht_error_t - overall error code enum.

* @warning              It is expected to call this function only from
                        hashtab_ctor().
*/
ht_error_t ht_storage_ctor(hashtab_t *ctx) {
    ctx->buckets = (bucket_t *)calloc(BucketsNum, sizeof(bucket_t));
    if(ctx->buckets == NULL) {
        return HASHTAB_MEMORY_ERROR;
    }

    _RETURN_IF_ERROR(ht_container_add(ctx, CONT_CTOR_FIRST));
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
    if(ctx->free_lists == NULL) {
        _RETURN_IF_ERROR(ht_container_add(ctx, CONT_CTOR_DEFAULT));
        _HT_DUMP(ctx, "Dump after new container");
    }
    list_t *result = ctx->free_lists;

    ctx->free_lists = result->next;
    *list = result;
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
    list->prev = NULL;
    list->next = ctx->free_lists;
    ctx->free_lists = list;
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
    free(ctx->buckets);
    for(size_t i = 0; i < ctx->used_containers; i++) {
        free(ctx->containers[i]);
        ctx->containers[i] = NULL;
    }
    ctx->buckets = NULL;
    ctx->free_lists = NULL;
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
    _C_ASSERT(ListContainerSize >= BucketsNum,
              return HASHTAB_INVALID_CONTAINER_SIZE);
    size_t index = ctx->used_containers;
    if(index >= ListContsNum) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Unable to allocate new container as "
                     "only %lu containers are supported\n",
                     ListContsNum);
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

    ctx->containers[index] = lists;
    ctx->used_containers++;
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
