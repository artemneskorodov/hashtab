#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "hashtab.h"
#include "parse_text.h"
#include "colors.h"
#include "utils.h"

static const size_t BufferWordsNum = 1024;

ht_error_t parse_text(hashtab_t *ctx) {
    FILE *input = fopen(ctx->data_file, "rb");
    if(input == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'\n",
                     ctx->data_file);
        return HASHTAB_OPENING_FILE_ERROR;
    }

    size_t input_size = file_size(input);

    char *buffer = (char *)calloc(input_size + 1 + BufferWordsNum * KeyWordSize, sizeof(char));
    if(buffer == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory for buffer\n");
        fclose(input);
        return HASHTAB_MEMORY_ERROR;
    }
    char *words_buffer = buffer + input_size + 1;
    size_t words_buffer_pos = 0;

    if(fread(buffer, sizeof(char), input_size, input) != input_size) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reading input file.\n");
        fclose(input);
        free(buffer);
        return HASHTAB_READING_FILE_ERROR;
    }
    fclose(input);

    FILE *output = fopen(ctx->output_file, "w");
    if(output == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'\n",
                     ctx->output_file);
        free(buffer);
        return HASHTAB_OPENING_FILE_ERROR;
    }

    for(size_t pos = 0; pos < input_size; ) {
        size_t end = pos;
        while(isalpha(buffer[end])) {
            end++;
        }
        for(size_t i = 0; i < end - pos; i++) {
            words_buffer[words_buffer_pos + i] = buffer[pos + i];
        }
        words_buffer_pos += KeyWordSize + 1;
        if(words_buffer_pos == BufferWordsNum * (KeyWordSize + 1)) {
            fwrite(words_buffer, sizeof(char) * (KeyWordSize + 1), BufferWordsNum, output);
            memset(words_buffer, 0, sizeof(char) * (KeyWordSize + 1) * BufferWordsNum);
            words_buffer_pos = 0;
        }
        pos = end;
        while(!isalpha(buffer[pos])) {
            pos++;
        }
    }

    fwrite(words_buffer, sizeof(char), words_buffer_pos, output);
    fclose(output);
    free(buffer);
    return HASHTAB_SUCCESS;
}
