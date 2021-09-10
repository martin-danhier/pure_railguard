#include <railguard/utils/maps.h>
#include <string.h>
#include <assert.h>

int main(void)
{
    rg_hash_map *map = rg_create_hash_map();
    assert(map != NULL);

    // Define the values that will be used to populate the map
#define TEST_VALUES_COUNT 21
    int values[TEST_VALUES_COUNT] = {
        4,
        2,
        27,
        22,
        999,
        1,
        -55,
        0,
        -100000,
        28,
        888,
        6432,
        1,
        -999988,
        4,
        19,
        32,
        22,
        11,
        75,
        99999999
    };
    const char *keys[TEST_VALUES_COUNT] = {
        "key1",
        "key2",
        "key3",
        "aaaaaaaaaaaa",
        "key5",
        "long key with spaces inside it",
        "key7",
        "wdfgj",
        "key9",
        "key10",
        "key11",
        "key12",
        "key13",
        "key14",
        "key15",
        "key16",
        "key17",
        "a",
        "b",
        "foo",
        "bar"
    };

    // Populate the map
    for (int i = 0; i < TEST_VALUES_COUNT; i++)
    {
        bool success = rg_hash_map_set(map, keys[i], &values[i]);
        assert(success);
    }

    // Check that all values are in the map
    for (int i = 0; i < TEST_VALUES_COUNT; i++)
    {
        int *value = rg_hash_map_get(map, keys[i]);
        assert(value != NULL);
        assert(*value == values[i]);
    }

    // Iterator
    rg_hash_map_it it = rg_hash_map_iterator(map);
    int values_in_iterator_count = 0;
    bool found_indexes[TEST_VALUES_COUNT] = {};
    while (rg_hash_map_next(&it)) {
        values_in_iterator_count++;

        // We need to check that all values are in the array and that the values are correct
        // However, the order is not necessarily the same as in the values array
        // Thus, we need to find the key each time manually
        bool found = false;
        for (int i = 0; i < TEST_VALUES_COUNT && !found; i++) {
            if (strcmp(it.key, keys[i]) == 0) {
                // Found the key, the value should match
                assert(*(int*) it.value == values[i]);
                found = true;

                // The key should not already have been found
                assert(found_indexes[i] == false);
                found_indexes[i] = true;
            }
        }
        assert(found);
    }
    // We must have found the same number of elements
    assert(values_in_iterator_count == TEST_VALUES_COUNT);

    // Check that a get with a wrong key returns NULL
    assert(rg_hash_map_get(map, "nonexisting") == NULL);
    assert(rg_hash_map_get(map, "nonsense") == NULL);
    assert(rg_hash_map_get(map, "xyz") == NULL);
    assert(rg_hash_map_get(map, "74dfg531dfg563dfg321dfg354df32dfg32dfg") == NULL);
    assert(rg_hash_map_get(map, "") == NULL);

    // Set with empty key should work
    assert(rg_hash_map_set(map, "", &values[0]) == true);
    int *empty_key_value = rg_hash_map_get(map, "");
    assert(empty_key_value != NULL);
    assert(*empty_key_value == values[0]);

    // Test the editing of existing keys
    int new_value = 789456123;
    assert(rg_hash_map_set(map, "key12", &new_value));
    int *new_value_get = rg_hash_map_get(map, "key12");
    assert(new_value_get != NULL);
    assert(*new_value_get == new_value);

    // Test the erasing of keys

    // Initial state: the key exists, and we have some number of elements in the map
    assert(rg_hash_map_get(map, "key5") != NULL);
    size_t old_count = rg_hash_map_count(map);
    // Do the erasing
    rg_hash_map_erase(map, "key5");
    // Final state: the key does not exist anymore, and the number of elements has decreased by one
    size_t new_count = rg_hash_map_count(map);
    assert(new_count == old_count - 1);
    assert(rg_hash_map_get(map, "key5") == NULL);

    rg_destroy_hash_map(&map);
    assert (map == NULL);

    return 0;
}