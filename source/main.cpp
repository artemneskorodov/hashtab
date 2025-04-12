#include <stdlib.h>

#include "hashtab.h"
#include "parse_text.h"

#define _EXIT_IF_ERROR(...) {                   \
    ht_error_t _error_code = (__VA_ARGS__);     \
    if(_error_code != HASHTAB_SUCCESS) {        \
        fprintf(stderr, "[ERROR] %d in call '%s'\n", _error_code, #__VA_ARGS__);\
        hashtab_dtor(&_ctx);                    \
        return (int)_error_code;                \
    }                                           \
    fprintf(stderr, "[OK] '%s'\n", #__VA_ARGS__);\
}

int main(int argc, const char *argv[]) {
    hashtab_t _ctx = {};
    _EXIT_IF_ERROR(hashtab_ctor(&_ctx, argc, argv));
    if(_ctx.parse_flag) {
        _EXIT_IF_ERROR(parse_text(&_ctx));
    }
    else {
        _EXIT_IF_ERROR(hashtab_read_data(&_ctx));
        _EXIT_IF_ERROR(hashtab_run_tests(&_ctx));
        hashtab_dtor(&_ctx);
    }

    return EXIT_SUCCESS;
}
