#include "railguard/utils/storage.h"

#include <railguard/utils/memory.h>

// region Storage

// --=== Type Definitions ===--

typedef struct rg_storage
{
    // Used to generate unique IDs for each storage entry.
    rg_storage_id  id_counter;
    rg_struct_map *map;
} rg_storage;

// --=== Functions ===--

rg_storage *rg_create_storage(size_t element_size)
{
    // Allocate the storage structure.
    rg_storage *storage = rg_calloc(1, sizeof(rg_storage));
    if (storage == NULL)
    {
        return NULL;
    }

    // Initialize the storage's map.
    storage->map = rg_create_struct_map(element_size);
    if (storage->map == NULL)
    {
        rg_free(storage);
        return NULL;
    }

    // Initialize the storage's ID counter.
    // Start at one, since zero is reserved for RG_STORAGE_NULL_ID.
    storage->id_counter = 1;

    return storage;
}

void rg_destroy_storage(rg_storage **storage)
{
    if (storage == NULL || *storage == NULL)
    {
        return;
    }

    // Destroy the storage's map.
    rg_destroy_struct_map(&(*storage)->map);

    // Free the storage structure.
    rg_free(*storage);
    *storage = NULL;
}

rg_storage_id rg_storage_push(rg_storage *storage, void *data)
{
    if (storage == NULL || data == NULL)
    {
        return RG_STORAGE_NULL_ID;
    }

    // Generate a new ID for the storage entry.
    rg_storage_id id = storage->id_counter++;

    // Add the storage entry to the map.
    // This will copy the data into the map's internal buffer.
    if (rg_struct_map_set(storage->map, id, data) == NULL)
    {
        storage->id_counter--;
        return RG_STORAGE_NULL_ID;
    }

    return id;
}

void *rg_storage_get(rg_storage *storage, rg_storage_id id)
{
    if (storage == NULL)
    {
        return NULL;
    }

    // Get the storage entry from the map.
    return rg_struct_map_get(storage->map, id);
}

void rg_storage_erase(rg_storage *storage, rg_storage_id id)
{
    if (storage == NULL)
    {
        return;
    }

    // Remove the storage entry from the map.
    rg_struct_map_erase(storage->map, id);
}

rg_storage_it rg_storage_iterator(rg_storage *storage)
{
    if (storage == NULL)
    {
        return (rg_storage_it) {0};
    }

    return (rg_storage_it) {
        .map_it = rg_struct_map_iterator(storage->map),
    };
}

bool rg_storage_next(rg_storage_it *it)
{
    if (it == NULL)
    {
        return false;
    }

    // Get the next storage entry from the map.
    bool result = rg_struct_map_next(&it->map_it);

    if (result)
    {
        it->id    = it->map_it.key;
        it->value = it->map_it.value;
    }
    else
    {
        it->id    = RG_STORAGE_NULL_ID;
        it->value = NULL;
    }

    return result;
}

size_t rg_storage_count(rg_storage *storage)
{
    if (storage == NULL)
    {
        return 0;
    }

    return rg_struct_map_count(storage->map);
}

bool rg_storage_exists(rg_storage *storage, rg_storage_id id)
{
    if (storage == NULL)
    {
        return false;
    }

    return rg_struct_map_exists(storage->map, id);
}

// endregion

// region Handle storage

// --=== Type Definitions ===--

typedef struct rg_handle_storage
{
    rg_storage_id id_counter;
    rg_hash_map  *map;
} rg_handle_storage;

// --=== Functions ===--

rg_handle_storage *rg_create_handle_storage(void)
{
    // Allocate the storage structure.
    rg_handle_storage *storage = rg_calloc(1, sizeof(rg_handle_storage));
    if (storage == NULL)
    {
        return NULL;
    }

    // Initialize the storage's map.
    storage->map = rg_create_hash_map();
    if (storage->map == NULL)
    {
        rg_free(storage);
        return NULL;
    }

    // Initialize the storage's ID counter.
    // Start at one, since zero is reserved for RG_STORAGE_NULL_ID.
    storage->id_counter = 1;

    return storage;
}

void rg_destroy_handle_storage(rg_handle_storage **storage)
{
    if (storage == NULL || *storage == NULL)
    {
        return;
    }

    // Destroy the storage's map.
    rg_destroy_hash_map(&(*storage)->map);

    // Free the storage structure.
    rg_free(*storage);
    *storage = NULL;
}

rg_storage_id rg_handle_storage_push(rg_handle_storage *storage, void *handle)
{
    if (storage == NULL)
    {
        return RG_STORAGE_NULL_ID;
    }

    // Generate a new ID for the storage entry.
    rg_storage_id id = storage->id_counter++;

    // Add the storage entry to the map.
    // This will copy the handle into the map's internal buffer.
    if (rg_hash_map_set(storage->map, id, (rg_hash_map_value_t) {.as_ptr = handle}) == false)
    {
        storage->id_counter--;
        return RG_STORAGE_NULL_ID;
    }

    return id;
}

rg_handle_storage_get_result rg_handle_storage_get(rg_handle_storage *storage, rg_storage_id id)
{
    if (storage == NULL)
    {
        return (rg_handle_storage_get_result) {
            .exists = false,
        };
    }

    // Get the storage entry from the map.
    rg_hash_map_get_result get_result = rg_hash_map_get(storage->map, id);
    if (!get_result.exists)
    {
        return (rg_handle_storage_get_result) {
            .exists = false,
        };
    }

    return (rg_handle_storage_get_result) {
        .value  = get_result.value.as_ptr,
        .exists = true,
    };
}

void rg_handle_storage_erase(rg_handle_storage *storage, rg_storage_id id)
{
    if (storage == NULL)
    {
        return;
    }

    // Remove the storage entry from the map.
    rg_hash_map_erase(storage->map, id);
}

rg_handle_storage_it rg_handle_storage_iterator(rg_handle_storage *storage)
{
    if (storage == NULL)
    {
        return (rg_handle_storage_it) {0};
    }

    return (rg_handle_storage_it) {
        .map_it = rg_hash_map_iterator(storage->map),
    };
}

bool rg_handle_storage_next(rg_handle_storage_it *it)
{
    if (it == NULL)
    {
        return false;
    }

    // Get the next storage entry from the map.
    bool result = rg_hash_map_next(&it->map_it);

    if (result)
    {
        it->id    = it->map_it.key;
        it->value = it->map_it.value.as_ptr;
    }
    else
    {
        it->id    = RG_STORAGE_NULL_ID;
        it->value = NULL;
    }

    return result;
}

size_t rg_handle_storage_count(rg_handle_storage *storage)
{
    if (storage == NULL)
    {
        return 0;
    }

    return rg_hash_map_count(storage->map);
}

// endregion