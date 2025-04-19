/*============================================================================*/
#ifndef HT_DUMP_H
#define HT_DUMP_H
/*============================================================================*/
/**
* @file     ht_dump.h
* @author   Artem Neskorodov
* @date     2024-04-19
* @brief    Header file with functions to dump hashtab structure while using.
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

ht_error_t hashtab_dump_ctor   (hashtab_t      *ctx,
                                const char     *filename);

ht_error_t hashtab_dump        (hashtab_t      *ctx,
                                const char     *label,
                                const char     *file,
                                const char     *func,
                                int             line);

ht_error_t hashtab_dump_dtor   (hashtab_t      *ctx);

/*============================================================================*/

#endif
