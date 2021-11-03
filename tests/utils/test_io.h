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

    // Add null char because strcmp needs it, and it is not loaded by the function since we are in binary mode
    char *contents_str = malloc(length + 1);
    memcpy(contents_str, contents, length);
    contents_str[length] = '\0';

    EXPECT_TRUE(result);
    EXPECT_TRUE(strcmp(contents_str, TEST_TEXT_CONTENT) == 0);
    EXPECT_TRUE(length == TEST_TEXT_SIZE);

    // Free memory
    free(contents);
    free(contents_str);
    contents = NULL;
    contents_str = NULL;

    // A nonexisting file returns false
    result = rg_load_file_binary(RG_CSTR_CONST("resources/nonexisting.txt"), (void **) &contents, &length);
    EXPECT_FALSE(result);
}
