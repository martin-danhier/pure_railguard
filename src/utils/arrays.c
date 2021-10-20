#include "railguard/utils/arrays.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// --=== Arrays ===--

rg_array rg_create_array(size_t size, size_t element_size)
{
    rg_array array = {
        .count = size,
        .data  = malloc(size * element_size),
    };
    assert(array.data != NULL);
    return array;
}

rg_array rg_create_array_zeroed(size_t size, size_t element_size)
{
    rg_array array = {
        .count = size,
        .data  = calloc(size, element_size),
    };
    assert(array.data != NULL);
    return array;
}

void rg_destroy_array(rg_array *p_array)
{
    // Free pointer and set values to NULL
    free(p_array->data);
    p_array->data  = NULL;
    p_array->count = 0;
}

// --=== Vectors ===--

bool rg_create_vector(size_t initial_capacity, size_t element_size, rg_vector *p_dest_vector)
{
    // Init default fields
    p_dest_vector->capacity     = initial_capacity;
    p_dest_vector->element_size = element_size;
    p_dest_vector->count        = 0;
    p_dest_vector->data         = malloc(element_size * initial_capacity);

    if (p_dest_vector->data == NULL)
    {
        return false;
    }
    return true;
}

void rg_destroy_vector(rg_vector *p_vector)
{
    p_vector->capacity = 0;
    p_vector->count    = 0;
    free(p_vector->data);
    p_vector->data = NULL;
}

void rg_vector_ensure_capacity(rg_vector *p_vector, size_t required_minimum_capacity)
{
    // Resize if smaller than required capacity
    if (p_vector->capacity < required_minimum_capacity)
    {
        p_vector->data     = realloc(p_vector->data, required_minimum_capacity * p_vector->element_size);
        p_vector->capacity = required_minimum_capacity;
    }
}

void *rg_vector_push_back(rg_vector *p_vector, void *p_data)
{
    // Make sure that there is enough room in the allocation for this new p_data
    size_t new_count = p_vector->count + 1;
    rg_vector_ensure_capacity(p_vector, new_count);

    // Add the p_data:
    void *p_element = p_vector->data + (p_vector->count * p_vector->element_size);
    void *res       = memcpy(p_element, p_data, p_vector->element_size);

    if (res != NULL)
    {
        p_vector->count = new_count;
        return p_element;
    }
    return NULL;
}

void *rg_vector_push_back_no_data(rg_vector *p_vector)
{
    // Make sure that there is enough room in the allocation for this new p_data
    size_t new_count = p_vector->count + 1;
    rg_vector_ensure_capacity(p_vector, new_count);

    void *p_element = p_vector->data + (p_vector->count * p_vector->element_size);

    p_vector->count = new_count;
    return p_element;
}

void rg_vector_pop_back(rg_vector *p_vector)
{
    // Remove 1 at the count
    // -> Don't do anything else because the memory will be overwritten by the next push anyway
    // -> Check for 0 to avoid underflow if the p_vector was empty.
    if (p_vector->count > 0)
    {
        p_vector->count -= 1;
    }
}

bool rg_vector_is_empty(rg_vector *p_vector)
{
    return p_vector->count == 0;
}

void *rg_vector_get_element(rg_vector *p_vector, size_t pos)
{
    if (pos < p_vector->count)
    {
        return p_vector->data + (pos * p_vector->element_size);
    }
    else
    {
        return NULL;
    }
}

size_t rg_vector_last_index(rg_vector *p_vector)
{
    return p_vector->count - 1;
}

void *rg_vector_set_element(rg_vector *p_vector, size_t pos, void *p_data)
{
    void *p_element = rg_vector_get_element(p_vector, pos);
    if (p_element != NULL)
    {
        void *res = memcpy(p_element, p_data, p_vector->element_size);
        if (res != NULL)
        {
            return p_element;
        }
    }
    return NULL;
}

rg_vector_it rg_vector_iterator(rg_vector *p_vector)
{
    return (rg_vector_it) {
        .next_index = 0,
        .vector     = p_vector,
    };
}

bool rg_vector_next(rg_vector_it *it)
{
    rg_vector *vec = it->vector;
    while (it->next_index < vec->count)
    {
        size_t i = it->next_index;

        // Increment index
        it->next_index++;

        it->value = vec->data + (i * vec->element_size);
        return true;
    }

    // When the loop is finished, there are no more elements
    return false;
}

bool rg_vector_copy(rg_vector *p_vector, size_t srcPos, size_t dstPos)
{
    if (srcPos >= p_vector->count || dstPos >= p_vector->count)
    {
        // The indexes must be valid
        return false;
    }

    // Don't bother if the items are the same index, it's already done
    if (srcPos == dstPos)
    {
        return true;
    }

    return memcpy(p_vector->data + (dstPos * p_vector->element_size),
                    p_vector->data + (srcPos * p_vector->element_size),
                    p_vector->element_size)
           != NULL;
}