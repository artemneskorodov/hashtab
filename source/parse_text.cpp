/*============================================================================*/
/**
* @file     parse_text.cpp
* @author   Artem Neskorodov
* @date     2024-04-18
* @brief    File which is used when used wants to parse a text.
*/
/*============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/*============================================================================*/

#include "hashtab.h"
#include "parse_text.h"
#include "colors.h"
#include "utils.h"

/*============================================================================*/
/**
* @brief                Size of buffer to use while writing.
*/
static const size_t BufferWordsNum = 1024;

/*============================================================================*/

static ht_error_t read_input_file(hashtab_t    *ctx,
                                  char        **buffer,
                                  size_t       *input_size);

/*============================================================================*/
/**
* @brief                Parsing text to database format.

* @param ctx            Pointer to hashtab context.

* @result               ht_error_t - overall error code enum.

* @warning              It is not expected to call this function after
                        hashtab_ctor().
*/
ht_error_t parse_text(hashtab_t *ctx) {
    char *buffer = NULL;
    size_t input_size = 0;
    _RETURN_IF_ERROR(read_input_file(ctx, &buffer, &input_size));

    char *words_buffer = buffer + input_size;
    size_t words_buffer_pos = 0;
    FILE *output = fopen(ctx->output_file, "w");
    if(output == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'\n",
                     ctx->output_file);
        free(buffer);
        return HASHTAB_OPENING_FILE_ERROR;
    }

    size_t pos = 0;
    while(!isalpha(buffer[pos])) {
        pos++;
    }

    while(pos < input_size) {
        size_t end = pos;
        while(isalpha(buffer[end]) && pos != input_size) {
            end++;
        }
        for(size_t i = 0; i < end - pos; i++) {
            words_buffer[words_buffer_pos + i] = buffer[pos + i];
        }
        words_buffer_pos += KeyWordSize;
        if(words_buffer_pos == BufferWordsNum * (KeyWordSize)) {
            fwrite(words_buffer,
                   sizeof(char) * (KeyWordSize),
                   BufferWordsNum,
                   output);
            memset(words_buffer,
                   0,
                   sizeof(char) * (KeyWordSize) * BufferWordsNum);
            words_buffer_pos = 0;
        }
        pos = end;
        while(!isalpha(buffer[pos]) && pos != input_size) {
            pos++;
        }
    }

    fwrite(words_buffer, sizeof(char), words_buffer_pos, output);
    fclose(output);
    free(buffer);
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
/**
* @brief                Inserting element in linked list.

* @param ctx            Pointer to hashtab context.
* @param buffer         Place to write read buffer.
* @param input_size     Place to write buffer size.

* @result               ht_error_t - overall error code enum.

* @warning              It is not expected to call this function after
                        hashtab_ctor().
*/
ht_error_t read_input_file(hashtab_t *ctx, char **buffer, size_t *input_size) {
    FILE *input = fopen(ctx->data_file, "rb");
    if(input == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'\n",
                     ctx->data_file);
        return HASHTAB_OPENING_FILE_ERROR;
    }

    *input_size = file_size(input);
    *buffer = (char *)calloc(*input_size + BufferWordsNum * KeyWordSize,
                             sizeof(char));
    if(*buffer == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory for buffer\n");
        fclose(input);
        return HASHTAB_MEMORY_ERROR;
    }

    if(fread(*buffer, sizeof(char), *input_size, input) != *input_size) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reading input file.\n");
        fclose(input);
        free(*buffer);
        return HASHTAB_READING_FILE_ERROR;
    }
    fclose(input);
    return HASHTAB_SUCCESS;
}

/*============================================================================*/
