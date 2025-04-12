#ifndef HT_STORAGE_H
#define HT_STORAGE_H

#include "hashtab.h"

ht_error_t ht_storage_ctor(hashtab_t *ctx);
ht_error_t ht_storage_get_list(hashtab_t *ctx, list_t **list);
ht_error_t ht_storage_free_list(hashtab_t *ctx, list_t *list);
ht_error_t ht_storage_dtor(hashtab_t *ctx);

#endif
