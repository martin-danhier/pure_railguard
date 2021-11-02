#pragma once

#include "../framework/test_framework.h"
#include <railguard/utils/maps.h>

#include <string.h>

TEST(HashMap)
{
    rg_hash_map *map = rg_create_hash_map();
    ASSERT_NOT_NULL(map);

    // Define the values that will be used to populate the map
#define TEST_VALUES_COUNT 21
    int values[TEST_VALUES_COUNT] = {4,    2, 27,      22, 999, 1,  -55, 0,  -100000, 28,      888,
                                     6432, 1, -999988, 4,  19,  32, 22,  11, 75,      99999999};

    // The 0 key should not be allowed
    EXPECT_FALSE(rg_hash_map_set(map, 0, (rg_hash_map_value_t) (void *) &values[0]));

    // Populate the map
    for (uint64_t i = 0; i < TEST_VALUES_COUNT; i++)
    {
        bool success = rg_hash_map_set(map, i + 1, (rg_hash_map_value_t) (void *) &values[i]);
        EXPECT_TRUE(success);
    }

    // Check that all values are in the map
    for (uint64_t i = 0; i < TEST_VALUES_COUNT; i++)
    {
        rg_hash_map_get_result result = rg_hash_map_get(map, i + 1);
        ASSERT_TRUE(result.exists);
        EXPECT_TRUE(*(int *) result.value.as_ptr == values[i]);
    }

    // Iterator
    rg_hash_map_it it                               = rg_hash_map_iterator(map);
    int            values_in_iterator_count         = 0;
    bool           found_indexes[TEST_VALUES_COUNT] = {};
    while (rg_hash_map_next(&it))
    {
        values_in_iterator_count++;

        // We need to check that all values are in the array and that the values are correct
        // However, the order is not necessarily the same as in the values array
        // Thus, we need to find the key each time manually
        bool found = false;
        for (uint64_t i = 0; i < TEST_VALUES_COUNT && !found; i++)
        {
            if (it.key == i + 1)
            {
                // Found the key, the value should match
                EXPECT_TRUE(*(int *) it.value.as_ptr == values[i]);
                found = true;

                // The key should not already have been found
                EXPECT_FALSE(found_indexes[i]);
                found_indexes[i] = true;
            }
        }
        EXPECT_TRUE(found);
    }
    // We must have found the same number of elements
    EXPECT_TRUE(values_in_iterator_count == TEST_VALUES_COUNT);

    // Check that a get with a wrong key returns NULL
    EXPECT_FALSE(rg_hash_map_get(map, 87543656).exists);
    EXPECT_FALSE(rg_hash_map_get(map, 5555).exists);
    EXPECT_FALSE(rg_hash_map_get(map, UINT64_MAX).exists);
    EXPECT_FALSE(rg_hash_map_get(map, RG_HASH_MAP_NULL_KEY).exists);

    // Test the editing of existing keys
    int new_value = 789456123;
    EXPECT_TRUE(rg_hash_map_set(map, 12, (rg_hash_map_value_t) (void *) &new_value));
    rg_hash_map_get_result new_value_get_result = rg_hash_map_get(map, 12);
    EXPECT_TRUE(new_value_get_result.exists);
    EXPECT_TRUE(*(int *) new_value_get_result.value.as_ptr == new_value);

    // Test the erasing of keys

    // Initial state: the key exists, and we have some number of elements in the map
    EXPECT_TRUE(rg_hash_map_get(map, 5).exists);
    size_t old_count = rg_hash_map_count(map);
    // Do the erasing
    rg_hash_map_erase(map, 5);
    // Final state: the key does not exist anymore, and the number of elements has decreased by one
    size_t new_count = rg_hash_map_count(map);
    EXPECT_TRUE(new_count == old_count - 1);
    EXPECT_FALSE(rg_hash_map_get(map, 5).exists);

    rg_destroy_hash_map(&map);
    EXPECT_NULL(map);
}