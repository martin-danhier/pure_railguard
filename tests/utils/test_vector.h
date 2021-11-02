#pragma once

#include "../framework/test_framework.h"
#include <railguard/utils/arrays.h>

TEST(Vector)
{
    // Creation
    rg_vector vec = {};
    rg_create_vector(4, sizeof(uint32_t), &vec);
    EXPECT_NOT_NULL(vec.data);
    EXPECT_TRUE(vec.capacity == 4);
    EXPECT_TRUE(vec.count == 0);
    EXPECT_TRUE(vec.element_size == sizeof(uint32_t));
    EXPECT_TRUE(rg_vector_is_empty(&vec));

    // Create some data that we can add in the vector
#define VALUE_COUNT 54
    uint32_t values[VALUE_COUNT] = {23,  344,    1,       0,  22,  UINT32_MAX, 24, 3456, 99,   3762938723, 11,  2345, 9832,  42,
                                    222, 776101, 1221212, 14, 4,   5,          6,  273,  9751, 62,         3,   8323, 93939, 8765421,
                                    11,  234,    154,     11, 989, 0,          77, 12,   1,    876,        902, 312,  873,   1,
                                    13,  4,      12,      4,  54,  987,        7,  02,   7,    1,          2,   34};

    // The first 4 should not resize the vector
    for (uint32_t i = 0; i < 4; i++)
    {
        uint32_t *res = rg_vector_push_back(&vec, &values[i]);
        EXPECT_NOT_NULL(res);
        EXPECT_TRUE(*res == values[i]);
        EXPECT_TRUE(vec.count == i + 1);
    }
    EXPECT_TRUE(vec.capacity == 4);
    EXPECT_TRUE(vec.count == 4);
    EXPECT_FALSE(rg_vector_is_empty(&vec));

    // This one should resize the vector by 1
    uint32_t *res = rg_vector_push_back(&vec, &values[4]);
    EXPECT_NOT_NULL(res);
    EXPECT_TRUE(*res == values[4]);
    EXPECT_TRUE(vec.capacity == 5);
    EXPECT_TRUE(vec.count == 5);

    // This one by 2
    res = rg_vector_push_back(&vec, &values[5]);
    EXPECT_NOT_NULL(res);
    EXPECT_TRUE(*res == values[5]);
    EXPECT_TRUE(vec.capacity == 7);
    EXPECT_TRUE(vec.count == 6);

    // These 2 by 4
    for (uint32_t i = 6; i < 8; i++)
    {
        res = rg_vector_push_back(&vec, &values[i]);
        EXPECT_NOT_NULL(res);
        EXPECT_TRUE(*res == values[i]);
        EXPECT_TRUE(vec.count == i + 1);
    }
    EXPECT_TRUE(vec.capacity == 11);
    EXPECT_TRUE(vec.count == 8);

    // These 4 by 8
    for (uint32_t i = 8; i < 12; i++)
    {
        res = rg_vector_push_back(&vec, &values[i]);
        EXPECT_NOT_NULL(res);
        EXPECT_TRUE(*res == values[i]);
        EXPECT_TRUE(vec.count == i + 1);
    }
    EXPECT_TRUE(vec.capacity == 19);
    EXPECT_TRUE(vec.count == 12);

    // These 8 by 16
    for (uint32_t i = 12; i < 20; i++)
    {
        res = rg_vector_push_back(&vec, &values[i]);
        EXPECT_NOT_NULL(res);
        EXPECT_TRUE(*res == values[i]);
        EXPECT_TRUE(vec.count == i + 1);
    }
    EXPECT_TRUE(vec.capacity == 35);
    EXPECT_TRUE(vec.count == 20);

    // And so on until the end of our value array
    for (uint32_t i = 20; i < VALUE_COUNT; i++)
    {
        res = rg_vector_push_back(&vec, &values[i]);
        EXPECT_NOT_NULL(res);
        EXPECT_TRUE(*res == values[i]);
        EXPECT_TRUE(vec.count == i + 1);
    }
    EXPECT_TRUE(vec.capacity == 67);
    EXPECT_TRUE(vec.count == VALUE_COUNT);

    // Test iterator
    rg_vector_it it = rg_vector_iterator(&vec);
    size_t       i  = 0;
    while (rg_vector_next(&it))
    {
        EXPECT_TRUE(it.index == i);
        EXPECT_TRUE(*(uint32_t *) it.value == values[i]);
        i++;
    }
    EXPECT_NULL(it.value);
    // Ensure that we checked all the values
    EXPECT_TRUE(i == VALUE_COUNT);

    // Test advanced push
    res = rg_vector_push_back_no_data(&vec);
    EXPECT_NOT_NULL(res);
    EXPECT_TRUE(vec.count == VALUE_COUNT + 1);
    *res = 42;

    // Try to get the values
    for (uint32_t idx = 0; idx < vec.count - 1; idx++)
    {
        res = rg_vector_get_element(&vec, idx);
        EXPECT_NOT_NULL(res);
        EXPECT_TRUE(*res == values[idx]);
    }
    res = rg_vector_get_element(&vec, vec.count - 1);
    EXPECT_NOT_NULL(res);
    EXPECT_TRUE(*res == 42);

    // Try again using only the data array (check location in memory)
    for (uint32_t idx = 0; idx < vec.count - 1; idx++)
    {
        res = vec.data + (idx * sizeof(uint32_t));
        EXPECT_NOT_NULL(res);
        EXPECT_TRUE(*res == values[idx]);
    }
    res = vec.data + ((vec.count - 1) * sizeof(uint32_t));
    EXPECT_NOT_NULL(res);
    EXPECT_TRUE(*res == 42);

    // Check last index
    EXPECT_TRUE(rg_vector_last_index(&vec) == VALUE_COUNT);

    // Check pop_back
    rg_vector_pop_back(&vec);
    EXPECT_TRUE(vec.count = VALUE_COUNT);
    EXPECT_TRUE(vec.capacity = 67);

    // Check set
    uint32_t value = 873287343;
    rg_vector_set_element(&vec, 22, &value);
    value = 3;
    rg_vector_set_element(&vec, 2, &value);
    value = 67;
    rg_vector_set_element(&vec, 13, &value);
    EXPECT_TRUE(*(uint32_t *) (vec.data + 22 * sizeof(uint32_t)) == 873287343);
    EXPECT_TRUE(*(uint32_t *) (vec.data + 2 * sizeof(uint32_t)) == 3);
    EXPECT_TRUE(*(uint32_t *) (vec.data + 13 * sizeof(uint32_t)) == 67);

    // Copy elem 13 in slot 2
    EXPECT_TRUE(rg_vector_copy(&vec, 13, 2));
    // 13 should not be modified
    EXPECT_TRUE(*(uint32_t *) (vec.data + 13 * sizeof(uint32_t)) == 67);
    EXPECT_TRUE(*(uint32_t *) (vec.data + 2 * sizeof(uint32_t)) == 67);

    // Clear
    // It shouldn't deallocate the data, only allow them to be overwritten by new pushes
    void *old_data = vec.data;
    rg_vector_clear(&vec);
    EXPECT_TRUE(vec.count == 0);
    EXPECT_TRUE(vec.capacity == 67);
    EXPECT_TRUE(vec.data == old_data);
    EXPECT_TRUE(vec.element_size == sizeof(uint32_t));

    // Ensure that a new push is added at the beginning
    value = 789678;
    res = rg_vector_push_back(&vec, &value);
    EXPECT_NOT_NULL(res);
    EXPECT_TRUE(*(uint32_t *) vec.data == 789678);
    EXPECT_TRUE(vec.count == 1);

    // Destruction
    rg_destroy_vector(&vec);
    EXPECT_NULL(vec.data);
    EXPECT_TRUE(vec.count == 0);
    EXPECT_TRUE(vec.capacity == 0);
}