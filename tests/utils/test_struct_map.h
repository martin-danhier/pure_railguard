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
    ASSERT_NOT_NULL(struct_map);

    ASSERT_TRUE(rg_struct_map_count(struct_map) == 0);

    // Populate it
    rg_test_struct_map_data  data        = {.number = 42, .pos = {7, -9.5, 2}};
    rg_test_struct_map_data *key1_in_map = rg_struct_map_set(struct_map, "key1", &data);
    ASSERT_NOT_NULL(key1_in_map);
    ASSERT_TRUE(rg_struct_map_count(struct_map) == 1);

    // Update data and save another
    data.number                          = 89;
    data.pos[0]                          = 78;
    rg_test_struct_map_data *key2_in_map = rg_struct_map_set(struct_map, "key2", &data);
    ASSERT_NOT_NULL(key2_in_map);
    ASSERT_TRUE(rg_struct_map_count(struct_map) == 2);
    ASSERT_TRUE(key1_in_map != key2_in_map);

    // Since the data is copied, the value should stay unchanged even if the original struct is modified
    data.pos[1] = 88;
    ASSERT_TRUE(key1_in_map->number == 42);
    ASSERT_TRUE(key1_in_map->pos[0] == 7);
    ASSERT_TRUE(key1_in_map->pos[1] == -9.5);
    ASSERT_TRUE(key1_in_map->pos[2] == 2);
    ASSERT_TRUE(key2_in_map->number == 89);
    ASSERT_TRUE(key2_in_map->pos[0] == 78);
    ASSERT_TRUE(key2_in_map->pos[1] == -9.5);
    ASSERT_TRUE(key2_in_map->pos[2] == 2);

    // Memory should be packed together
    // key1_in_map should be the first, then key2_in_map should follow
    // In between, there is the pointer to "key1"
    const char *key1 = *(const char **) ((void*) key1_in_map + sizeof(rg_test_struct_map_data));
    ASSERT_TRUE(strcmp(key1, "key1") == 0);
    ASSERT_TRUE((rg_test_struct_map_data *) ((void *) key1_in_map + sizeof(rg_test_struct_map_data) + sizeof(const char *))
                == key2_in_map);
    const char *key2 = *(const char **) ((void*) key2_in_map + sizeof(rg_test_struct_map_data));
    ASSERT_TRUE(strcmp(key2, "key2") == 0);

    // Erasing
    rg_struct_map_erase(struct_map, "key1");
    ASSERT_TRUE(rg_struct_map_count(struct_map) == 1);

    // Now, key1_in_map should point to key2 because it was moved at the beginning of the array
    ASSERT_TRUE(key1_in_map->number == 89);
    ASSERT_TRUE(key1_in_map->pos[0] == 78);
    ASSERT_TRUE(key1_in_map->pos[1] == -9.5);
    ASSERT_TRUE(key1_in_map->pos[2] == 2);
    key1 = *(const char **) ((void*) key1_in_map + sizeof(rg_test_struct_map_data));
    ASSERT_TRUE(strcmp(key1, "key2") == 0);

    // Add some more values
    data.number = 789;
    data.pos[0] = -8888888.55;
    data.pos[2] = 99;
    rg_test_struct_map_data *other_key_in_map = rg_struct_map_set(struct_map, "other_key", &data);
    ASSERT_NOT_NULL(other_key_in_map);
    ASSERT_TRUE(rg_struct_map_count(struct_map) == 2);
    // The slot that was used before for key2 should now be reused
    ASSERT_TRUE(other_key_in_map == key2_in_map);
    ASSERT_TRUE(other_key_in_map->number == 789);
    ASSERT_TRUE(other_key_in_map->pos[0] == -8888888.55);
    ASSERT_TRUE(other_key_in_map->pos[1] == 88);
    ASSERT_TRUE(other_key_in_map->pos[2] == 99);

    // Add even more values
    data.number = 542;
    data.pos[0] = 0.1;
    data.pos[1] = 30.42;
    data.pos[2] = 3.141592;
    rg_test_struct_map_data *key3_in_map = rg_struct_map_set(struct_map, "key3", &data);
    ASSERT_NOT_NULL(key3_in_map);
    ASSERT_TRUE(rg_struct_map_count(struct_map) == 3);

    data = (rg_test_struct_map_data) {};

    // The data should still exist
    // However, since we exceeded the capacity of the storage vector, it got moved somewhere else
    // We need to get the pointers again
    rg_test_struct_map_data *new_key2_in_map = rg_struct_map_get(struct_map, "key2");
    ASSERT_NOT_NULL(new_key2_in_map);
    ASSERT_TRUE(key1_in_map != new_key2_in_map);
    ASSERT_TRUE(new_key2_in_map->number == 89);
    ASSERT_TRUE(new_key2_in_map->pos[0] == 78);
    ASSERT_TRUE(new_key2_in_map->pos[1] == -9.5);
    ASSERT_TRUE(new_key2_in_map->pos[2] == 2);

    rg_test_struct_map_data *new_key3_in_map = rg_struct_map_get(struct_map, "key3");
    ASSERT_NOT_NULL(new_key3_in_map);
    // Key 3 is the same, since it was added after the reallocation
    ASSERT_TRUE(key3_in_map == new_key3_in_map);
    ASSERT_TRUE(key3_in_map->number == 542);
    ASSERT_TRUE(key3_in_map->pos[0] == 0.1);
    ASSERT_TRUE(key3_in_map->pos[1] == 30.42);
    ASSERT_TRUE(key3_in_map->pos[2] == 3.141592);

    rg_test_struct_map_data *new_other_key_in_map = rg_struct_map_get(struct_map, "other_key");
    ASSERT_NOT_NULL(new_other_key_in_map);
    ASSERT_TRUE(other_key_in_map != new_other_key_in_map);
    ASSERT_TRUE(new_other_key_in_map->number == 789);
    ASSERT_TRUE(new_other_key_in_map->pos[0] == -8888888.55);
    ASSERT_TRUE(new_other_key_in_map->pos[1] == 88);
    ASSERT_TRUE(new_other_key_in_map->pos[2] == 99);

    // Iterator
    rg_struct_map_it it = rg_struct_map_iterator(struct_map);

    bool res = rg_struct_map_next(&it);
    ASSERT_TRUE(res);
    ASSERT_TRUE(strcmp(it.key, "key2") == 0);
    ASSERT_TRUE(it.value == new_key2_in_map);

    res = rg_struct_map_next(&it);
    ASSERT_TRUE(res);
    ASSERT_TRUE(strcmp(it.key, "other_key") == 0);
    ASSERT_TRUE(it.value == new_other_key_in_map);

    res = rg_struct_map_next(&it);
    ASSERT_TRUE(res);
    ASSERT_TRUE(strcmp(it.key, "key3") == 0);
    ASSERT_TRUE(it.value == new_key3_in_map);

    // End of iteration
    res = rg_struct_map_next(&it);
    ASSERT_FALSE(res);
    ASSERT_NULL((void*) it.key);
    ASSERT_NULL(it.value);

    // That shouldn't have changed
    ASSERT_TRUE(rg_struct_map_count(struct_map) == 3);

    // Update a value
    data.number = 77777;
    data.pos[0] = 1.6180339887;
    data.pos[1] = 5;
    data.pos[2] = 42.4242;
    rg_test_struct_map_data *updated_other_key_in_map = rg_struct_map_set(struct_map, "other_key", &data);
    ASSERT_NOT_NULL(updated_other_key_in_map);
    ASSERT_TRUE(rg_struct_map_count(struct_map) == 3);
    ASSERT_TRUE(updated_other_key_in_map == new_other_key_in_map);
    ASSERT_TRUE(updated_other_key_in_map->number == 77777);
    ASSERT_TRUE(updated_other_key_in_map->pos[0] == 1.6180339887);
    ASSERT_TRUE(updated_other_key_in_map->pos[1] == 5);
    ASSERT_TRUE(updated_other_key_in_map->pos[2] == 42.4242);

    // Destruction
    rg_destroy_struct_map(&struct_map);
    ASSERT_NULL(struct_map);
}