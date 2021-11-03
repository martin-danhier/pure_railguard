#include "railguard/utils/io.h"

#include <stdio.h>
#include <stdlib.h>

bool rg_load_file_binary(rg_string file_name, void **data, size_t *size)
{
    bool result = false;

    // Open file
    RG_AS_CSTR(file_name, file_path)
    {
        FILE *file = NULL;
        DEFER(file = fopen(file_path, "rb"), fclose(file))
        {
            if (file != NULL)
            {
                // Get file size
                fseek(file, 0, SEEK_END);
                *size = ftell(file);

                // Go back to the beginning of the file
                fseek(file, 0, SEEK_SET);

                // Allocate memory
                *data = malloc(*size);
                if (*data != NULL)
                {
                    // Read file
                    size_t read_count = fread(*data, *size, 1, file);
                    // Handle result
                    if (read_count == 1)
                    {
                        // Everything worked
                        result = true;
                    }
                }
#ifndef UNIT_TESTS
                else
                {
                    fprintf(stderr, "Failed to allocate memory for file: %s\n", file_path);
                }
#endif
            }
            else
            {
#ifndef UNIT_TESTS
                fprintf(stderr, "Failed to open file: %s\n", file_path);
#endif
                // Exit defer without calling fclose
                break;
            }
        }
    }

    return result;
}
