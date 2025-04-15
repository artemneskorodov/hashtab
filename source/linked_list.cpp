#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>

#include "hashtab.h"
#include "linked_list.h"
#include "ht_storage.h"
#include "ht_dump.h"

// #define _OPTIMIZE_STRCMP

#if defined(_OPTIMIZE_STRCMP)
    static bool cmp_key_ymm(__m256i key1_ymm, const char *key2);
#else
    static bool cmp_key(const char *key1, const char *key2);
#endif

ht_error_t list_insert(hashtab_t *ctx, bucket_t *bucket, const char *key, data_t *data) {
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
}

//TODO optimize with asm
ht_error_t list_search(hashtab_t *ctx, bucket_t *bucket, const char *key, data_t **result) {
    list_t *head = bucket->head->next;

    #if defined(_OPTIMIZE_STRCMP)
        __m256i search_key = _mm256_lddqu_si256((const __m256i *)key);
    #endif

    while(head != bucket->head) {
        #if defined(_OPTIMIZE_STRCMP)
            if(cmp_key_ymm(search_key, head->data.key)) {
                *result = &head->data;
                break;
            }
        #else
            if(cmp_key(key, head->data.key)) {
                *result = &head->data;
                break;
            }
        #endif
        head = head->next;
    }
    return HASHTAB_SUCCESS;
}


#if defined(_OPTIMIZE_STRCMP)
    bool __attribute__ ((noinline)) cmp_key_ymm(__m256i key1_ymm, const char *key2) {
        __m256i key2_ymm = _mm256_lddqu_si256((const __m256i *)key2);
        __m256i cmp = _mm256_cmpeq_epi8(key1_ymm, key2_ymm);
        uint32_t cmp_mask = (uint32_t)_mm256_movemask_epi8(cmp);
        return (cmp_mask == 0xffffffff);
    }
#else
    bool __attribute__ ((noinline)) cmp_key(const char *key1, const char *key2) {
        return (strcmp(key1, key2) == 0);
    }
#endif

//TODO check this function works
ht_error_t list_remove(hashtab_t *ctx, bucket_t *bucket, const char *key) {
    list_t *head = bucket->head->next;
    while(head != bucket->head) {
        if(strcmp(key, head->data.key) == 0) {
            head->prev->next = head->next;
            head->next->prev = head->prev;
            _RETURN_IF_ERROR(ht_storage_free_list(ctx, head));
            break;
        }
        head = head->next;
    }
    return HASHTAB_SUCCESS;
}

