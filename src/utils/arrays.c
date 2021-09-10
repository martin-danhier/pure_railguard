#include "railguard/utils/arrays.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// --=== Arrays ===--

rg_array rg_create_array(size_t size, size_t element_size)
{
    return (rg_array) {
        .count = size,
        .data  = malloc(size * element_size),
    };
}

rg_array rg_create_array_zeroed(size_t size, size_t element_size)
{
    return (rg_array) {
        .count = size,
        .data  = calloc(size, element_size),
    };
}

void rg_destroy_array(rg_array *array)
{
    // Free pointer and set values to NULL
    free(array->data);
    array->data  = NULL;
    array->count = 0;
}

// --=== Vectors ===--

void rg_create_vector(size_t initial_capacity, size_t element_size, rg_vector *dest_vector)
{
    // Init default fields
    dest_vector->capacity     = initial_capacity;
    dest_vector->element_size = element_size;
    dest_vector->count        = 0;
    dest_vector->data         = malloc(element_size * initial_capacity);
}

void rg_destroy_vector(rg_vector *vector)
{
    vector->capacity = 0;
    vector->count    = 0;
    free(vector->data);
    vector->data = NULL;
}

void rg_vector_ensure_capacity(rg_vector *vector, size_t required_minimum_capacity)
{
    // Resize if smaller than required capacity
    if (vector->capacity < required_minimum_capacity)
    {
        vector->data     = realloc(vector->data, required_minimum_capacity * vector->element_size);
        vector->capacity = required_minimum_capacity;
    }
}

void rg_vector_push_back(rg_vector *vector, void *element)
{
    // Make sure that there is enough room in the allocation for this new element
    size_t new_count = vector->count + 1;
    rg_vector_ensure_capacity(vector, new_count);

    // Add the element:
    memcpy(vector->data + (vector->count * vector->element_size), element, vector->element_size);
    vector->count = new_count;
}

void rg_vector_pop_back(rg_vector *vector)
{
    // Remove 1 at the count
    // -> Don't do anything else because the memory will be overwritten by the next push anyway
    // -> Check for 0 to avoid underflow if the vector was empty.
    if (vector->count > 0) {
        vector->count -= 1;
    }
}

bool rg_vector_is_empty(rg_vector *vector)
{
    return vector->count == 0;
}

void *rg_vector_get_element(rg_vector *vector, size_t pos)
{
    if (pos < vector->count)
    {
        return vector->data + (pos * vector->element_size);
    }
    else {
        return NULL;
    }
}
