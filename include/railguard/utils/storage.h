#pragma once

#include <railguard/utils/maps.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// region Storage

// --=== Constants ===--

#define RG_STORAGE_NULL_ID 0

// --=== Types ===--

/**
 * A storage is a data structure with the following properties:\n
 * • New elements can be pushed in the storage, which returns a new unique id for the element\n
 * • Using the id, the element can be read, updated or deleted\n
 * • The allocation of the data is handled by the storage (data is copied inside it from the pointer when pushed)\n
 * • Inside the storage, the data is kept tightly packed, even if elements are deleted\n
 * • With an iterator, it is possible to read all the elements in the storage
 */
typedef struct rg_storage rg_storage;

typedef uint32_t rg_storage_id;

typedef struct rg_storage_it
{
    rg_struct_map_it map_it;
    rg_storage_id    id;
    void            *value;
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
bool          rg_storage_exists(rg_storage *storage, rg_storage_id id);

// endregion

// region Handle Storage

// --=== Types ===--

/**
 * A handle storage is a storage that can be used to store handles.
 * Since handles are pointer, we do not need to manage the allocation of the pointed data.
 * Thus, the underlying implementation will use a hash map instead of a struct map, which is simpler.
 */
typedef struct rg_handle_storage rg_handle_storage;

typedef struct rg_handle_storage_get_result {
    bool exists;
    void *value;
} rg_handle_storage_get_result;

typedef struct rg_handle_storage_it
{
    rg_hash_map_it map_it;
    rg_storage_id  id;
    void          *value;
} rg_handle_storage_it;

// --=== Functions ===--

/**
 * Creates a new handle storage.
 * @return The new handle storage, or NULL if there was an error.
 */
rg_handle_storage *rg_create_handle_storage(void);

/**
 * Destroys a handle storage.
 * @param storage The handle storage to destroy.
 */
void rg_destroy_handle_storage(rg_handle_storage **storage);

/**
 * Adds a new handle to the storage and generates a new id for it.
 * @param storage The storage to add the handle to.
 * @param handle A pointer to the handle to add.
 * @return the id of the new handle, or RG_STORAGE_NULL_ID if there was an error.
 */
rg_storage_id rg_handle_storage_push(rg_handle_storage *storage, void *handle);

/**
 * Gets the handle stored at the given id.
 * @param storage The storage to get the handle from.
 * @param id The id of the handle to get.
 * @return The handle, or NULL if the id is invalid.
 */
rg_handle_storage_get_result rg_handle_storage_get(rg_handle_storage *storage, rg_storage_id id);

/**
 * Erases the handle stored at the given id.
 * @param storage The storage to erase the handle from.
 * @param id The id of the handle to erase.
 */
void rg_handle_storage_erase(rg_handle_storage *storage, rg_storage_id id);

/**
 * Gets an iterator to the first handle in the storage.
 * @param storage The storage to get the iterator from.
 * @return The iterator.
 */
rg_handle_storage_it rg_handle_storage_iterator(rg_handle_storage *storage);

/**
 * Updates the iterator to point to the next element of the storage.
 * @param it The iterator to update.
 * @return true if the iterator was updated, false if there was no next element.
 */
bool rg_handle_storage_next(rg_handle_storage_it *it);

/**
 * Gets the number of handles in the storage.
 * @param storage The storage to get the number of handles from.
 * @return The number of handles.
 */
size_t rg_handle_storage_count(rg_handle_storage *storage);

// endregion