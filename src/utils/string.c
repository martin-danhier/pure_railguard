#include "railguard/utils/string.h"

#include <railguard/utils/memory.h>

#include <stdlib.h>
#include <string.h>

rg_string rg_create_string_from_cstr(const char *cstr)
{
    if (cstr == NULL)
    {
        return RG_EMPTY_STRING;
    }

    // Count the length of the string
    size_t len = 0;
    while (cstr[len] != '\0')
    {
        len++;
    }

    if (len == 0)
    {
        return RG_EMPTY_STRING;
    }

    return (rg_string) {
        .data   = (char *) cstr,
        .length = len,
    };
}

rg_string rg_clone_string(const rg_string string)
{
    // Empty string: no need to malloc
    if (rg_string_is_empty(string))
    {
        return RG_EMPTY_STRING;
    }

    // Copy the string into a new string
    rg_string clone = {
        .data   = (char *) rg_malloc(string.length + 1),
        .length = string.length,
    };
    if (clone.data == NULL)
    {
        return RG_EMPTY_STRING;
    }

    // Copy the string
    void *new_data = memcpy(clone.data, string.data, string.length + 1);
    if (new_data == NULL)
    {
        rg_free(clone.data);
        return RG_EMPTY_STRING;
    }

    return clone;
}

rg_string rg_create_string_from_buffer(void *buffer, size_t length)
{
    if (buffer == NULL || length == 0)
    {
        return RG_EMPTY_STRING;
    }

    // Add a null terminator
    char *new_data = (char *) rg_malloc(length + 1);
    if (new_data == NULL)
    {
        return RG_EMPTY_STRING;
    }

    // Copy the string
    void *new_data_copy = memcpy(new_data, buffer, length);
    if (new_data_copy == NULL)
    {
        rg_free(new_data);
        return RG_EMPTY_STRING;
    }

    // Add the null terminator
    new_data[length] = '\0';

    return (rg_string) {
        .data   = new_data,
        .length = length,
    };
}

rg_string rg_string_concat(rg_string a, rg_string b)
{
    // Do not malloc 0 bytes of memory in case of empty strings
    if (rg_string_is_empty(a) && rg_string_is_empty(b))
    {
        return RG_EMPTY_STRING;
    }

    // Allocate memory for the new string. It will be greater than 0 since we know that they are not both empty.
    size_t    new_length = a.length + b.length;
    rg_string new_string = {
        .data   = (char *) rg_malloc(new_length + 1),
        .length = new_length,
    };
    if (new_string.data == NULL)
    {
        return RG_EMPTY_STRING;
    }

    // Copy the strings
    bool ok = true;
    if (!rg_string_is_empty(a))
    {
        ok = memcpy(new_string.data, a.data, a.length) != NULL;
    }
    if (ok && !rg_string_is_empty(b))
    {
        ok = memcpy(new_string.data + a.length, b.data, b.length) != NULL;
    }

    // Add the null terminator
    if (ok)
    {
        new_string.data[new_length] = '\0';
    }
    // If there was an error, free the memory and return an empty string
    else
    {
        rg_free(new_string.data);
        return RG_EMPTY_STRING;
    }

    return new_string;
}

bool rg_string_equals(rg_string a, rg_string b)
{
    // Two empty strings are equal
    if (rg_string_is_empty(a) && rg_string_is_empty(b))
    {
        return true;
    }

    // Two equal strings must have the same length
    if (a.length != b.length)
    {
        return false;
    }

    // Compare the contents of the strings
    return memcmp(a.data, b.data, a.length) == 0;
}

ssize_t rg_string_find_char(rg_string string, char c)
{
    if (rg_string_is_empty(string)) {
        return -1;
    }

    // Find the first occurrence of the character
    for (ssize_t i = 0; i < string.length; i++)
    {
        if (string.data[i] == c)
        {
            return i;
        }
    }

    // Not found
    return -1;
}

ssize_t rg_string_find_char_reverse(rg_string string, char c)
{
    if (rg_string_is_empty(string)) {
        return -1;
    }

    // Find the last occurrence of the character
    for (ssize_t i = (ssize_t) string.length; i > 0; i--)
    {
        if (string.data[i - 1] == c)
        {
            return i - 1;
        }
    }

    // Not found
    return -1;
}

rg_string rg_string_get_substring(rg_string string, size_t start, size_t end)
{
    if (start >= 0 && end < string.length && start < end)
    {
        size_t length = end - start + 1;
        rg_string substring = {
            .data   = string.data + start,
            .length = length,
        };

        return substring;
    }

    return RG_EMPTY_STRING;
}

rg_array rg_string_array_to_cstr_array(rg_string *strings, size_t length)
{
    // Allocate memory for the array
    rg_array array = rg_create_array(length, sizeof(char *));


    // Copy the strings into the array
    for (size_t i = 0; i < length; i++)
    {
        ((char**) array.data)[i] = strings[i].data;
    }

    return array;
}

rg_array rg_string_array_from_cstr_array(const char * const *cstrs, size_t length)
{
    // Create new array
    rg_array array = rg_create_array(length, sizeof(rg_string));

    // Copy the strings into array.data
    for (size_t i = 0; i < length; i++)
    {
        ((rg_string *) array.data)[i] = RG_CSTR(cstrs[i]);
    }

    return array;
}

