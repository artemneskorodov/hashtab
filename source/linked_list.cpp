/*============================================================================*/
/**
* @file     linked_list.cpp
* @author   Artem Neskorodov
* @date     2024-04-18
* @brief    File with linked_list access implementation.
*/
/*============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>

/*============================================================================*/

#include "hashtab.h"
#include "linked_list.h"
#include "ht_storage.h"
#include "ht_dump.h"

/*============================================================================*/

#if defined(_OPTIMIZE_STRCMP)
    /*------------------------------------------------------------------------*/
    /**
    * @brief                Optimized function to compare strings.

    * @param key1_ymm       First string moved to YMM0 register.
    * @param key2           Pointer to second string moved to RDI register.

    * @result               ZF flag is set if strings are equal.
    * @result               AL is set to zeros if strings are equal.

    * @note                 This function is implemented in cmp_key.asm.
    */
    extern "C" bool cmp_key_optimized(__m256i key1_ymm, const char *key2);
    /*------------------------------------------------------------------------*/
    /**
    * @brief                Macro to call keys comparison if strcmp
                            optimization is enabled.

    * @param _string_key1   Pointer to first string. It is not used.
    * @param _ymm_key1      __m256i variable with stored string in it.
    * @param _string_key2   Pointer to second string.

    * @note                 It won't use parameter _string_key1
    */
    #define CMP_KEY(_string_key1, _ymm_key1, _string_key2)                     \
        !cmp_key_optimized((_ymm_key1), (_string_key2))
    /*------------------------------------------------------------------------*/
#else
    /*------------------------------------------------------------------------*/
    static bool cmp_key_default(const char *key1, const char *key2);
    /*------------------------------------------------------------------------*/
    /**
    * @brief                Macro to call keys comparison if strcmp
                            optimization is disabled.

    * @param _string_key1   Pointer to first string. It is not used.
    * @param _ymm_key1      __m256i variable with stored string in it.
    * @param _string_key2   Pointer to second string.

    * @note                 It won't use parameter _ymm_key1
    */
    #define CMP_KEY(_string_key1, _ymm_key1, _string_key2)                     \
        cmp_key_default((_string_key1), (_string_key2))
    /*------------------------------------------------------------------------*/
#endif

/*============================================================================*/
/**
* @brief                Inserting element in linked list.

* @param ctx            Pointer to hashtab context.
* @param bucket         Pointer to bucket, which has particular linked list.
* @param key            Pointer to 32 bytes array of chars.
* @param data           Pointer to structure with data. It may be NULL.

* @result               ht_error_t - overall error code enum.

* @warning              It is not expected to call this function not from
                        hashtab functions.
*/
ht_error_t list_insert(hashtab_t  *ctx,
                       bucket_t   *bucket,
                       const char *key,
                       data_t     *data) {
    /*------------------------------------------------------------------------*/
    /* Checking if elements is already added to list.                         */
    list_t *current = bucket->head->next;
    while(current != bucket->head) {
        if(strcmp(key, current->data.key) == 0) {
            current->data.value++;
            return HASHTAB_SUCCESS;
        }
        current = current->next;
    }

    bucket->elements++;

    list_t *new_element = NULL;
    _RETURN_IF_ERROR(ht_storage_get_list(ctx, &new_element));

    new_element->data.key   = key;
    new_element->data.value = 1;

    if(data != NULL) {
        new_element->data.value = data->value;
    }

    new_element->next = bucket->head->next;
    new_element->prev = bucket->head;

    bucket->head->next->prev = new_element;
    bucket->head->next       = new_element;

    _HT_DUMP(ctx, "Dump after list allocation");

    ctx->counter++;
    return HASHTAB_SUCCESS;
    /*------------------------------------------------------------------------*/
}

/*============================================================================*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if defined(_OPTIMIZE_SEARCH)
/*============================================================================*/
    /**
    * @brief                Optimized variant of list_search(). It uses asm
                            inlines to avoid saving YMM registers in memory.

    * @param ctx            Pointer to hashtab context.
    * @param key            Pointer to 32 bytes array of chars - key to find.
    * @param result         Place to write a result of search.

    * @result               ht_error_t - overall error code enum.

    * @note                 Use macro LIST_SEARCH(_bucket, _key, _result) to
                            call this functions as it allows to turn off
                            optimization.

    * @warning              It is expected to call this function only from
                            hashtab functions. Always check disassembler of this
                            function as it forces value to YMM0 register and we
                            cannot be sure that compiler wouldn't use it.
                            Don't call other functions from here. This code was
                            not expected to be ever changed, so read comments,
                            documentation and readme before changing it very
                            carefully.
    */
    ht_error_t list_search_optimized(bucket_t     *bucket,
                                     const char   *key,
                                     data_t      **result) {
        /*--------------------------------------------------------------------*/
        /* Current position in list.                                          */
        list_t *head = bucket->head->next;
        /*--------------------------------------------------------------------*/
        /* We don't change YMM registers in this function and we only change  */
        /* YMM1 and RAX in cmp_key, so we can avoid saving value of YMM0,     */
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
            /* This asm goto inline calls cmp_keys, without saving YMM0 to    */
            /* memory as we know that YMM0 is not changed by this functions.  */
            /* Last function instruction sets ZF flag to 1 if strings are     */
            /* equal, so we use JNZ instruction to skip writing the result.   */
            asm goto ("mov  %[key], %%rdi     \n"
                      "call cmp_key_optimized \n"
                      "jnz %l[skip_cmp_true]  \n"
                      :
                      : [key] "r"  (head->data.key)
                      : "%rdi", "%ymm1", "%rax", "cc"
                      : skip_cmp_true);
            /*----------------------------------------------------------------*/
            /* Writing the result and going out from loop. This part is       */
            /* skipped by previous asm goto if strings are not equal.         */
            *result = &head->data;
            break;
            /*----------------------------------------------------------------*/
            /* This is C label which is used to jump to skip writing the      */
            /* result. It is used by previous asm inline in JNZ command.      */
            skip_cmp_true:
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
    /**
    * @brief                Unoptimized variant of list_search(). This function
                            is safe.

    * @param ctx            Pointer to hashtab context.
    * @param key            Pointer to 32 bytes array of chars - key to find.
    * @param result         Place to write a result of search.

    * @result               ht_error_t - overall error code enum.

    * @note                 Use macro LIST_SEARCH(_bucket, _key, _result) to
                            call this functions as it allows to turn on
                            optimization.

    * @warning              It is expected to call this function only from
                            hashtab functions.
    */
    ht_error_t list_search_default(bucket_t     *bucket,
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
            /* Result of keys comparison.                                     */
            bool cmp_result = CMP_KEY(key, search_key, head->data.key);
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
    /**
    * @brief                Default variant of keys comparison.

    * @param key1           Pointer to first key.
    * @param key2           Pointer to second key.

    * @result               TRUE if strings are equal.

    * @remark               It will be better to turn on optimization with
                            _OPTIMIZE_STRCMP if your processor supports SSE 4.2
    */
    bool cmp_key_default(const char *key1,
                         const char *key2) {
        return (strcmp(key1, key2) == 0);
    }

/*============================================================================*/
#endif /* not(defined(_OPTIMIZE_STRCMP))                                      */
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*============================================================================*/
/**
* @brief                Removes element with this key from list.

* @param ctx            Pointer to hashtab context.
* @param bucket         Pointer to bucket with element to remove.
* @param key            Keyword of element to remove.

* @result               ht_error_t - overall error code enum.

* @warning              It is expected to call this function only from
                        hashtab functions.

* @todo                 Check this function with unit tests.
*/
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

