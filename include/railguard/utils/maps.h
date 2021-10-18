#pragma once

#include <stdbool.h>
#include <stddef.h>

// --=== Hash map ===--

/**
 * @brief A hash map is a data struct that allows to store pointers (void*) or size_t numbers.
 */
typedef struct rg_hash_map rg_hash_map;

typedef union
{
    void  *as_ptr;
    size_t as_num;
} rg_hash_map_value_t;

typedef struct rg_hash_map_it
{
    const char         *key;
    rg_hash_map_value_t value;
    rg_hash_map        *hash_map;
    size_t              current_index;
} rg_hash_map_it;

typedef struct rg_hash_map_get_result
{
    bool                exists;
    rg_hash_map_value_t value;
} rg_hash_map_get_result;

// Functions
rg_hash_map           *rg_create_hash_map(void);
void                   rg_destroy_hash_map(rg_hash_map **p_hash_map);
rg_hash_map_get_result rg_hash_map_get(rg_hash_map *hash_map, const char *key);
bool                   rg_hash_map_set(rg_hash_map *hash_map, const char *key, rg_hash_map_value_t value);
size_t                 rg_hash_map_count(rg_hash_map *hash_map);
void                   rg_hash_map_erase(rg_hash_map *hash_map, const char *key);
rg_hash_map_it         rg_hash_map_iterator(rg_hash_map *hash_map);
bool                   rg_hash_map_next(rg_hash_map_it *it);

// --=== Struct map ===--

/**
 * A struct map is an hash map that takes structs (or any data) as value, instead of just pointers.
 * The difference is that the struct map will store and handle the values of the structs themselves, and thus do not require the user
 * to ensure that the pointed value is valid throughout the lifetime of the map. Also, it stores all the values in a compact area in
 * memory to avoid each element to be scattered around the memory.
 */
typedef struct rg_struct_map rg_struct_map;

typedef struct rg_struct_map_get_result
{
    bool  exists;
    void *value;
} rg_struct_map_get_result;

typedef struct rg_struct_map_it
{
    const char    *key;
    void          *value;
    size_t         next_index;
    rg_struct_map *struct_map;
} rg_struct_map_it;

rg_struct_map *rg_create_struct_map(size_t value_size);
void           rg_destroy_struct_map(rg_struct_map **p_struct_map);
/**
 * Gets a value in the map.
 * @param p_struct_map
 * @param p_key
 * @return a struct containing a exists boolean which is true if the value exists or not in the map. If exists is true, value contains
 * a pointer to the value. This pointer points into the map's internal storage and is value_size bytes long.
 */
void *rg_struct_map_get(rg_struct_map *p_struct_map, const char *p_key);
/**
 * @brief Sets the value of a p_key.
 * @param p_struct_map is the struct map to be accessed
 * @param p_key is the p_key of the data in the map.
 * @param p_data is a pointer to the data. It must be at least value_size bytes long. The data will be copied inside the map.
 * @returns the address of the data in the storage if the affectation was successful, NULL otherwise. This pointer will thus stay valid
 * while the element is still stored in the map and the map is still valid, even if the pointer passed to the p_data parameter is not
 * valid anymore.
 */
void            *rg_struct_map_set(rg_struct_map *p_struct_map, const char *p_key, void *p_data);
size_t           rg_struct_map_count(rg_struct_map *struct_map);
void             rg_struct_map_erase(rg_struct_map *struct_map, const char *key);
rg_struct_map_it rg_struct_map_iterator(rg_struct_map *struct_map);
bool             rg_struct_map_next(rg_struct_map_it *it);
bool             rg_struct_map_exists(rg_struct_map *struct_map, const char *p_key);
