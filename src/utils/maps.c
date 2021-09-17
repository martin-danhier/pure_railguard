#include "railguard/utils/maps.h"

#include <railguard/utils/arrays.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// --=== Hash Maps ===--

// region Hash Map

// --=== Constants ===--

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME  1099511628211UL

// --=== Types ===--

typedef struct rg_hash_map_entry
{
    const char         *key;
    rg_hash_map_value_t value;
} rg_hash_map_entry;

typedef struct rg_hash_map
{
    rg_hash_map_entry *data;
    size_t             capacity;
    size_t             count;
} rg_hash_map;

// --=== Utils functions ===--

// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
uint64_t hash_key(const char *key)
{
    uint64_t hash = FNV_OFFSET;
    for (const char *p = key; *p; p++)
    {
        hash ^= (uint64_t) (unsigned char) (*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

bool rg_hash_map_set_entry(rg_hash_map_entry *entries, size_t capacity, const char *key, rg_hash_map_value_t value, size_t *count)
{
    // Compute the index of the key in the array
    size_t index = (size_t) (hash_key(key) & ((uint64_t) capacity - 1));

    while (entries[index].key != NULL)
    {
        // If the slot is not empty, but the key is the same
        // We want to edit that slot
        if (strcmp(key, entries[index].key) == 0)
        {
            // Edit value
            entries[index].value = value;
            return true;
        }

        // Else, increment to find the next empty slot
        index++;
        // Wrap around to stay inside the array
        if (index >= capacity)
        {
            index = 0;
        }
    }

    // The key didn't already exist
    // We need to add it to an empty slot
    // Index is guarantied to point to an empty slot now, given the loop above.
    // We will use that one.

    if (count != NULL) {
        key = strdup(key);
        if (key == NULL) {
            return false;
        }
        (*count)++;
    }

    entries[index].key = (char *) key;
    entries[index].value = value;
    return true;
}

bool rg_hash_map_expand(rg_hash_map *hash_map)
{
    // Always use powers of 2, so we can replace modulo with and operation
    size_t new_capacity = hash_map->capacity * 2;
    if (new_capacity < hash_map->capacity) {
        return false;
    }

    rg_hash_map_entry *new_entries = calloc(new_capacity, sizeof(rg_hash_map_entry));
    if (new_entries == NULL) {
        return false;
    }

    // Move the entries to the new array
    for (size_t i = 0; i < hash_map->capacity; i++) {
        rg_hash_map_entry entry = hash_map->data[i];
        if (entry.key != NULL) {
            rg_hash_map_set_entry(new_entries, new_capacity, entry.key, entry.value, NULL);
        }
    }

    free(hash_map->data);
    hash_map->data     = new_entries;
    hash_map->capacity = new_capacity;
    return true;
}

bool rg_init_hash_map(rg_hash_map *hash_map)
{
    // Allocate array
    hash_map->capacity = 1;
    hash_map->count    = 0;
    hash_map->data     = calloc(hash_map->capacity, sizeof(rg_hash_map_entry));
    if (hash_map->data == NULL)
    {
        return false;
    }
    return true;
}

// --=== Hash map ===--

// Inspired from https://benhoyt.com/writings/hash-table-in-c/

rg_hash_map *rg_create_hash_map(void)
{
    rg_hash_map *map = malloc(sizeof(rg_hash_map));
    if (map == NULL)
    {
        return NULL;
    }

    if (!rg_init_hash_map(map))
    {
        free(map);
        return NULL;
    }

    return map;
}

void rg_destroy_hash_map(rg_hash_map **p_hash_map)
{
    // Free the data
    free((*p_hash_map)->data);

    // Free the map
    free(*p_hash_map);
    *p_hash_map = NULL;
}

rg_hash_map_get_result rg_hash_map_get(rg_hash_map *hash_map, const char *key)
{
    // Compute the index of the key in the array
    size_t index = (size_t) (hash_key(key) & ((uint64_t) hash_map->capacity - 1));

    // Search for the value in the location, until we find an empty slot
    while (hash_map->data[index].key != NULL)
    {
        // If the key is the same, we found the good slot !
        if (strcmp(key, hash_map->data[index].key) == 0)
        {
            return (rg_hash_map_get_result) {
                .value  = hash_map->data[index].value,
                .exists = true,
            };
        }

        index++;
        // Wrap around to stay inside the array
        if (index >= hash_map->capacity)
        {
            index = 0;
        }
    }
    return (rg_hash_map_get_result) {
        .exists = false,
    };
}

bool rg_hash_map_set(rg_hash_map *hash_map, const char *key, rg_hash_map_value_t value)
{
    // Expand the capacity of the array if it is more than half full
    if (hash_map->count >= hash_map->capacity / 2)
    {
        if (!rg_hash_map_expand(hash_map))
        {
            return false;
        }
    }

    return rg_hash_map_set_entry(hash_map->data, hash_map->capacity, key, value, &hash_map->count);
}

size_t rg_hash_map_count(rg_hash_map *hash_map)
{
    return hash_map->count;
}

rg_hash_map_it rg_hash_map_iterator(rg_hash_map *hash_map)
{
    // Return iterator at the beginning
    return (rg_hash_map_it) {
        .hash_map = hash_map,
        .current_index = 0,
    };
}
bool rg_hash_map_next(rg_hash_map_it *it)
{
    rg_hash_map *map = it->hash_map;
    while (it->current_index < map->capacity) {
        size_t i = it->current_index;

        // Increment index
        it->current_index++;

        // If the slot is not empty, use it
        if (map->data[i].key != NULL) {
            it->key = map->data[i].key;
            it->value = map->data[i].value;
            return true;
        }
    }

    // When the loop is finished, there are no more elements in the map
    return false;
}

void rg_hash_map_erase(rg_hash_map *hash_map, const char *key)
{
    // Compute the index of the key in the array
    size_t index = (size_t) (hash_key(key) & ((uint64_t) hash_map->capacity - 1));

    // Search for the slot near the pointed index, until an empty slot is found.
    while (hash_map->data[index].key != NULL)
    {
        // We found the slot to remove if the key is the same
        if (strcmp(key, hash_map->data[index].key) == 0)
        {
            // Set key and value to null
            hash_map->data[index].value.as_ptr = NULL;
            hash_map->data[index].key          = NULL;
            hash_map->count -= 1;
            return;
        }

        // Else, increment to find the next empty slot
        index++;
        // Wrap around to stay inside the array
        if (index >= hash_map->capacity)
        {
            index = 0;
        }
    }

    // If we reach this point, then the key wasn't found
    // Technically, the contract is filled since we needed to remove it
    // Thus, we always succeed, and we don't need to return an error.
}
// endregion

// --=== Struct Maps ===--

// region Struct Map

// --=== Types ===--

typedef struct rg_struct_map
{
    rg_hash_map hash_map; // We take advantage of the fact that we are in the same c file to avoid a pointer here
    rg_vector   storage;

} rg_struct_map;

// --=== Functions ===--

rg_struct_map *rg_create_struct_map(size_t value_size)
{
    rg_struct_map *map = malloc(sizeof(rg_struct_map));
    if (map == NULL)
    {
        return NULL;
    }

    // Init the hash map
    if (!rg_init_hash_map(&map->hash_map))
    {
        free(map);
        return NULL;
    }

    // Init the vector
    if (!rg_create_vector(2, value_size, &map->storage))
    {
        rg_hash_map *hash_map = &map->hash_map;
        rg_destroy_hash_map(&hash_map);
        free(map);
        return NULL;
    }

    return map;
}

void rg_destroy_struct_map(rg_struct_map **p_struct_map)
{
    // Destroy the vector
    rg_destroy_vector(&(*p_struct_map)->storage);

    // Destroy the hash map
    rg_hash_map *hash_map = &(*p_struct_map)->hash_map;
    rg_destroy_hash_map(&hash_map);

    // Destroy the struct map
    free(*p_struct_map);
    *p_struct_map = NULL;
}

size_t rg_struct_map_count(rg_struct_map *struct_map)
{
    // The keys are still managed by the hash map
    // So the count is fetched from it
    return struct_map->hash_map.count;
}

rg_hash_map_get_result rg_struct_map_get(rg_struct_map *struct_map, const char *key) {

}

// endregion