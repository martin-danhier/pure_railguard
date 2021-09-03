#include "railguard/utils/arrays.h"

#include <stdlib.h>

rg_array rg_create_array(size_t size, size_t element_size)
{
    return (rg_array) {
        .size = size,
        .data = malloc(size * element_size),
    };
}

rg_array rg_create_array_zeroed(size_t size, size_t element_size)
{
    return (rg_array) {
        .size = size,
        .data = calloc(size, element_size),
    };
}

void rg_destroy_array(rg_array *array)
{
    // Free pointer and set values to NULL
    free(array->data);
    array->data = NULL;
    array->size = 0;
}
