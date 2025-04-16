#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>

#include "hashtab.h"
#include "linked_list.h"
#include "ht_storage.h"
#include "ht_dump.h"

#define _OPTIMIZE_STRCMP
#define _OPTIMIZE_SEARCH

#if defined(_OPTIMIZE_STRCMP)
    extern "C" bool cmp_key(__m256i key1_ymm, const char *key2);
#else
    static bool cmp_key(const char *key1, const char *key2);
#endif

/*============================================================================*/

ht_error_t list_insert(hashtab_t  *ctx,
                       bucket_t   *bucket,
                       const char *key,
                       data_t     *data) {
    /*------------------------------------------------------------------------*/
    /* Checking if elements is already added to list.                         */
    list_t *current = bucket->head->next;
    while(current != bucket->head) {
        if(strcmp(key, current->data.key) == 0) {
            return HASHTAB_SUCCESS;
        }
        current = current->next;
    }

    list_t *new_element = NULL;
    _RETURN_IF_ERROR(ht_storage_get_list(ctx, &new_element));

    new_element->data.key = key;
    if(data != NULL) {
        new_element->data.value = data->value;
    }

    new_element->next = bucket->head->next;
    new_element->prev = bucket->head;

    bucket->head->next->prev = new_element;
    bucket->head->next = new_element;

    _HT_DUMP(ctx, "Dump after list allocation");

    ctx->counter++;
    return HASHTAB_SUCCESS;
    /*------------------------------------------------------------------------*/
}

/*============================================================================*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if defined(_OPTIMIZE_SEARCH)
/*============================================================================*/

    ht_error_t list_search(bucket_t     *bucket,
                           const char   *key,
                           data_t      **result) {
        /*--------------------------------------------------------------------*/
        /* Current position in list.                                          */
        list_t *head = bucket->head->next;
        /*--------------------------------------------------------------------*/
        /* We don't change YMM registers in this function and we only change  */
        /* YMM1 and YMM2 in cmp_key, so we can avoid saving value of YMM0,    */
        /* which has written key in it to memory. This asm inline forces      */
        /* saving key string in YMM0 register.                                */
        asm("vmovdqu %[key], %%ymm0\n"
            :
            : [key] "m" (*key)
            : "%ymm0");
        /*--------------------------------------------------------------------*/
        /* Running through list elements.                                     */
        while(head != bucket->head) {
            /*----------------------------------------------------------------*/
            /* Variable in register to write the result of comparison.        */
            bool cmp_res = false;
            /*----------------------------------------------------------------*/
            /* This part calls cmp_key without moving YMM0 value from memory  */
            /* and saving it, as we know that cmp_key does not change YMM0.   */
            /* The result of cmp_key is written to AL, which is C variable    */
            /* 'cmp_res' that is forced to AL register by output part of this */
            /* asm inline.                                                    */
            asm("mov  %[key], %%rdi   \n"
                "call cmp_key         \n"
                : "=a" (cmp_res)
                : [key] "r"  (head->data.key)
                : "%rdi", "%ymm1", "%ymm3");
            /*----------------------------------------------------------------*/
            /* Checking if the list element with keyword was found.           */
            if(cmp_res) {
                *result = &head->data;
                break;
            }
            /*----------------------------------------------------------------*/
            /* Moving current head to next element in list.                   */
            head = head->next;
        }
        /*--------------------------------------------------------------------*/
        return HASHTAB_SUCCESS;
    }

/*============================================================================*/
#else /* defined(_OPTIMIZE_SEARCH)                                            */
/*============================================================================*/

    ht_error_t list_search(hashtab_t    *ctx,
                           bucket_t     *bucket,
                           const char   *key,
                           data_t      **result) {
        /*--------------------------------------------------------------------*/
        /* Current position in list.                                          */
        list_t *head = bucket->head->next;
        /*--------------------------------------------------------------------*/
        /* Loading key that we need to find in YMM register if string compare */
        /* optimization is enabled.                                           */
        #if defined(_OPTIMIZE_STRCMP)
            __m256i search_key = _mm256_lddqu_si256((const __m256i *)key);
        #endif
        /*--------------------------------------------------------------------*/
        /* Running through list elements.                                     */
        while(head != bucket->head) {
            /*----------------------------------------------------------------*/
            /* Variable to store result of keys comparison.                   */
            bool cmp_result = false;
            /*----------------------------------------------------------------*/
            /* Calling assembler written cmp_key function if its optimization */
            /* is enabled.                                                    */
            #if defined(_OPTIMIZE_STRCMP)
                cmp_result = cmp_key(search_key, head->data.key);
            /*----------------------------------------------------------------*/
            /* Calling default cmp_key function overwise.                     */
            #else
                cmp_result = cmp_key(key, head->data.key);
            #endif
            /*----------------------------------------------------------------*/
            /* Checking if list element with key word was found.              */
            if(cmp_result) {
                *result = &head->data;
                break;
            }
            /*----------------------------------------------------------------*/
            /* Moving current list head to next element.                      */
            head = head->next;
        }
        /*--------------------------------------------------------------------*/
        return HASHTAB_SUCCESS;
    }

/*============================================================================*/
#endif /* defined(_OPTIMIZE_SEARCH)                                           */
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if not(defined(_OPTIMIZE_STRCMP))
/*============================================================================*/

    bool __attribute__ ((noinline)) cmp_key(const char *key1,
                                            const char *key2) {
        return (strcmp(key1, key2) == 0);
    }

/*============================================================================*/
#endif /* not(defined(_OPTIMIZE_STRCMP))                                      */
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*============================================================================*/
//TODO check this function works
ht_error_t list_remove(hashtab_t *ctx, bucket_t *bucket, const char *key) {
    /*------------------------------------------------------------------------*/
    /* Current list elements.                                                 */
    list_t *head = bucket->head->next;
    /*------------------------------------------------------------------------*/
    /* Running through elements in list.                                      */
    while(head != bucket->head) {
        /*--------------------------------------------------------------------*/
        /* Checking if key of current element and key to find are equal.      */
        if(strcmp(key, head->data.key) == 0) {
            /*----------------------------------------------------------------*/
            /* Removing previous and next elements connections.               */
            head->prev->next = head->next;
            head->next->prev = head->prev;
            /*----------------------------------------------------------------*/
            /* Returning element to storage as free.                          */
            _RETURN_IF_ERROR(ht_storage_free_list(ctx, head));
            /*----------------------------------------------------------------*/
            break;
        }
        /*--------------------------------------------------------------------*/
        /* Moving current list head to next element.                          */
        head = head->next;
    }
    /*------------------------------------------------------------------------*/
    return HASHTAB_SUCCESS;
}

/*============================================================================*/

