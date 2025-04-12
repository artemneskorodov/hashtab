#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtab.h"
#include "linked_list.h"
#include "ht_storage.h"
#include "ht_dump.h"

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

    return HASHTAB_SUCCESS;
}

ht_error_t list_search(hashtab_t *ctx, bucket_t *bucket, const char *key, data_t **result) {
    list_t *head = bucket->head->next;

    while(head != bucket->head) {
        if(strcmp(key, head->data.key) == 0) {
            *result = &head->data;
            break;
        }
        head = head->next;
    }
    return HASHTAB_SUCCESS;
}

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

