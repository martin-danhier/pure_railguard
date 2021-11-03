#include "railguard/utils/string.h"

#include <stdlib.h>
#include <string.h>

rg_string rg_create_string_from_cstr(const char *cstr)
{
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
    // Do not malloc 0 bytes of memory in case of empty string
    if (rg_string_is_empty(string))
    {
        return RG_EMPTY_STRING;
    }

    // Copy the string into a new string
    rg_string clone = {
        .data   = (char *) malloc(string.length),
        .length = string.length,
    };
    if (clone.data == NULL)
    {
        return RG_EMPTY_STRING;
    }

    // Copy the string
    void *new_data = memcpy(clone.data, string.data, string.length);
    if (new_data == NULL)
    {
        free(clone.data);
        return RG_EMPTY_STRING;
    }

    return clone;
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
        .data   = (char *) malloc(new_length),
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

    // If there was an error, free the memory and return an empty string
    if (!ok)
    {
        free(new_string.data);
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

char *rg_string_to_cstr(rg_string string)
{
    // Even if the string is empty, we need to allocate memory
    // This is because a c string will always have a null terminator
    char *cstr = (char *) malloc(string.length + 1);
    if (cstr == NULL)
    {
        return NULL;
    }

    // Copy the string
    if (rg_string_is_empty(string))
    {
        cstr[0] = '\0';
    }
    else
    {
        memcpy(cstr, string.data, string.length);
        cstr[string.length] = '\0';
    }

    return cstr;
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
