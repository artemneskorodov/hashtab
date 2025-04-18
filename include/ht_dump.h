#ifndef HT_DUMP_H
#define HT_DUMP_H
/*============================================================================*/
/**
* @file     linked_list.cpp
* @author   Artem Neskorodov
* @date     2024-04-18
* @brief    File with linked_list access implementation.
*/
/*============================================================================*/

#include "hashtab.h"

/*============================================================================*/

#define _NO_DUMP

/*============================================================================*/

#ifdef _NO_DUMP
    /**
     * @defgroup            DumpDisabled
     * @brief               Macros to use dump when it is turned off.
     * @{
    */
    #define _HT_DUMP_CTOR(_ctx, _filename)
    #define _HT_DUMP(_ctx, _label)
    #define _HT_DUMP_DTOR(_ctx)
    /** @} */
#else
    /**
     * @defgroup            DumpEnabled
     *@brief               Macros to use dump when it is turned on.
     * @{
    */
    #define _HT_DUMP_CTOR(_ctx, _filename) hashtab_dump_ctor((_ctx),           \
                                                             (_filename));

    #define _HT_DUMP(_ctx, _label)         hashtab_dump     ((_ctx),           \
                                                             (_label),         \
                                                             __FILE__,         \
                                                             __PRETTY_FUNCT__, \
                                                             __LINE__);

    #define _HT_DUMP_DTOR(_ctx)            hashtab_dump_dtor(_ctx);
    /** @} */
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
