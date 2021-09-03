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

/**
 * @brief Creates an array but do NOT set each element to zero.
 * @warning Use it only if you initialize all fields directly. By default, fields have an undefined value.
 * @see rg_create_array_zeroed to zero all the fields when allocating.
 * @param size Number of elements in the array
 * @param element_size Size of a single element of the array
 * @return The array struct
 */
rg_array rg_create_array(size_t size, size_t element_size);
/**
 * @brief Creates an array and set each byte to zero
 * @param size Number of elements in the array
 * @param element_size Size of a single element of the array
 * @return The array struct
 */
rg_array rg_create_array_zeroed(size_t size, size_t element_size);
void rg_destroy_array(rg_array *array);