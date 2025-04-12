#include <stdio.h>
#include <string.h>

#include "hashtab.h"
#include "parse_flags.h"
#include "colors.h"

struct flag_t {
    const char *long_name;
    ht_error_t (*handler)(hashtab_t *, int *, int, const char *[]);
};

static ht_error_t handler_data(hashtab_t *ctx, int *current, int argc, const char *argv[]);
static ht_error_t handler_test(hashtab_t *ctx, int *current, int argc, const char *argv[]);
static ht_error_t handler_parse(hashtab_t *ctx, int *current, int argc, const char *argv[]);

static const flag_t SupportedFlags[] = {
    {.long_name = "--data", .handler = handler_data},
    {.long_name = "--test", .handler = handler_test},
    {.long_name = "--parse", .handler = handler_parse},
};

static const size_t SupportedFlagsSize = sizeof(SupportedFlags) /
                                         sizeof(SupportedFlags[0]);

ht_error_t parse_flags(hashtab_t *ctx, int argc, const char *argv[]) {
    int current = 1;
    while(current < argc) {
        bool flag_found = false;
        for(size_t i = 0; i < SupportedFlagsSize; i++) {
            if(strcmp(argv[current], SupportedFlags[i].long_name) == 0) {
                current++;
                flag_found = true;
                _RETURN_IF_ERROR(SupportedFlags[i].handler(ctx, &current, argc, argv));
                break;
            }
        }
        if(!flag_found) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unknown flag '%s'\n", argv[current]);
            return HASHTAB_UNEXPECTED_PARAMS;
        }
    }
    return HASHTAB_SUCCESS;
}

ht_error_t handler_data(hashtab_t *ctx, int *current, int argc, const char *argv[]) {
    if(*current >= argc) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Flag '%s' expected to have a parameter with data file name\n",
                     argv[*current - 1]);
        return HASHTAB_UNEXPECTED_PARAMS;
    }

    ctx->data_file = argv[*current];
    (*current)++;
    return HASHTAB_SUCCESS;
}

ht_error_t handler_test(hashtab_t *ctx, int *current, int argc, const char *argv[]) {
    if(*current >= argc) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Flag '%s' expected to have a parameter with test file name\n",
                     argv[*current - 1]);
        return HASHTAB_UNEXPECTED_PARAMS;
    }

    ctx->test_file = argv[*current];
    (*current)++;
    return HASHTAB_SUCCESS;
}

ht_error_t handler_parse(hashtab_t *ctx, int *current, int argc, const char *argv[]) {
    if(*current + 1 >= argc) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Flag '%s' expected to have two parameters with name of file to parse and output file\n",
                     argv[*current - 1]);
        return HASHTAB_UNEXPECTED_PARAMS;
    }

    ctx->data_file = argv[*current];
    ctx->output_file = argv[*current + 1];

    ctx->parse_flag = true;
    (*current) += 2;
    return HASHTAB_SUCCESS;
}
