#include "railguard/utils/io.h"

#include <stdio.h>
#include <stdlib.h>

bool rg_load_file_binary(const char *file_name, void **data, size_t *size)
{
    // Open file
    FILE *file = fopen(file_name, "rb");
    if (file == NULL)
    {
#ifndef UNIT_TESTS
        fprintf(stderr, "Failed to open file: %s\n", file_name);
#endif
        return false;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);

    // Go back to the beginning of the file
    fseek(file, 0, SEEK_SET);

    // Allocate memory
    *data = malloc(*size);
    if (*data == NULL)
    {
#ifndef UNIT_TESTS
        fprintf(stderr, "Failed to allocate memory for file: %s\n", file_name);
#endif
        return false;
    }

    // Read file
    size_t read_count = fread(*data, *size, 1, file);

    // Close file
    fclose(file);

    // Handle result
    if (read_count != 1)
    {
        return false;
    }

    return true;
}
