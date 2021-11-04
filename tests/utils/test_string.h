#pragma once

#include "../framework/test_framework.h"
#include <railguard/utils/string.h>

TEST(String)
{
    // Empty string is empty
    rg_string empty = RG_EMPTY_STRING;
    EXPECT_TRUE(rg_string_is_empty(empty));
    ASSERT_NOT_NULL(empty.data);
    EXPECT_TRUE(empty.data[0] == '\0');
    EXPECT_TRUE(empty.length == 0);
    empty = RG_CSTR_CONST("");
    EXPECT_TRUE(rg_string_is_empty(empty));
    ASSERT_NOT_NULL(empty.data);
    EXPECT_TRUE(empty.data[0] == '\0');
    EXPECT_TRUE(empty.length == 0);
    empty = RG_CSTR("");
    EXPECT_TRUE(rg_string_is_empty(empty));
    ASSERT_NOT_NULL(empty.data);
    EXPECT_TRUE(empty.data[0] == '\0');
    EXPECT_TRUE(empty.length == 0);

    // Create string from const cstr
    rg_string s = RG_CSTR_CONST("Hello");
    ASSERT_TRUE(s.length == 5);
    ASSERT_NOT_NULL(s.data);
    EXPECT_TRUE(s.data[0] == 'H');
    EXPECT_TRUE(s.data[1] == 'e');
    EXPECT_TRUE(s.data[2] == 'l');
    EXPECT_TRUE(s.data[3] == 'l');
    EXPECT_TRUE(s.data[4] == 'o');
    EXPECT_FALSE(rg_string_is_empty(s));

    // Load string from non const cstr
    char     *cstr = "World";
    rg_string s2   = RG_CSTR(cstr);
    ASSERT_TRUE(s2.length == 5);
    ASSERT_NOT_NULL(s2.data);
    EXPECT_TRUE(s2.data[0] == 'W');
    EXPECT_TRUE(s2.data[1] == 'o');
    EXPECT_TRUE(s2.data[2] == 'r');
    EXPECT_TRUE(s2.data[3] == 'l');
    EXPECT_TRUE(s2.data[4] == 'd');
    EXPECT_FALSE(rg_string_is_empty(s2));

    // Load string from other string
    rg_string s3 = rg_clone_string(s2);
    ASSERT_TRUE(s3.length == 5);
    ASSERT_NOT_NULL(s3.data);
    EXPECT_TRUE(s3.data[0] == 'W');
    EXPECT_TRUE(s3.data[1] == 'o');
    EXPECT_TRUE(s3.data[2] == 'r');
    EXPECT_TRUE(s3.data[3] == 'l');
    EXPECT_TRUE(s3.data[4] == 'd');
    EXPECT_TRUE(s3.data != s2.data);
    EXPECT_FALSE(rg_string_is_empty(s3));

    // Try to concat them
    rg_string s4 = rg_string_concat(s, s2);
    ASSERT_TRUE(s4.length == 10);
    ASSERT_NOT_NULL(s4.data);
    EXPECT_TRUE(s4.data[0] == 'H');
    EXPECT_TRUE(s4.data[1] == 'e');
    EXPECT_TRUE(s4.data[2] == 'l');
    EXPECT_TRUE(s4.data[3] == 'l');
    EXPECT_TRUE(s4.data[4] == 'o');
    EXPECT_TRUE(s4.data[5] == 'W');
    EXPECT_TRUE(s4.data[6] == 'o');
    EXPECT_TRUE(s4.data[7] == 'r');
    EXPECT_TRUE(s4.data[8] == 'l');
    EXPECT_TRUE(s4.data[9] == 'd');
    EXPECT_FALSE(rg_string_is_empty(s4));

    // If there is one or two empty strings in the parameters of concat, it still works
    rg_string s5 = rg_string_concat(s, RG_EMPTY_STRING);
    ASSERT_TRUE(s5.length == 5);
    ASSERT_NOT_NULL(s5.data);
    EXPECT_TRUE(s5.data[0] == 'H');
    EXPECT_TRUE(s5.data[1] == 'e');
    EXPECT_TRUE(s5.data[2] == 'l');
    EXPECT_TRUE(s5.data[3] == 'l');
    EXPECT_TRUE(s5.data[4] == 'o');
    EXPECT_FALSE(rg_string_is_empty(s5));

    rg_string s6 = rg_string_concat(RG_EMPTY_STRING, s);
    ASSERT_TRUE(s6.length == 5);
    ASSERT_NOT_NULL(s6.data);
    EXPECT_TRUE(s6.data[0] == 'H');
    EXPECT_TRUE(s6.data[1] == 'e');
    EXPECT_TRUE(s6.data[2] == 'l');
    EXPECT_TRUE(s6.data[3] == 'l');
    EXPECT_TRUE(s6.data[4] == 'o');
    EXPECT_FALSE(rg_string_is_empty(s6));

    rg_string s7 = rg_string_concat(RG_EMPTY_STRING, RG_EMPTY_STRING);
    ASSERT_TRUE(s7.length == 0);
    ASSERT_NOT_NULL(s7.data);
    EXPECT_TRUE(s7.data[0] == '\0');
    EXPECT_TRUE(rg_string_is_empty(s7));

    // Test equality

    // Equality is symmetric
    EXPECT_FALSE(rg_string_equals(s, s2));
    EXPECT_FALSE(rg_string_equals(s2, s));
    EXPECT_TRUE(rg_string_equals(s2, s3));
    EXPECT_TRUE(rg_string_equals(s3, s2));

    // Equality is reflexive
    EXPECT_TRUE(rg_string_equals(s, s));

    // Equality is transitive
    EXPECT_TRUE(rg_string_equals(s, s5));
    EXPECT_TRUE(rg_string_equals(s5, s6));
    EXPECT_TRUE(rg_string_equals(s, s6));

    // Test equality with empty strings
    EXPECT_TRUE(rg_string_equals(RG_EMPTY_STRING, RG_EMPTY_STRING));
    EXPECT_FALSE(rg_string_equals(RG_EMPTY_STRING, s));
    EXPECT_FALSE(rg_string_equals(s, RG_EMPTY_STRING));
    EXPECT_TRUE(rg_string_equals(s7, RG_EMPTY_STRING));
    EXPECT_TRUE(rg_string_equals(RG_EMPTY_STRING, s7));

    // Test conversion to c string
    char *s_cstr = s.data;
    ASSERT_NOT_NULL(s_cstr);
    EXPECT_TRUE(strcmp(s_cstr, "Hello") == 0);
    EXPECT_TRUE(s_cstr[5] == '\0');

    char *empty_cstr = empty.data;
    ASSERT_NOT_NULL(empty_cstr);
    EXPECT_TRUE(strcmp(empty_cstr, "") == 0);
    EXPECT_TRUE(empty_cstr[0] == '\0');

    // Test find functions
    ssize_t i = rg_string_find_char(s, 'l');
    EXPECT_TRUE(i == 2);
    i = rg_string_find_char(s, 'p');
    EXPECT_TRUE(i == -1);
    i = rg_string_find_char_reverse(s, 'l');
    EXPECT_TRUE(i == 3);
    i = rg_string_find_char_reverse(s, 'p');
    EXPECT_TRUE(i == -1);
    i = rg_string_find_char(RG_EMPTY_STRING, 'a');
    EXPECT_TRUE(i == -1);
    i = rg_string_find_char_reverse(RG_EMPTY_STRING, 'a');
    EXPECT_TRUE(i == -1);

    // Test get char function
    EXPECT_TRUE(rg_string_get_char(s, 0) == 'H');
    EXPECT_TRUE(rg_string_get_char(s, 1) == 'e');
    EXPECT_TRUE(rg_string_get_char(s, 2) == 'l');
    EXPECT_TRUE(rg_string_get_char(s, 3) == 'l');
    EXPECT_TRUE(rg_string_get_char(s, 4) == 'o');
    EXPECT_TRUE(rg_string_get_char(s, 5) == -1);
    EXPECT_TRUE(rg_string_get_char(RG_EMPTY_STRING, 5) == -1);

    // Test substring
    rg_string substring = rg_string_get_substring(s4, 3, 6);
    ASSERT_NOT_NULL(substring.data);
    EXPECT_TRUE(substring.length == 4);
    EXPECT_TRUE(substring.data[0] == s4.data[3]);
    EXPECT_TRUE(substring.data[1] == s4.data[4]);
    EXPECT_TRUE(substring.data[2] == s4.data[5]);
    EXPECT_TRUE(substring.data[3] == s4.data[6]);
    EXPECT_FALSE(rg_string_is_empty(substring));

    // Substring of empty string
    substring = rg_string_get_substring(RG_EMPTY_STRING, 0, 4);
    ASSERT_NOT_NULL(substring.data);
    EXPECT_TRUE(substring.data[0] == '\0');
    EXPECT_TRUE(substring.length == 0);
    EXPECT_TRUE(rg_string_is_empty(substring));

    // Substring of 0 char
    substring = rg_string_get_substring(s4, 4, 4);
    ASSERT_NOT_NULL(substring.data);
    EXPECT_TRUE(substring.data[0] == '\0');
    EXPECT_TRUE(substring.length == 0);
    EXPECT_TRUE(rg_string_is_empty(substring));

    // Substring out of bounds
    substring = rg_string_get_substring(s4, 0, 789);
    ASSERT_NOT_NULL(substring.data);
    EXPECT_TRUE(substring.data[0] == '\0');
    EXPECT_TRUE(substring.length == 0);
    EXPECT_TRUE(rg_string_is_empty(substring));

    substring = rg_string_get_substring(s4, 78, 2);
    ASSERT_NOT_NULL(substring.data);
    EXPECT_TRUE(substring.data[0] == '\0');
    EXPECT_TRUE(substring.length == 0);
    EXPECT_TRUE(rg_string_is_empty(substring));

    // Start > end
    substring = rg_string_get_substring(s4, 5, 2);
    ASSERT_NOT_NULL(substring.data);
    EXPECT_TRUE(substring.data[0] == '\0');
    EXPECT_TRUE(substring.length == 0);
    EXPECT_TRUE(rg_string_is_empty(substring));

    // Test end function
    EXPECT_TRUE(rg_string_end(s) == 4);
    EXPECT_TRUE(rg_string_end(RG_EMPTY_STRING) == 0);

    // Clean up
    rg_free(s3.data);
    rg_free(s4.data);
    rg_free(s5.data);
    rg_free(s6.data);



}