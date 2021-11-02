#pragma once

#include "../framework/test_framework.h"
#include "string.h"


typedef struct rg_test_struct_map_data
{
    int    number;
    double pos[3];
} rg_test_struct_map_data;

TEST(StructMap)
{
    // Creation
    rg_struct_map *struct_map = rg_create_struct_map(sizeof(rg_test_struct_map_data));
    EXPECT_NOT_NULL(struct_map);

    EXPECT_TRUE(rg_struct_map_count(struct_map) == 0);

    // Populate it
    rg_test_struct_map_data  data        = {.number = 42, .pos = {7, -9.5, 2}};
    rg_test_struct_map_data *key1_in_map = rg_struct_map_set(struct_map, 1, &data);
    EXPECT_NOT_NULL(key1_in_map);
    EXPECT_TRUE(rg_struct_map_count(struct_map) == 1);

    // Update data and save another
    data.number                          = 89;
    data.pos[0]                          = 78;
    rg_test_struct_map_data *key2_in_map = rg_struct_map_set(struct_map, 2, &data);
    EXPECT_NOT_NULL(key2_in_map);
    EXPECT_TRUE(rg_struct_map_count(struct_map) == 2);
    EXPECT_TRUE(key1_in_map != key2_in_map);

    // Since the data is copied, the value should stay unchanged even if the original struct is modified
    data.pos[1] = 88;
    EXPECT_TRUE(key1_in_map->number == 42);
    EXPECT_TRUE(key1_in_map->pos[0] == 7);
    EXPECT_TRUE(key1_in_map->pos[1] == -9.5);
    EXPECT_TRUE(key1_in_map->pos[2] == 2);
    EXPECT_TRUE(key2_in_map->number == 89);
    EXPECT_TRUE(key2_in_map->pos[0] == 78);
    EXPECT_TRUE(key2_in_map->pos[1] == -9.5);
    EXPECT_TRUE(key2_in_map->pos[2] == 2);

    // Memory should be packed together
    // key1_in_map should be the first, then key2_in_map should follow
    // In between, there is the pointer to 1
    rg_hash_map_key_t key1 = *(rg_hash_map_key_t *) ((void*) key1_in_map + sizeof(rg_test_struct_map_data));
    EXPECT_TRUE(key1 == 1);
    EXPECT_TRUE((rg_test_struct_map_data *) ((void *) key1_in_map + sizeof(rg_test_struct_map_data) + sizeof(const char *))
                == key2_in_map);
    rg_hash_map_key_t key2 = *(rg_hash_map_key_t *) ((void*) key2_in_map + sizeof(rg_test_struct_map_data));
    EXPECT_TRUE(key2 == 2);

    // Erasing
    rg_struct_map_erase(struct_map, 1);
    EXPECT_TRUE(rg_struct_map_count(struct_map) == 1);

    // Now, key1_in_map should point to key2 because it was moved at the beginning of the array
    EXPECT_TRUE(key1_in_map->number == 89);
    EXPECT_TRUE(key1_in_map->pos[0] == 78);
    EXPECT_TRUE(key1_in_map->pos[1] == -9.5);
    EXPECT_TRUE(key1_in_map->pos[2] == 2);
    key1 = *(rg_hash_map_key_t *) ((void*) key1_in_map + sizeof(rg_test_struct_map_data));
    EXPECT_TRUE(key1 == 2);

    // Add some more values
    data.number = 789;
    data.pos[0] = -8888888.55;
    data.pos[2] = 99;
    rg_test_struct_map_data *other_key_in_map = rg_struct_map_set(struct_map, 987654, &data);
    EXPECT_NOT_NULL(other_key_in_map);
    EXPECT_TRUE(rg_struct_map_count(struct_map) == 2);
    // The slot that was used before for key2 should now be reused
    EXPECT_TRUE(other_key_in_map == key2_in_map);
    EXPECT_TRUE(other_key_in_map->number == 789);
    EXPECT_TRUE(other_key_in_map->pos[0] == -8888888.55);
    EXPECT_TRUE(other_key_in_map->pos[1] == 88);
    EXPECT_TRUE(other_key_in_map->pos[2] == 99);

    // Add even more values
    data.number = 542;
    data.pos[0] = 0.1;
    data.pos[1] = 30.42;
    data.pos[2] = 3.141592;
    rg_test_struct_map_data *key3_in_map = rg_struct_map_set(struct_map, 3, &data);
    EXPECT_NOT_NULL(key3_in_map);
    EXPECT_TRUE(rg_struct_map_count(struct_map) == 3);

    data = (rg_test_struct_map_data) {};

    // The data should still exist
    // However, since we exceeded the capacity of the storage vector, it got moved somewhere else
    // We need to get the pointers again
    rg_test_struct_map_data *new_key2_in_map = rg_struct_map_get(struct_map, 2);
    EXPECT_NOT_NULL(new_key2_in_map);
    EXPECT_TRUE(key1_in_map == new_key2_in_map);
    EXPECT_TRUE(new_key2_in_map->number == 89);
    EXPECT_TRUE(new_key2_in_map->pos[0] == 78);
    EXPECT_TRUE(new_key2_in_map->pos[1] == -9.5);
    EXPECT_TRUE(new_key2_in_map->pos[2] == 2);

    rg_test_struct_map_data *new_key3_in_map = rg_struct_map_get(struct_map, 3);
    EXPECT_NOT_NULL(new_key3_in_map);
    // Key 3 is the same, since it was added after the reallocation
    EXPECT_TRUE(key3_in_map == new_key3_in_map);
    EXPECT_TRUE(key3_in_map->number == 542);
    EXPECT_TRUE(key3_in_map->pos[0] == 0.1);
    EXPECT_TRUE(key3_in_map->pos[1] == 30.42);
    EXPECT_TRUE(key3_in_map->pos[2] == 3.141592);

    rg_test_struct_map_data *new_other_key_in_map = rg_struct_map_get(struct_map, 987654);
    EXPECT_NOT_NULL(new_other_key_in_map);
    EXPECT_TRUE(other_key_in_map == new_other_key_in_map);
    EXPECT_TRUE(new_other_key_in_map->number == 789);
    EXPECT_TRUE(new_other_key_in_map->pos[0] == -8888888.55);
    EXPECT_TRUE(new_other_key_in_map->pos[1] == 88);
    EXPECT_TRUE(new_other_key_in_map->pos[2] == 99);

    // Iterator
    rg_struct_map_it it = rg_struct_map_iterator(struct_map);

    bool res = rg_struct_map_next(&it);
    EXPECT_TRUE(res);
    EXPECT_TRUE(it.key == 2);
    EXPECT_TRUE(it.value == new_key2_in_map);

    res = rg_struct_map_next(&it);
    EXPECT_TRUE(res);
    EXPECT_TRUE(it.key == 987654);
    EXPECT_TRUE(it.value == new_other_key_in_map);

    res = rg_struct_map_next(&it);
    EXPECT_TRUE(res);
    EXPECT_TRUE(it.key == 3);
    EXPECT_TRUE(it.value == new_key3_in_map);

    // End of iteration
    res = rg_struct_map_next(&it);
    EXPECT_FALSE(res);
    EXPECT_NULL((void*) it.key);
    EXPECT_NULL(it.value);

    // That shouldn't have changed
    EXPECT_TRUE(rg_struct_map_count(struct_map) == 3);

    // Update a value
    data.number = 77777;
    data.pos[0] = 1.6180339887;
    data.pos[1] = 5;
    data.pos[2] = 42.4242;
    rg_test_struct_map_data *updated_other_key_in_map = rg_struct_map_set(struct_map, 987654, &data);
    EXPECT_NOT_NULL(updated_other_key_in_map);
    EXPECT_TRUE(rg_struct_map_count(struct_map) == 3);
    EXPECT_TRUE(updated_other_key_in_map == new_other_key_in_map);
    EXPECT_TRUE(updated_other_key_in_map->number == 77777);
    EXPECT_TRUE(updated_other_key_in_map->pos[0] == 1.6180339887);
    EXPECT_TRUE(updated_other_key_in_map->pos[1] == 5);
    EXPECT_TRUE(updated_other_key_in_map->pos[2] == 42.4242);

    // Destruction
    rg_destroy_struct_map(&struct_map);
    EXPECT_NULL(struct_map);
}