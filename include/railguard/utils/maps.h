#pragma once

#include <stdbool.h>
#include <stddef.h>

// --=== Types ===--

typedef struct rg_hash_map rg_hash_map;
typedef struct rg_hash_map_it
{
    const char  *key;
    void        *value;
    rg_hash_map *hash_map;
    size_t       current_index;
} rg_hash_map_it;

// --=== Hash map ===--

rg_hash_map   *rg_create_hash_map(void);
void           rg_destroy_hash_map(rg_hash_map **p_hash_map);
void          *rg_hash_map_get(rg_hash_map *hash_map, const char *key);
bool           rg_hash_map_set(rg_hash_map *hash_map, const char *key, void *value);
size_t         rg_hash_map_count(rg_hash_map *hash_map);
void           rg_hash_map_erase(rg_hash_map *hash_map, const char *key);
rg_hash_map_it rg_hash_map_iterator(rg_hash_map *hash_map);
bool           rg_hash_map_next(rg_hash_map_it *it);
