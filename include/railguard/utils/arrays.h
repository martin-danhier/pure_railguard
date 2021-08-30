#pragma once

#include <stddef.h>

/**
 * @brief Smart pointer to a fixed size array.
 */
typedef struct rg_array
{
    /**
     * @brief Number of elements in the array.
     */
    size_t size;
    /**
     * @brief Pointer to the first element of the array.
     * @warning There must be enough allocated memory after it to hold the entire array, which is ``size * sizeof(T)`` where T is the
     * type of a single element.
     */
    void *data;
} rg_array;

rg_array rg_create_array(size_t size, size_t element_size);
void rg_destroy_array(rg_array *array);