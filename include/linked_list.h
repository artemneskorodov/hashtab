#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "hashtab.h"

ht_error_t list_insert(hashtab_t *ctx, bucket_t *bucket, const char *key, data_t *data);
ht_error_t list_search(hashtab_t *ctx, bucket_t *bucket, const char *key, data_t **result);
ht_error_t list_remove(hashtab_t *ctx, bucket_t *bucket, const char *key);

#endif
