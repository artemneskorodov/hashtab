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

#if defined(_NO_DUMP)
/*----------------------------------------------------------------------------*/
    #define _HT_DUMP_CTOR(_ctx, _filename)
    #define _HT_DUMP(_ctx, _label)
    #define _HT_DUMP_DTOR(_ctx)
/*----------------------------------------------------------------------------*/
#else
/*----------------------------------------------------------------------------*/
    /**
    * @brief                Calling hashtab_dump_ctor().

    * @param _ctx           hashtab_t * - Pointer to hashtab context.
    * @param _filename      const char * - Name of file to write dump.

    * @note                 Expands in nothing if dump is disabled.
    */
    #define _HT_DUMP_CTOR(_ctx, _filename) hashtab_dump_ctor((_ctx),           \
                                                             (_filename));
    /*------------------------------------------------------------------------*/
    /**
    * @brief                Calling hashtab_dump().

    * @param _ctx           hashtab_t * - Pointer to hashtab context.
    * @param _label         const char * - Label for dump.

    * @note                 Expands in nothing if dump is disabled.
    */
    #define _HT_DUMP(_ctx, _label)         hashtab_dump     ((_ctx),           \
                                                             (_label),         \
                                                             __FILE__,         \
                                                             __PRETTY_FUNCT__, \
                                                             __LINE__);
    /*------------------------------------------------------------------------*/
    /**
    * @brief                Calling hashtab_dump_dtor().

    * @param _ctx           hashtab_t * - Pointer to hashtab context.

    * @note                 Expands in nothing if dump is disabled.
    */
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
