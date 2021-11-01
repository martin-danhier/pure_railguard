#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// --=== Constants ===--

#define RG_STORAGE_NULL_ID 0

// --=== Types ===--

/**
 * A storage is a data structure with the following properties:
 * - New elements can be pushed in the storage, which returns a new unique id for the element
 * - Using the id, the element can be read, updated or deleted
 * - The allocation of the data is handled by the storage (data is copied inside it from the pointer when pushed)
 * - Inside the storage, the data is kept tightly packed, even if elements are deleted
 * - With an iterator, it is possible to read all the elements in the storage
 */
typedef struct rg_storage rg_storage;

typedef uint32_t rg_storage_id;

typedef struct rg_storage_it
{
    rg_storage   *storage;
    rg_storage_id id;
    void         *value;
} rg_storage_it;

// --=== Functions ===--

/**
 * Creates a new storage with the given element size.
 * @return The new storage, or NULL if there was an error.
 */
rg_storage *rg_create_storage(size_t element_size);
void        rg_destroy_storage(rg_storage **storage);

/**
 * Adds a new element to the storage and generates a new id for it.
 * @param storage The storage to add the element to.
 * @param data A pointer to the data to add. It will be copied inside the storage, so it can be freed after the call.
 * @return the id of the new element, or RG_STORAGE_NULL_ID if there was an error.
 */
rg_storage_id rg_storage_push(rg_storage *storage, void *data);
void         *rg_storage_get(rg_storage *storage, rg_storage_id id);
void          rg_storage_erase(rg_storage *storage, rg_storage_id id);
rg_storage_it rg_storage_iterator(rg_storage *storage);
bool          rg_storage_next(rg_storage_it *it);
size_t        rg_storage_count(rg_storage *storage);
