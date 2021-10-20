#include "../framework/test_framework.h"
#include <railguard/utils/maps.h>

#include <string.h>

TEST(HashMap)
{
    rg_hash_map *map = rg_create_hash_map();
    ASSERT_NOT_NULL(map);

    // Define the values that will be used to populate the map
#define TEST_VALUES_COUNT 21
    int         values[TEST_VALUES_COUNT] = {4,    2, 27,      22, 999, 1,  -55, 0,  -100000, 28,      888,
                                     6432, 1, -999988, 4,  19,  32, 22,  11, 75,      99999999};
    const char *keys[TEST_VALUES_COUNT]   = {
        "key1",  "key2",  "key3",  "aaaaaaaaaaaa", "key5",  "long key with spaces inside it",
        "key7",  "wdfgj", "key9",  "key10",        "key11", "key12",
        "key13", "key14", "key15", "key16",        "key17", "a",
        "b",     "foo",   "bar",
    };

    // Populate the map
    for (int i = 0; i < TEST_VALUES_COUNT; i++)
    {
        bool success = rg_hash_map_set(map, keys[i], (rg_hash_map_value_t) (void *) &values[i]);
        ASSERT_TRUE(success);
    }

    // Check that all values are in the map
    for (int i = 0; i < TEST_VALUES_COUNT; i++)
    {
        rg_hash_map_get_result result = rg_hash_map_get(map, keys[i]);
        ASSERT_TRUE(result.exists);
        ASSERT_TRUE(*(int *) result.value.as_ptr == values[i]);
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
        for (int i = 0; i < TEST_VALUES_COUNT && !found; i++)
        {
            if (strcmp(it.key, keys[i]) == 0)
            {
                // Found the key, the value should match
                ASSERT_TRUE(*(int *) it.value.as_ptr == values[i]);
                found = true;

                // The key should not already have been found
                ASSERT_FALSE(found_indexes[i]);
                found_indexes[i] = true;
            }
        }
        ASSERT_TRUE(found);
    }
    // We must have found the same number of elements
    ASSERT_TRUE(values_in_iterator_count == TEST_VALUES_COUNT);

    // Check that a get with a wrong key returns NULL
    ASSERT_FALSE(rg_hash_map_get(map, "nonexisting").exists);
    ASSERT_FALSE(rg_hash_map_get(map, "nonsense").exists);
    ASSERT_FALSE(rg_hash_map_get(map, "xyz").exists);
    ASSERT_FALSE(rg_hash_map_get(map, "74dfg531dfg563dfg321dfg354df32dfg32dfg").exists);
    ASSERT_FALSE(rg_hash_map_get(map, "").exists);

    // Set with empty key should work
    ASSERT_TRUE(rg_hash_map_set(map, "", (rg_hash_map_value_t) (size_t) 75));
    rg_hash_map_get_result empty_key_value_result = rg_hash_map_get(map, "");
    ASSERT_TRUE(empty_key_value_result.exists);
    ASSERT_TRUE(empty_key_value_result.value.as_num == 75);

    // Test the editing of existing keys
    int new_value = 789456123;
    ASSERT_TRUE(rg_hash_map_set(map, "key12", (rg_hash_map_value_t) (void *) &new_value));
    rg_hash_map_get_result new_value_get_result = rg_hash_map_get(map, "key12");
    ASSERT_TRUE(new_value_get_result.exists);
    ASSERT_TRUE(*(int *) new_value_get_result.value.as_ptr == new_value);

    // Test the erasing of keys

    // Initial state: the key exists, and we have some number of elements in the map
    ASSERT_TRUE(rg_hash_map_get(map, "key5").exists);
    size_t old_count = rg_hash_map_count(map);
    // Do the erasing
    rg_hash_map_erase(map, "key5");
    // Final state: the key does not exist anymore, and the number of elements has decreased by one
    size_t new_count = rg_hash_map_count(map);
    ASSERT_TRUE(new_count == old_count - 1);
    ASSERT_FALSE(rg_hash_map_get(map, "key5").exists);

    rg_destroy_hash_map(&map);
    ASSERT_NULL(map);
}