#pragma once

#include "../framework/test_framework.h"
#include "test_struct_map.h"
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
    EXPECT_FALSE(rg_hash_map_set(map, 0, (rg_hash_map_value_t) {(void *) &values[0]}));

    // Populate the map
    for (uint64_t i = 0; i < TEST_VALUES_COUNT; i++)
    {
        bool success = rg_hash_map_set(map, i + 1, (rg_hash_map_value_t) {(void *) &values[i]});
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
    bool           found_indexes[TEST_VALUES_COUNT] = {0};
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
    EXPECT_TRUE(rg_hash_map_set(map, 12, (rg_hash_map_value_t) {(void *) &new_value}));
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

TEST(HashFunction)
{
    // The hash should always return the same value for the same key

    // Set a list of 10 very different uint64_t keys
    uint64_t keys[10] = {
        0xC0FFEE,
        0xDEADBEEF,
        0xBEEFCAFE,
        0x833FDEAD,
        0xB0BABEEF,
        0xABABABAB,
        0xCAFEBAD,
        0xCAFEBABE,
        0xBADCAFE,
        0xDAD15ABEEF,
    };

    // Check that the hash function is consistent
    for (uint64_t i = 0; i < 10; i++)
    {
        uint64_t hash1 = rg_hash_map_hash(keys[i]);
        uint64_t hash2 = rg_hash_map_hash(keys[i]);
        EXPECT_TRUE(hash1 == hash2);
    }
}

TEST(HashMap_Stress)
{
    // Create a hash map
    rg_hash_map *map = rg_create_hash_map();
    ASSERT_NOT_NULL(map);

    // Add a lot of values
    for (uint64_t i = 1; i < 10000; i++)
    {
        bool success = rg_hash_map_set(map, i, (rg_hash_map_value_t) {.as_num = i});
        EXPECT_TRUE(success);

        // Check that the value is in the map
        rg_hash_map_get_result result = rg_hash_map_get(map, i);
        EXPECT_TRUE(result.exists);
        EXPECT_TRUE(result.value.as_num == i);

        // Check count
        EXPECT_TRUE(rg_hash_map_count(map) == i);
    }

    // Now to it has been expanded many times, check that all values are still there
    for (uint64_t i = 1; i < 10000; i++)
    {
        rg_hash_map_get_result result = rg_hash_map_get(map, i);
        EXPECT_TRUE(result.exists);
        EXPECT_TRUE(result.value.as_num == i);
    }

    // Destroy a part of the values
    for (uint64_t i = 1; i < 10000; i += 2)
    {
        // Check that the value is in the map
        rg_hash_map_get_result result = rg_hash_map_get(map, i);
        EXPECT_TRUE(result.exists);
        EXPECT_TRUE(result.value.as_num == i);

        rg_hash_map_erase(map, i);

        // Check that the value is not in the map
        result = rg_hash_map_get(map, i);
        EXPECT_FALSE(result.exists);
    }

    // Check that the count is correct
    EXPECT_TRUE(rg_hash_map_count(map) == 4999);

    // Check that all remaining values are still there
    for (uint64_t i = 2; i < 10000; i += 2)
    {
        rg_hash_map_get_result result = rg_hash_map_get(map, i);
        EXPECT_TRUE(result.exists);
        EXPECT_TRUE(result.value.as_num == i);
    }

    // Clean up
    rg_destroy_hash_map(&map);
    EXPECT_NULL(map);
}

TEST(HashMap_Erase)
{
    // This test tests a problem that occurred in the past:
    // Four values with these keys were added in order
    // Then, they were deleted in order
    // Though, when trying to delete the last one, it wasn't found in the map
    // Thus, the deletion was skipped, but the value still existed in the map, taking memory

    // Create a hash map
    rg_hash_map *map = rg_create_hash_map();
    ASSERT_NOT_NULL(map);
    EXPECT_TRUE(map->capacity == 1);
    EXPECT_TRUE(map->count == 0);

    // Define keys that caused problems in the past
    rg_hash_map_key_t keys[4] = {
        0x00000270E8C65E20, // -> with hash, points to index 7 with capacity 8
        0x00000270E8C66000, // -> 1
        0x00000270E8C66190, // -> 2
        0x00000270E8C656A0, // -> 7
    };

    // Add the first one
    bool success = rg_hash_map_set(map, keys[0], (rg_hash_map_value_t) {.as_num = 0});
    EXPECT_TRUE(success);
    EXPECT_TRUE(map->capacity == 2);
    EXPECT_TRUE(map->count == 1);

    // It should be in the map
    rg_hash_map_get_result result = rg_hash_map_get(map, keys[0]);
    EXPECT_TRUE(result.exists);
    EXPECT_TRUE(result.value.as_num == 0);

    // Add the second one
    success = rg_hash_map_set(map, keys[1], (rg_hash_map_value_t) {.as_num = 1});
    EXPECT_TRUE(success);
    EXPECT_TRUE(map->capacity == 4);
    EXPECT_TRUE(map->count == 2);

    // The map was expanded, both should be in the map
    for (uint64_t i = 0; i < 2; i++)
    {
        result = rg_hash_map_get(map, keys[i]);
        EXPECT_TRUE(result.exists);
        EXPECT_TRUE(result.value.as_num == i);
    }

    // Add the third one
    success = rg_hash_map_set(map, keys[2], (rg_hash_map_value_t) {.as_num = 2});
    EXPECT_TRUE(success);
    EXPECT_TRUE(map->capacity == 8);
    EXPECT_TRUE(map->count == 3);

    // It was expanded again, all three should be in the map
    for (uint64_t i = 0; i < 3; i++)
    {
        result = rg_hash_map_get(map, keys[i]);
        EXPECT_TRUE(result.exists);
        EXPECT_TRUE(result.value.as_num == i);
    }

    // Add the fourth one
    success = rg_hash_map_set(map, keys[3], (rg_hash_map_value_t) {.as_num = 3});
    EXPECT_TRUE(success);
    EXPECT_TRUE(map->capacity == 8);
    EXPECT_TRUE(map->count == 4);

    // It was not expanded again, but for completion's sake , all four should be in the map
    for (uint64_t i = 0; i < 4; i++)
    {
        result = rg_hash_map_get(map, keys[i]);
        EXPECT_TRUE(result.exists);
        EXPECT_TRUE(result.value.as_num == i);
    }

    // -> If everything worked here, then the SET and the EXPAND should work
    // Things start getting a little more complicated now, because we're going to delete the values
    // If a hole is created in the map array, some values can become inaccessible
    // The erase function should remove the values in such a way that everything stays accessible

    // Currently, the array should look like this:
    // [key4, key2, key3, NULL, NULL, NULL, NULL, key1]

    // key1, key2 and key3 are at the right place (index returned by hash)
    // key4 wants to be in the index 7, but there's already key1 there

    // The bug was:
    // -> When key1 is removed, key4 becomes inaccessible (it should be moved to index 7)

    // Erase them in order
    for (uint64_t i = 0; i < 4; i++) {
        // All keys that were not deleted yet should still be there
        for (uint64_t j = i; j < 4; j++) {
            result = rg_hash_map_get(map, keys[j]);
            EXPECT_TRUE(result.exists);
            EXPECT_TRUE(result.value.as_num == j);
        }

        // Erase it
        rg_hash_map_erase(map, keys[i]);

        // It should not be there
        result = rg_hash_map_get(map, keys[i]);
        EXPECT_FALSE(result.exists);

        // The others should still be there
        for (uint64_t j = i + 1; j < 4; j++) {
            result = rg_hash_map_get(map, keys[j]);
            EXPECT_TRUE(result.exists);
            EXPECT_TRUE(result.value.as_num == j);
        }
    }

    // Clean up
    rg_destroy_hash_map(&map);
    EXPECT_NULL(map);
}