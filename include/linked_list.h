/*============================================================================*/
#ifndef LINKED_LIST_H
#define LINKED_LIST_H
/*============================================================================*/
/**
* @file     linked_list.h
* @author   Artem Neskorodov
* @date     2024-04-19
* @brief    Header file with functions to access linked list structure.
*/
/*============================================================================*/

#include "hashtab.h"

/*============================================================================*/
/**
* @brief                Enables optimization of strcmp().

* @note                 Optimization uses comparison of YMM register. It can be
                        used if your processor supports SSE 4.2. See function
                        cmp_key_optimized() for more info.
*/
#define _OPTIMIZE_STRCMP
/**
* @brief                Enables optimization of list_search_default().

* @note                 Optimization helps to avoid saving YMM register value
                        in memory before calling string comparison function.
*/
#define _OPTIMIZE_SEARCH

/*============================================================================*/

#if defined(_OPTIMIZE_SEARCH)
/*----------------------------------------------------------------------------*/
    #define LIST_SEARCH(_bucket, _key, _result)                                \
        list_search_optimized((_bucket), (_key), (_result));
/*----------------------------------------------------------------------------*/
#else
/*----------------------------------------------------------------------------*/
    #define LIST_SEARCH(_bucket, _key, _result)                                \
        list_search_default((_bucket), (_key), (_result));
/*----------------------------------------------------------------------------*/
#endif

/*============================================================================*/

ht_error_t list_insert             (hashtab_t      *ctx,
                                    bucket_t       *bucket,
                                    const char     *key,
                                    data_t         *data);

ht_error_t list_search_optimized   (bucket_t       *bucket,
                                    const char     *key,
                                    data_t        **result);

ht_error_t list_search_default     (bucket_t       *bucket,
                                    const char     *key,
                                    data_t        **result);

ht_error_t list_remove             (hashtab_t      *ctx,
                                    bucket_t       *bucket,
                                    const char     *key);

/*============================================================================*/
#endif
/*============================================================================*/
