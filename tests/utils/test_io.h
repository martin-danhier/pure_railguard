#pragma once

#include "../framework/test_framework.h"

#include <railguard/utils/io.h>
#include <stdlib.h>

#define TEST_TEXT_CONTENT "This is a file containing test text."
#define TEST_TEXT_SIZE 36

TEST(FileIO) {
    // Test reading a file
    char *contents = NULL;
    size_t length = 0;
    bool result = rg_load_file_binary(RG_CSTR_CONST("resources/test.txt"), (void **) &contents, &length);

    ASSERT_TRUE(result);
    rg_string contents_str = rg_create_string_from_buffer(contents, length);
    EXPECT_TRUE(rg_string_equals(contents_str, RG_CSTR_CONST(TEST_TEXT_CONTENT)));
    EXPECT_TRUE(length == TEST_TEXT_SIZE);

    // Free memory
    rg_free(contents);
    contents = NULL;
    rg_free(contents_str.data);
    contents_str.data = NULL;

    // A nonexisting file returns false
    result = rg_load_file_binary(RG_CSTR_CONST("resources/nonexisting.txt"), (void **) &contents, &length);
    EXPECT_FALSE(result);

}
