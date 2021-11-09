#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// --=== Hash map ===--

// region Hash Map

#define RG_HASH_MAP_NULL_KEY 0

/**
 * @brief A hash map is a data struct that allows to store pointers (void*) or size_t numbers.
 */
typedef struct rg_hash_map rg_hash_map;

typedef uint64_t rg_hash_map_key_t;

typedef union
{
    void  *as_ptr;
    size_t as_num;
} rg_hash_map_value_t;

typedef struct rg_hash_map_it
{
    rg_hash_map_key_t   key;
    rg_hash_map_value_t value;
    rg_hash_map        *hash_map;
    size_t              next_index;
} rg_hash_map_it;

typedef struct rg_hash_map_get_result
{
    bool                exists;
    rg_hash_map_value_t value;
} rg_hash_map_get_result;

// Functions
rg_hash_map           *rg_create_hash_map(void);
void                   rg_destroy_hash_map(rg_hash_map **p_hash_map);
rg_hash_map_get_result rg_hash_map_get(rg_hash_map *hash_map, rg_hash_map_key_t key);
bool                   rg_hash_map_set(rg_hash_map *hash_map, rg_hash_map_key_t key, rg_hash_map_value_t value);
size_t                 rg_hash_map_count(rg_hash_map *hash_map);
void                   rg_hash_map_erase(rg_hash_map *hash_map, rg_hash_map_key_t key);
rg_hash_map_it         rg_hash_map_iterator(rg_hash_map *hash_map);
bool                   rg_hash_map_next(rg_hash_map_it *it);
void                   rg_hash_map_clear(rg_hash_map *hash_map);

#ifdef UNIT_TESTS
uint64_t rg_hash_map_hash(uint64_t key);
#endif

// endregion

// region Struct Map

// --=== Struct map ===--

/**
 * A struct map is an hash map that takes structs (or any data) as value, instead of just pointers.
 * The difference is that the struct map will store and handle the values of the structs themselves, and thus do not require the user
 * to ensure that the pointed value is valid throughout the lifetime of the map. Also, it stores all the values in a compact area in
 * memory to avoid each element to be scattered around the memory.
 */
typedef struct rg_struct_map rg_struct_map;

typedef struct rg_struct_map_it
{
    rg_hash_map_key_t key;
    void             *value;
    size_t            next_index;
    rg_struct_map    *struct_map;
} rg_struct_map_it;

/**
 * @brief Creates a new struct map.
 * @param value_size The size of the structs that will be stored in the map.
 * @return the created map, or NULL if an error occurred.
 */
rg_struct_map *rg_create_struct_map(size_t value_size);
void           rg_destroy_struct_map(rg_struct_map **p_struct_map);
/**
 * Gets a value in the map.
 * @param p_struct_map The map to get the value from.
 * @param key the key of the value to get.
 * @return the value, or NULL if the key does not exist.
 */
void *rg_struct_map_get(rg_struct_map *p_struct_map, rg_hash_map_key_t key);
/**
 * @brief Sets the value of a p_key.
 * @param p_struct_map is the struct map to be accessed
 * @param key is the p_key of the data in the map.
 * @param p_data is a pointer to the data. It must be at least value_size bytes long. The data will be copied inside the map.
 * @returns the address of the data in the storage if the affectation was successful, NULL otherwise. This pointer will thus stay valid
 * while the element is still stored in the map and the map is still valid, even if the pointer passed to the p_data parameter is not
 * valid anymore.
 */
void            *rg_struct_map_set(rg_struct_map *p_struct_map, rg_hash_map_key_t key, void *p_data);
size_t           rg_struct_map_count(rg_struct_map *struct_map);
void             rg_struct_map_erase(rg_struct_map *struct_map, rg_hash_map_key_t key);
rg_struct_map_it rg_struct_map_iterator(rg_struct_map *struct_map);
bool             rg_struct_map_next(rg_struct_map_it *it);
bool             rg_struct_map_exists(rg_struct_map *struct_map, rg_hash_map_key_t key);

// endregion