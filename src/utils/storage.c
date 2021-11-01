#include "railguard/utils/storage.h"

#include <railguard/utils/maps.h>

#include <stdlib.h>

// --=== Type Definitions ===--

typedef struct rg_storage {
    // Used to generate unique IDs for each storage entry.
    rg_storage_id id_counter;
    rg_struct_map *map;
} rg_storage;

// --=== Functions ===--

rg_storage *rg_create_storage(size_t element_size)
{
    // Allocate the storage structure.
    rg_storage *storage = calloc(1, sizeof(rg_storage));
    if (storage == NULL) {
        return NULL;
    }

    // Initialize the storage's map.
    storage->map = rg_create_struct_map(element_size);
    if (storage->map == NULL) {
        free(storage);
        return NULL;
    }

    // Initialize the storage's ID counter.
    // Start at one, since zero is reserved for RG_STORAGE_NULL_ID.
    storage->id_counter = 1;

    return storage;
}

void rg_destroy_storage(rg_storage **storage)
{
    if (storage == NULL || *storage == NULL) {
        return;
    }

    // Destroy the storage's map.
    rg_destroy_struct_map(&(*storage)->map);

    // Free the storage structure.
    free(*storage);
    *storage = NULL;
}

rg_storage_id rg_storage_push(rg_storage *storage, void *data)
{
    if (storage == NULL || data == NULL) {
        return RG_STORAGE_NULL_ID;
    }

    // Generate a new ID for the storage entry.
    rg_storage_id id = storage->id_counter++;

    // Add the storage entry to the map.


    return id;
}
