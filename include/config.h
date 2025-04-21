/*============================================================================*/
#ifndef CONFIG_H
#define CONFIG_H
/*============================================================================*/
/**
* @brief                Enables optimization of hash function with x86
                        intrinsic. It is fully safe to use this optimization if
                        your processor supports SSE 4.2
*/
#define _OPTIMIZE_HASH

/*----------------------------------------------------------------------------*/
/**
* @brief                Enables optimization of strcmp().

* @note                 Optimization uses comparison of YMM register. It can be
                        used if your processor supports SSE 4.2. See function
                        cmp_key_optimized() for more info.
*/
#define _OPTIMIZE_STRCMP

/*----------------------------------------------------------------------------*/
/**
* @brief                Enables optimization of list_search_default().

* @note                 Optimization helps to avoid saving YMM register value
                        in memory before calling string comparison function.
*/
#define _OPTIMIZE_SEARCH

/*============================================================================*/
#endif
/*============================================================================*/
