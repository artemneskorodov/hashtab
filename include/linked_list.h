/*============================================================================*/
#ifndef LINKED_LIST_H
#define LINKED_LIST_H
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
    /**
    * @brief                Macro to call list search function

    * @param _bucket        bucket_t * - Pointer to bucket that stores linked
                            list with value to search.

    * @param _key           const char * - Pointer to keyword, that must be
                            32 bytes long.
    * @param _result        data_t ** - Result of search is written to this
                            pointer.

    * @note                 Calls list_search_optimized() if optimization is
                            enabled and list_search_default() if not.
    */
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
