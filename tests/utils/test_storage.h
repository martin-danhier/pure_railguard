#pragma once

#include "../framework/test_framework.h"
#include <railguard/utils/storage.h>

typedef struct rg_test_storage_data {
    uint64_t a;
    double b;
} rg_test_storage_data;

TEST(Storage)
{
    // Create storage
    rg_storage *storage = rg_create_storage(sizeof(rg_test_storage_data));
    ASSERT_NOT_NULL(storage);

    // Populate storage
    EXPECT_TRUE(rg_storage_count(storage) == 0);

    rg_test_storage_data data = {
        .a = 0xDEADBEEF,
        .b = 3.14159265358979323846
    };
    rg_storage_id id1 = rg_storage_push(storage, &data);
    EXPECT_TRUE(id1 != RG_STORAGE_NULL_ID);
    EXPECT_TRUE(id1 == 1);
    EXPECT_TRUE(rg_storage_count(storage) == 1);

    // Update data and push another value. Do that several times
    data.a = 0xCAFEBABE;
    data.b = 2.7182818284590452353;
    rg_storage_id id2 = rg_storage_push(storage, &data);
    EXPECT_TRUE(id2 != RG_STORAGE_NULL_ID);
    EXPECT_TRUE(id2 == 2);
    EXPECT_TRUE(rg_storage_count(storage) == 2);

    // Third one
    data.a = 0xABADCAFE;
    data.b = 1.4142135623730951;
    rg_storage_id id3 = rg_storage_push(storage, &data);
    EXPECT_TRUE(id3 != RG_STORAGE_NULL_ID);
    EXPECT_TRUE(id3 == 3);
    EXPECT_TRUE(rg_storage_count(storage) == 3);

    // Modify the original data struct to ensure that it is copied
    data.a = 0;
    data.b = 0;

    // Try to get data now
    rg_test_storage_data *data1 = rg_storage_get(storage, id1);
    ASSERT_NOT_NULL(data1);
    EXPECT_TRUE(data1->a == 0xDEADBEEF);
    EXPECT_TRUE(data1->b == 3.14159265358979323846);
    EXPECT_TRUE(rg_storage_exists(storage, id1));

    rg_test_storage_data *data2 = rg_storage_get(storage, id2);
    ASSERT_NOT_NULL(data2);
    EXPECT_TRUE(data2->a == 0xCAFEBABE);
    EXPECT_TRUE(data2->b == 2.7182818284590452353);
    EXPECT_TRUE(rg_storage_exists(storage, id2));

    rg_test_storage_data *data3 = rg_storage_get(storage, id3);
    ASSERT_NOT_NULL(data3);
    EXPECT_TRUE(data3->a == 0xABADCAFE);
    EXPECT_TRUE(data3->b == 1.4142135623730951);
    EXPECT_TRUE(rg_storage_exists(storage, id3));

    // Check if all the data is located in a packed array
    EXPECT_TRUE(data2 == ((void*) data1) + sizeof(rg_test_storage_data) + sizeof(rg_hash_map_key_t));
    EXPECT_TRUE(data3 == ((void*) data2) + sizeof(rg_test_storage_data) + sizeof(rg_hash_map_key_t));

    // Try to get data that does not exist
    rg_test_storage_data *invalid_data = rg_storage_get(storage, 0xDEADBEEF);
    EXPECT_TRUE(invalid_data == NULL);
    EXPECT_FALSE(rg_storage_exists(storage, 0xDEADBEEF));

    // Try to remove data
    rg_storage_erase(storage, id2);
    EXPECT_TRUE(rg_storage_count(storage) == 2);

    // data3 should have been moved to where data2 was.
    // So now, the "data2" pointer points to data3, and the data3 pointer is invalid
    EXPECT_TRUE(data2->a = 0xABADCAFE);
    EXPECT_TRUE(data2->b = 1.4142135623730951);

    // Try to add data again
    data.a = 0xF00DBABE;
    data.b = 1.6180339887498948482;
    rg_storage_id id4 = rg_storage_push(storage, &data);
    EXPECT_TRUE(id4 != RG_STORAGE_NULL_ID);
    EXPECT_TRUE(id4 == 4);
    EXPECT_TRUE(rg_storage_count(storage) == 3);

    // Try to get data again
    rg_test_storage_data *data4 = rg_storage_get(storage, id4);
    ASSERT_NOT_NULL(data4);
    EXPECT_TRUE(data4->a == 0xF00DBABE);
    EXPECT_TRUE(data4->b == 1.6180339887498948482);
    EXPECT_TRUE(rg_storage_exists(storage, id4));

    // data4 should be where data3 was
    EXPECT_TRUE(data3 == data4);

    // Getting data3 should return the new position
    data3 = rg_storage_get(storage, id3);
    ASSERT_NOT_NULL(data3);
    EXPECT_TRUE(data3 == data2);

    // Test iterator
    rg_storage_it it = rg_storage_iterator(storage);
    EXPECT_NULL(it.value);
    EXPECT_TRUE(it.id == RG_STORAGE_NULL_ID);

    // First element
    ASSERT_TRUE(rg_storage_next(&it));
    EXPECT_TRUE(it.id == id1);
    EXPECT_TRUE(it.value == data1);

    // Second element
    ASSERT_TRUE(rg_storage_next(&it));
    EXPECT_TRUE(it.id == id3);
    EXPECT_TRUE(it.value == data3);

    // Third element
    ASSERT_TRUE(rg_storage_next(&it));
    EXPECT_TRUE(it.id == id4);
    EXPECT_TRUE(it.value == data4);

    // Last time should return false
    ASSERT_FALSE(rg_storage_next(&it));
    EXPECT_NULL(it.value);
    EXPECT_TRUE(it.id == RG_STORAGE_NULL_ID);

    // Try to call functions with invalid parameters to see if they can handle unexpected situations
    EXPECT_NULL(rg_storage_get(NULL, id1));
    EXPECT_FALSE(rg_storage_exists(NULL, id1));
    EXPECT_TRUE(rg_storage_push(NULL, &data) == RG_STORAGE_NULL_ID);
    EXPECT_TRUE(rg_storage_count(NULL) == 0);
    EXPECT_FALSE(rg_storage_next(NULL));

    // Clean up
    rg_destroy_storage(&storage);
    EXPECT_NULL(storage);
}