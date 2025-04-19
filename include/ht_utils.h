/*============================================================================*/
#ifndef HT_UTILS_H
#define HT_UTILS_H
/*============================================================================*/
/**
* @file     ht_utils.h
* @author   Artem Neskorodov
* @date     2024-04-19
* @brief    Functions to dump hashtab, allocate nodes and other utils, which
            are used in hashtab.
* @note     Use macros to access dump functions, as dump may be unhelpful when
            using it with big number of buckets.
*/
/*============================================================================*/

#include "hashtab.h"

/*============================================================================*/

#define _NO_DUMP

/*============================================================================*/

#if defined(_NO_DUMP)
/*----------------------------------------------------------------------------*/
    #define _HT_DUMP_CTOR(_ctx, _filename)
    #define _HT_DUMP(_ctx, _label)
    #define _HT_DUMP_DTOR(_ctx)
/*----------------------------------------------------------------------------*/
#else
/*----------------------------------------------------------------------------*/
    #define _HT_DUMP_CTOR(_ctx, _filename) hashtab_dump_ctor((_ctx),           \
                                                             (_filename));
    #define _HT_DUMP(_ctx, _label)         hashtab_dump     ((_ctx),           \
                                                             (_label),         \
                                                             __FILE__,         \
                                                             __PRETTY_FUNCT__, \
                                                             __LINE__);
    #define _HT_DUMP_DTOR(_ctx)            hashtab_dump_dtor(_ctx);
/*----------------------------------------------------------------------------*/
#endif

/*============================================================================*/

ht_error_t hashtab_dump_ctor       (hashtab_t      *ctx,
                                    const char     *filename);

ht_error_t hashtab_dump            (hashtab_t      *ctx,
                                    const char     *label,
                                    const char     *file,
                                    const char     *func,
                                    int             line);

ht_error_t hashtab_dump_dtor       (hashtab_t      *ctx);

ht_error_t ht_storage_ctor         (hashtab_t      *ctx);

ht_error_t ht_storage_get_list     (hashtab_t      *ctx,
                                    list_t        **list);

ht_error_t ht_storage_free_list    (hashtab_t      *ctx,
                                    list_t         *list);

ht_error_t ht_storage_dtor         (hashtab_t      *ctx);

size_t     file_size               (FILE           *file);

ht_error_t parse_flags             (hashtab_t      *ctx,
                                    int             argc,
                                    const char     *argv[]);

ht_error_t parse_text              (hashtab_t      *ctx);

/*============================================================================*/
#endif
/*============================================================================*/
