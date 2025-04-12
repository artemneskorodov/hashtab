#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, const char *argv[]) {
    if(argc != 3) {
        fprintf(stderr, "UNEXPECTED PARAMETERS\n");
        return EXIT_FAILURE;
    }
    FILE *file = fopen(argv[1], "wb");

    size_t tests_num = strtoul(argv[2], NULL, 10);
    char buffer[32 + 1] = {};
    for(size_t i = 0; i < tests_num; i++) {
        memset(buffer, 0, 32 + 1);
        sprintf(buffer, "test_string_num_%lu", i);
        fwrite(buffer, sizeof(char), 32 + 1, file);
    }

    fclose(file);

    return EXIT_SUCCESS;
}
