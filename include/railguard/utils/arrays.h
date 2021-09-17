#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <railguard/core/window.h>


// --=== Types ===--

/**
 * @brief Smart pointer to a fixed count dynamically allocated array.
 */
typedef struct rg_array
{
    /**
     * @brief Number of elements in the array.
     */
    size_t count;
    /**
     * @brief Pointer to the first element of the array.
     * @warning There must be enough allocated memory after it to hold the entire array, which is ``count * sizeof(T)`` where T is the
     * type of a single element.
     */
    void *data;
} rg_array;


/**
 * Resizable dynamically allocated array.
 */
typedef struct rg_vector
{
    /** @brief Number of elements in the vector.
     *  @invariant Smaller or equal than capacity.
     */
    size_t count;
    /** @brief Maximum number of elements that the allocation can fit without resizing.
     *  A value of 0 indicates that the vector is not allocated (data is NULL).
     */
    size_t capacity;
    /** @brief Size of a single element. */
    size_t element_size;
    /** @brief Pointer to the first element of the vector
     *  @invariant Is NULL if capacity is 0, otherwise there is enough memory allocated after the pointed location to contain
     *  capacity * element_size bytes.
     */
    void *data;
} rg_vector;

// --=== Arrays ===--

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
void     rg_destroy_array(rg_array *array);

// --=== Vectors ===---

/**
 * Allocates the given unallocated vector.
 * @param initial_capacity
 * @param element_size
 * @param dest_vector is a pointer to the vector that will be allocated. It must be an unallocated vector.
 */
bool rg_create_vector(size_t initial_capacity, size_t element_size, rg_vector *dest_vector);
/**
 * Cleans up the given vector. It will become unallocated and unusable without a new call to rg_create_vector.
 * @param vector is the vector that will be deleted.
 */
void rg_destroy_vector(rg_vector *vector);
/**
 * Ensures that the vector has enough allocated memory for the given capacity. Resizes it if necessary.
 * @param vector is a pointer to the vector that is acted on.
 * @param required_minimum_capacity is the number of bytes that must be able to fit in the vector after the function call.
 * @warning If the vector is not allocated (capacity == 0 or data == NULL), rg_create_vector should be called instead.
 */
void rg_vector_ensure_capacity(rg_vector *vector, size_t required_minimum_capacity);
/**
 * Pushes a new element at the end of the vector. Resizes it if necessary.
 * @param vector is a pointer to the vector that is acted on.
 * @param element is a pointer to the data that will be added.
 */
void rg_vector_push_back(rg_vector *vector, void *element);
/**
 * Removes the last element from the vector.
 * @param vector is a pointer to the vector that is acted on.
 */
void rg_vector_pop_back(rg_vector *vector);
/**
 * Checks if a vector is empty.
 * @param vector
 * @returns true if the vector is empty, false otherwise.
 */
bool rg_vector_is_empty(rg_vector *vector);
/**
 * Gets an element from the vector.
 * @param vector The vector where the element will be fetched.
 * @param pos The index of the element in the vector.
 * @return a pointer to that element. Returns NULL if the index was out of bounds.
 * @warning do not modify values at that pointer beyond element_size bytes after it.
 */
void *rg_vector_get_element(rg_vector *vector, size_t pos);