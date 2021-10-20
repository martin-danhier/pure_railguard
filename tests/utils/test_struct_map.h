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

    // Populate it
    rg_test_struct_map_data  data        = {.number = 42, .pos = {7, -9.5, 2}};
    rg_test_struct_map_data *key1_in_map = rg_struct_map_set(struct_map, "key1", &data);
    ASSERT_NOT_NULL(key1_in_map);

    // Update data and save another
    data.number                          = 89;
    data.pos[0]                          = 78;
    rg_test_struct_map_data *key2_in_map = rg_struct_map_set(struct_map, "key2", &data);
    ASSERT_NOT_NULL(key2_in_map);
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
    // Now, key1_in_map should point to key2 because it was moved at the beginning of the array
    ASSERT_TRUE(key1_in_map->number == 89);
    ASSERT_TRUE(key1_in_map->pos[0] == 78);
    ASSERT_TRUE(key1_in_map->pos[1] == -9.5);
    ASSERT_TRUE(key1_in_map->pos[2] == 2);
    key1 = *(const char **) ((void*) key1_in_map + sizeof(rg_test_struct_map_data));
    ASSERT_TRUE(strcmp(key1, "key2") == 0);

    // Destruction
    rg_destroy_struct_map(&struct_map);
    ASSERT_NULL(struct_map);
}