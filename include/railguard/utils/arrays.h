#pragma once

#include <railguard/core/window.h>

#include <stdbool.h>
#include <stddef.h>

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

typedef struct rg_vector_it
{
    rg_vector *vector;
    size_t     next_index;
    void      *value;
} rg_vector_it;

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
void     rg_destroy_array(rg_array *p_array);

// --=== Vectors ===---

/**
 * Allocates the given unallocated vector.
 * @param initial_capacity
 * @param element_size
 * @param p_dest_vector is a pointer to the vector that will be allocated. It must be an unallocated vector.
 */
bool rg_create_vector(size_t initial_capacity, size_t element_size, rg_vector *p_dest_vector);
/**
 * Cleans up the given p_vector. It will become unallocated and unusable without a new call to rg_create_vector.
 * @param p_vector is the p_vector that will be deleted.
 */
void rg_destroy_vector(rg_vector *p_vector);
/**
 * Ensures that the p_vector has enough allocated memory for the given capacity. Resizes it if necessary.
 * @param p_vector is a pointer to the p_vector that is acted on.
 * @param required_minimum_capacity is the number of bytes that must be able to fit in the p_vector after the function call.
 * @warning If the p_vector is not allocated (capacity == 0 or data == NULL), rg_create_vector should be called instead.
 */
void rg_vector_ensure_capacity(rg_vector *p_vector, size_t required_minimum_capacity);
/**
 * Pushes a new p_data at the end of the p_vector. Resizes it if necessary.
 * @param p_vector is a pointer to the p_vector that is acted on.
 * @param p_data is a pointer to the data that will be added.
 * @returns a pointer to the new element if it worked, NULL otherwise.
 */
void *rg_vector_push_back(rg_vector *p_vector, void *p_data);
/**
 * Pushes a new element at the end of the p_vector, and resizes it if necessary. Though, no element will be copied
 * and it will need to be copied manually.
 * @param p_vector is a pointer to the p_vector that is acted on.
 * @return  a pointer to the new element if it worked, NULL otherwise.
 */
void *rg_vector_push_back_no_data(rg_vector *p_vector);
/**
 * Removes the last element from the p_vector.
 * @param p_vector is a pointer to the p_vector that is acted on.
 */
void rg_vector_pop_back(rg_vector *p_vector);
/**
 * Checks if a p_vector is empty.
 * @param p_vector
 * @returns true if the p_vector is empty, false otherwise.
 */
bool rg_vector_is_empty(rg_vector *p_vector);
/**
 * Gets an element from the p_vector.
 * @param p_vector The p_vector where the element will be fetched.
 * @param pos The index of the element in the p_vector.
 * @return a pointer to that element. Returns NULL if the index was out of bounds.
 * @warning do not modify values at that pointer beyond element_size bytes after it.
 */
void *rg_vector_get_element(rg_vector *p_vector, size_t pos);
/**
 * Sets an element in the p_vector
 * @param p_vector is the p_vector where an element will be modified.
 * @param pos is the index of the element to modify in the p_vector.
 * @param p_data is a pointer to the data that will be copied in the p_vector. This data must live throughout the call to this
 * function, but may be deleted afterwards because it will be copied.
 * @return the address of the modified element if it worked, NULL otherwise.
 * @warning in case of an error, the previous value is not guaranteed to still be present.
 */
void *rg_vector_set_element(rg_vector *p_vector, size_t pos, void *p_data);
/**
 * @param p_vector
 * @return the index of the current last element of the vector
 */
size_t rg_vector_last_index(rg_vector *p_vector);
/**
 * Copies the value at index srcPos to the element at index srcPos.
 * @param p_vector is the p_vector where an element will be copied.
 * @param srcPos index of the copied element in the vector.
 * @param dstPos index of the element where the src element will be copied
 * @return true if it worked, false otherwise
 * @example With an array comparison, it would look like: arr[dstPos] = arr[srcPos]
 */
bool rg_vector_copy(rg_vector *p_vector, size_t srcPos, size_t dstPos);
rg_vector_it rg_vector_iterator(rg_vector *p_vector);
bool         rg_vector_next(rg_vector_it *it);