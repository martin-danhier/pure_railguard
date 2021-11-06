#include "railguard/utils/maps.h"

#include <railguard/utils/arrays.h>
#include <railguard/utils/memory.h>

#include <stdint.h>
#include <string.h>

// --=== Hash Maps ===--

// region Hash Map

// --=== Constants ===--

#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME  1099511628211ULL

// --=== Types ===--

typedef struct rg_hash_map_entry
{
    rg_hash_map_key_t   key;
    rg_hash_map_value_t value;
} rg_hash_map_entry;

typedef struct rg_hash_map
{
    rg_hash_map_entry *data;
    size_t             capacity;
    size_t             count;
} rg_hash_map;

// --=== Utils functions ===--

// FNV hash of key
// Related wiki page: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
uint64_t rg_hash_map_hash(uint64_t key)
{
    uint64_t hash = FNV_OFFSET;
    for (size_t i = 0; i < sizeof(key); i++)
    {
        hash ^= ((uint8_t *) &key)[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

bool rg_hash_map_set_entry(rg_hash_map_entry  *entries,
                           size_t              capacity,
                           rg_hash_map_key_t   key,
                           rg_hash_map_value_t value,
                           size_t             *count)
{
    // Prevent the use of the zero key, which is reserved for the empty entry
    if (key == RG_HASH_MAP_NULL_KEY)
    {
        return false;
    }

    // Compute the index of the key in the array
    size_t index = (size_t) (rg_hash_map_hash(key) & ((uint64_t) capacity - 1));

    while (entries[index].key != RG_HASH_MAP_NULL_KEY)
    {
        // If the slot is not empty, but the key is the same
        // We want to edit that slot
        if (entries[index].key == key)
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

    if (count != NULL)
    {
        (*count)++;
    }

    entries[index].key   = key;
    entries[index].value = value;
    return true;
}

bool rg_hash_map_expand(rg_hash_map *hash_map)
{
    // Always use powers of 2, so we can replace modulo with and operation
    size_t new_capacity = hash_map->capacity * 2;
    if (new_capacity < hash_map->capacity)
    {
        return false;
    }

    rg_hash_map_entry *new_entries = rg_calloc(new_capacity, sizeof(rg_hash_map_entry));
    if (new_entries == NULL)
    {
        return false;
    }

    // Move the entries to the new array
    for (size_t i = 0; i < hash_map->capacity; i++)
    {
        rg_hash_map_entry entry = hash_map->data[i];
        if (entry.key != RG_HASH_MAP_NULL_KEY)
        {
            rg_hash_map_set_entry(new_entries, new_capacity, entry.key, entry.value, NULL);
        }
    }

    rg_free(hash_map->data);
    hash_map->data     = new_entries;
    hash_map->capacity = new_capacity;
    return true;
}

bool rg_init_hash_map(rg_hash_map *hash_map)
{
    // Allocate array
    hash_map->capacity = 1;
    hash_map->count    = 0;
    hash_map->data     = rg_calloc(hash_map->capacity, sizeof(rg_hash_map_entry));
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
    rg_hash_map *map = rg_malloc(sizeof(rg_hash_map));
    if (map == NULL)
    {
        return NULL;
    }

    if (!rg_init_hash_map(map))
    {
        rg_free(map);
        return NULL;
    }

    return map;
}

void rg_destroy_hash_map(rg_hash_map **p_hash_map)
{
    // Free the data
    rg_free((*p_hash_map)->data);

    // Free the map
    rg_free(*p_hash_map);
    *p_hash_map = NULL;
}

rg_hash_map_get_result rg_hash_map_get(rg_hash_map *hash_map, rg_hash_map_key_t key)
{
    // Directly filter invalid keys
    if (key == RG_HASH_MAP_NULL_KEY)
    {
        return (rg_hash_map_get_result) {.exists = false};
    }

    // Compute the index of the key in the array
    size_t index = (size_t) (rg_hash_map_hash(key) & ((uint64_t) hash_map->capacity - 1));

    // Search for the value in the location, until we find an empty slot
    while (hash_map->data[index].key != RG_HASH_MAP_NULL_KEY)
    {
        // If the key is the same, we found the good slot !
        if (key == hash_map->data[index].key)
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

bool rg_hash_map_set(rg_hash_map *hash_map, rg_hash_map_key_t key, rg_hash_map_value_t value)
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
        .hash_map   = hash_map,
        .next_index = 0,
    };
}
bool rg_hash_map_next(rg_hash_map_it *it)
{
    rg_hash_map *map = it->hash_map;
    while (it->next_index < map->capacity)
    {
        size_t i = it->next_index;

        // Increment index
        it->next_index++;

        // If the slot is not empty, use it
        if (map->data[i].key != RG_HASH_MAP_NULL_KEY)
        {
            it->key   = map->data[i].key;
            it->value = map->data[i].value;
            return true;
        }
    }

    // When the loop is finished, there are no more elements in the map
    it->key   = RG_HASH_MAP_NULL_KEY;
    it->value = (rg_hash_map_value_t) {NULL};
    return false;
}

void rg_hash_map_erase(rg_hash_map *hash_map, rg_hash_map_key_t key)
{
    // Compute the index of the key in the array
    size_t index = (size_t) (rg_hash_map_hash(key) & ((uint64_t) hash_map->capacity - 1));

    rg_hash_map_entry *deleted_slot           = NULL;
    size_t             deleted_index          = 0;
    size_t             invalidated_block_size = 0;

    while (hash_map->data[index].key != RG_HASH_MAP_NULL_KEY)
    {
        // We found the slot to remove if the key is the same
        if (key == hash_map->data[index].key)
        {
            deleted_slot  = &hash_map->data[index];
            deleted_index = index;
        }

        // If the deleted slot is not NULL, then we are between it and the NULL key
        // The current slot may then become inaccessible. We need to invalidate it.
        // For that, we need to count the number of slots between the deleted slot and the NULL key
        if (deleted_slot != NULL)
        {
            invalidated_block_size++;
        }

        // Else, increment to find the next empty slot
        index++;
        // Wrap around to stay inside the array
        if (index >= hash_map->capacity)
        {
            index = 0;
        }
    }

    // If we found a slot to remove, remove it and add the next ones again
    if (deleted_slot != NULL)
    {
        // If we have other slots to invalidate, store them in another array first and set them to NULL
        rg_hash_map_entry *invalidated_slots = NULL;
        if (invalidated_block_size > 1)
        {
            invalidated_slots = rg_malloc(sizeof(rg_hash_map_entry) * (invalidated_block_size - 1));

            // Copy the invalidated slots. Wrap around if needed
            size_t j = (deleted_index + 1) % hash_map->capacity;
            for (size_t i = 0; i < invalidated_block_size - 1; i++)
            {
                // Copy the slot
                invalidated_slots[i] = hash_map->data[j];
                // Set it to NULL
                hash_map->data[j] = (rg_hash_map_entry) {
                    .key   = RG_HASH_MAP_NULL_KEY,
                    .value = (rg_hash_map_value_t) {NULL},
                };

                // Increment the index
                j++;
                // Wrap around to stay inside the array
                if (j >= hash_map->capacity)
                {
                    j = 0;
                }
            }
        }

        // Set the deleted slot to NULL
        *deleted_slot = (rg_hash_map_entry) {
            .key   = RG_HASH_MAP_NULL_KEY,
            .value = (rg_hash_map_value_t) {NULL},
        };

        // Decrement the count. We remove the whole size of the block because the set function will increment it again
        hash_map->count--;

        // If we have slots to invalidate, add them back
        if (invalidated_slots != NULL)
        {
            for (size_t i = 0; i < invalidated_block_size - 1; i++)
            {
                rg_hash_map_set_entry(hash_map->data, hash_map->capacity, invalidated_slots[i].key, invalidated_slots[i].value, NULL);
            }
            rg_free(invalidated_slots);
        }
    }
}
// endregion

// --=== Struct Maps ===--

// region Struct Map

// --=== Types ===--

typedef struct rg_struct_map
{
    rg_hash_map hash_map; // We take advantage of the fact that we are in the same c file to avoid a pointer here
    rg_vector   storage;
    size_t      value_size;
} rg_struct_map;

// --=== Utils functions ===--

rg_hash_map_key_t rg_struct_map_get_key_of_storage_element(rg_struct_map *struct_map, void *p_storage_element)
{
    return *(rg_hash_map_key_t *) (((char *) p_storage_element) + struct_map->value_size);
}

// --=== Functions ===--

rg_struct_map *rg_create_struct_map(size_t value_size)
{
    rg_struct_map *map = rg_malloc(sizeof(rg_struct_map));
    if (map == NULL)
    {
        return NULL;
    }

    // Init the hash map
    if (!rg_init_hash_map(&map->hash_map))
    {
        rg_free(map);
        return NULL;
    }

    // Init the vector
    // The element size is value_size + size of key
    // That way, the storage will consist of an array alternating values and keys
    // Storing the key in the storage allows to create an iterator that does not even look at the hash map
    if (!rg_create_vector(2, value_size + sizeof(rg_hash_map_key_t), &map->storage))
    {
        rg_hash_map *hash_map = &map->hash_map;
        rg_destroy_hash_map(&hash_map);
        rg_free(map);
        return NULL;
    }

    map->value_size = value_size;

    return map;
}

void rg_destroy_struct_map(rg_struct_map **p_struct_map)
{
    // Destroy the vector
    rg_destroy_vector(&(*p_struct_map)->storage);

    // Destroy the hash map
    rg_free((*p_struct_map)->hash_map.data);

    // Destroy the struct map
    rg_free(*p_struct_map);
    *p_struct_map = NULL;
}

size_t rg_struct_map_count(rg_struct_map *struct_map)
{
    // The keys are still managed by the hash map
    // So the count is fetched from it
    return struct_map->hash_map.count;
}

void *rg_struct_map_get(rg_struct_map *p_struct_map, rg_hash_map_key_t key)
{
    // Check if there is already a value there in the map
    rg_hash_map_get_result get_result = rg_hash_map_get(&p_struct_map->hash_map, key);

    if (get_result.exists)
    {
        // There is a value, and the hash map returned its index in the storage
        return rg_vector_get_element(&p_struct_map->storage, get_result.value.as_num);
    }
    return NULL;
}

bool rg_struct_map_exists(rg_struct_map *struct_map, rg_hash_map_key_t key)
{
    // Check if there is already a value there in the map
    // No need to get the element from the storage
    rg_hash_map_get_result get_result = rg_hash_map_get(&struct_map->hash_map, key);
    return get_result.exists;
}

void *rg_struct_map_set(rg_struct_map *p_struct_map, rg_hash_map_key_t key, void *p_data)
{
    // Check if there is already a value there in the map
    rg_hash_map_get_result get_result = rg_hash_map_get(&p_struct_map->hash_map, key);

    void *p_data_in_storage = NULL;

    if (get_result.exists)
    {
        // There is a value: there is a place in the storage we can modify.
        p_data_in_storage = rg_vector_get_element(&p_struct_map->storage, get_result.value.as_num);
        if (p_data_in_storage != NULL)
        {
            return memcpy(p_data_in_storage, p_data, p_struct_map->value_size);
        }
        // We don't need to update the map nor the key since it points to a valid storage element.
    }
    else
    {
        // There is no value: we need to push the data in a new storage index.
        p_data_in_storage = rg_vector_push_back_no_data(&p_struct_map->storage);

        // If it worked, in this case, we also need to update the map to point to our new storage slot.
        if (p_data_in_storage != NULL)
        {
            bool success = true;
            // Store value
            success = memcpy(p_data_in_storage, p_data, p_struct_map->value_size) != NULL;
            if (success)
            {
                // Store key
                *(rg_hash_map_key_t *) (((char *) p_data_in_storage) + p_struct_map->value_size) = key;

                // Update hash map
                success = rg_hash_map_set(&p_struct_map->hash_map,
                                          key,
                                          (rg_hash_map_value_t) {
                                              .as_num = rg_vector_last_index(&p_struct_map->storage),
                                          });
            }

            // If there was an error, we shouldn't keep the value in the storage.
            if (!success)
            {
                rg_vector_pop_back(&p_struct_map->storage);
                p_data_in_storage = NULL;
            }
        }
    }

    return p_data_in_storage;
}

void rg_struct_map_erase(rg_struct_map *struct_map, rg_hash_map_key_t key)
{
    // Look up the element
    rg_hash_map_get_result get_result = rg_hash_map_get(&struct_map->hash_map, key);
    if (get_result.exists)
    {
        // We need to remove the element from the storage if it exists
        size_t deleted_slot_index = get_result.value.as_num;
        size_t last_slot_index    = rg_vector_last_index(&struct_map->storage);

        bool success = true;

        // If it is not the last slot, move the last slot to the deleted slot, that way data stays packed together
        if (deleted_slot_index < last_slot_index)
        {
            // Copy the last element to the deleted slot
            success = rg_vector_copy(&struct_map->storage, last_slot_index, deleted_slot_index);
            if (success)
            {
                // In case of success, we need to update the map to move the element that was last
                // For that, we can use the key that is stored with the data
                void *p_updated_slot = rg_vector_get_element(&struct_map->storage, deleted_slot_index);
                if (p_updated_slot != NULL)
                {
                    rg_hash_map_key_t last_elements_key = rg_struct_map_get_key_of_storage_element(struct_map, p_updated_slot);

                    success = rg_hash_map_set_entry(struct_map->hash_map.data,
                                                    struct_map->hash_map.capacity,
                                                    last_elements_key,
                                                    (rg_hash_map_value_t) {
                                                        .as_num = deleted_slot_index,
                                                    },
                                                    &struct_map->hash_map.count);
                }
            }
        }

        // Everything worked: we can pop the storage and remove the deleted key from the map
        if (success)
        {
            rg_vector_pop_back(&struct_map->storage);
            rg_hash_map_erase(&struct_map->hash_map, key);
        }
    }
}

rg_struct_map_it rg_struct_map_iterator(rg_struct_map *struct_map)
{
    return (rg_struct_map_it) {
        .struct_map = struct_map,
        .next_index = 0,
    };
}

bool rg_struct_map_next(rg_struct_map_it *it)
{
    rg_vector *storage = &it->struct_map->storage;
    while (it->next_index < storage->count)
    {
        size_t i = it->next_index;

        // Increment index
        it->next_index++;

        it->value = ((char *) storage->data) + i * storage->element_size;
        it->key   = rg_struct_map_get_key_of_storage_element(it->struct_map, it->value);

        return true;
    }

    it->value = NULL;
    it->key   = RG_HASH_MAP_NULL_KEY;
    return false;
}

// endregion