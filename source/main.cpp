#include <stdlib.h>

#include "hashtab.h"
#include "parse_text.h"
#include "colors.h"

#define _EXIT_IF_ERROR(...) {                                                  \
    ht_error_t _error_code = (__VA_ARGS__);                                    \
    if(_error_code != HASHTAB_SUCCESS) {                                       \
        color_printf(RED_TEXT,                                                 \
                     BOLD_TEXT,                                                \
                     DEFAULT_BACKGROUND,                                       \
                     "[FAIL] ");                                               \
        color_printf(WHITE_TEXT,                                               \
                     NORMAL_TEXT,                                              \
                     DEFAULT_BACKGROUND,                                       \
                     "in func '%s'\n",                                         \
                     #__VA_ARGS__);                                            \
        hashtab_dtor(&_ctx);                                                   \
        return (int)_error_code;                                               \
    }                                                                          \
    color_printf(GREEN_TEXT,                                                   \
                 BOLD_TEXT,                                                    \
                 DEFAULT_BACKGROUND,                                           \
                 "[ OK ] ");                                                   \
    color_printf(WHITE_TEXT,                                                   \
                 NORMAL_TEXT,                                                  \
                 DEFAULT_BACKGROUND,                                           \
                 "in func '%s'\n",                                             \
                 #__VA_ARGS__);                                                \
}

int main(int argc, const char *argv[]) {
    hashtab_t _ctx = {};
    _EXIT_IF_ERROR(hashtab_ctor(&_ctx, argc, argv));

    switch(_ctx.run_mode) {
        case HASHTAB_MODE_RUN_TEST: {
            _EXIT_IF_ERROR(hashtab_read_data(&_ctx));
            _EXIT_IF_ERROR(hashtab_run_tests(&_ctx));
            break;
        }
        case HASHTAB_MODE_TEST_LOAD: {
            _EXIT_IF_ERROR(hashtab_test_load(&_ctx));
            break;
        }
        case HASHTAB_MODE_PARSE_TEXT: {
            _EXIT_IF_ERROR(parse_text(&_ctx));
            break;
        }
        default: {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unknown run mode.\n");
            hashtab_dtor(&_ctx);
            return EXIT_FAILURE;
        }
    }
    hashtab_dtor(&_ctx);
    return EXIT_SUCCESS;
}
