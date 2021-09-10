#include "railguard/utils/maps.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// --=== Constants ===--

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME  1099511628211UL

// --=== Types ===--

typedef struct rg_hash_map_entry
{
    const char *key;
    void       *value;
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

bool set_entry(rg_hash_map_entry* entries, size_t capacity, const char *key, void *value, size_t *count) {

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
            set_entry(new_entries, new_capacity, entry.key, entry.value, NULL);
        }
    }

    free(hash_map->data);
    hash_map->data = new_entries;
    hash_map->capacity = new_capacity;
    return true;
}



// --=== Hash map ===--

// Inspired from https://benhoyt.com/writings/hash-table-in-c/

rg_hash_map *rg_create_hash_map(void)
{
    rg_hash_map *map = malloc(sizeof(rg_hash_map));

    // Allocate array
    map->capacity = 1;
    map->count    = 0;
    map->data     = calloc(map->capacity, sizeof(rg_hash_map_entry));

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

void *rg_hash_map_get(rg_hash_map *hash_map, const char *key)
{
    // Compute the index of the key in the array
    size_t index = (size_t) (hash_key(key) & ((uint64_t) hash_map->capacity - 1));

    // Search for the value in the location, until we find an empty slot
    while (hash_map->data[index].key != NULL)
    {
        // If the key is the same, we found the good slot !
        if (strcmp(key, hash_map->data[index].key) == 0)
        {
            return hash_map->data[index].value;
        }

        index++;
        // Wrap around to stay inside the array
        if (index >= hash_map->capacity)
        {
            index = 0;
        }
    }
    return NULL;
}

bool rg_hash_map_set(rg_hash_map *hash_map, const char *key, void *value)
{
    // The value should not be NULL
    if (value == NULL)
    {
        return false;
    }

    // Expand the capacity of the array if it is more than half full
    if (hash_map->count >= hash_map->capacity / 2) {
        if (!rg_hash_map_expand(hash_map)) {
            return false;
        }
    }

    return set_entry(hash_map->data, hash_map->capacity, key, value, &hash_map->count);
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
