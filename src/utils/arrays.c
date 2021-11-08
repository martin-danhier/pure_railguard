#include "railguard/utils/arrays.h"

#include <railguard/utils/memory.h>

#include <assert.h>
#include <string.h>

// --=== Arrays ===--

rg_array rg_create_array(size_t size, size_t element_size)
{
    rg_array array = {
        .count = size,
        .data  = rg_malloc(size * element_size),
    };
    assert(array.data != NULL);
    return array;
}

rg_array rg_create_array_zeroed(size_t size, size_t element_size)
{
    rg_array array = {
        .count = size,
        .data  = rg_calloc(size, element_size),
    };
    assert(array.data != NULL);
    return array;
}

void rg_destroy_array(rg_array *p_array)
{
    // Free pointer and set values to NULL
    rg_free(p_array->data);
    p_array->data  = NULL;
    p_array->count = 0;
}

// --=== Vectors ===--

bool rg_create_vector(size_t initial_capacity, size_t element_size, rg_vector *p_dest_vector)
{
    // Init default fields
    p_dest_vector->capacity      = initial_capacity;
    p_dest_vector->element_size  = element_size;
    p_dest_vector->growth_amount = 1;
    p_dest_vector->count         = 0;
    p_dest_vector->data          = rg_malloc(element_size * initial_capacity);

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
    rg_free(p_vector->data);
    p_vector->data = NULL;
}

void rg_vector_ensure_capacity(rg_vector *p_vector, size_t required_minimum_capacity)
{
    // Resize if smaller than required capacity
    if (p_vector->capacity < required_minimum_capacity)
    {
        // Grow with the growth amount, except if it does not reach the required capacity
        size_t new_capacity = p_vector->count + p_vector->growth_amount;
        if (required_minimum_capacity > new_capacity)
        {
            new_capacity = required_minimum_capacity;
        }
        else
        {
            // Multiply the growth amount, that way the more we push in a vector, the more it will try to anticipate
            // Same behavior as cpp vector
            p_vector->growth_amount *= 2;
        }

        p_vector->data     = rg_realloc(p_vector->data, new_capacity * p_vector->element_size);
        p_vector->capacity = new_capacity;

    }

}

void *rg_vector_push_back(rg_vector *p_vector, void *p_data)
{
    // Make sure that there is enough room in the allocation for this new p_data
    size_t new_count = p_vector->count + 1;
    rg_vector_ensure_capacity(p_vector, new_count);

    // Add the p_data:
    void *p_element = ((char *) p_vector->data) + (p_vector->count * p_vector->element_size);
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
    if (p_vector == NULL)
    {
        return NULL;
    }

    // Make sure that there is enough room in the allocation for this new p_data
    size_t new_count = p_vector->count + 1;
    rg_vector_ensure_capacity(p_vector, new_count);

    void *p_element = ((char *) p_vector->data) + (p_vector->count * p_vector->element_size);

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
        return ((char *) p_vector->data) + (pos * p_vector->element_size);
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
        .index  = -1,
        .vector = p_vector,
    };
}

bool rg_vector_next(rg_vector_it *it)
{
    // Increment index
    it->index++;

    rg_vector *vec = it->vector;
    if (it->index < vec->count)
    {
        it->value = ((char *) vec->data) + (it->index * vec->element_size);
        return true;
    }

    // When the loop is finished, there are no more elements
    it->value = NULL;
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

    return memcpy(((char *) p_vector->data) + (dstPos * p_vector->element_size),
                  ((char *) p_vector->data) + (srcPos * p_vector->element_size),
                  p_vector->element_size)
           != NULL;
}

void rg_vector_clear(rg_vector *p_vector)
{
    p_vector->count = 0;
}