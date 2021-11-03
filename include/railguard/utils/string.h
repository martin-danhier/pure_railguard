#pragma once

#include <stdbool.h>
#include <stddef.h>

// --=== Types ===--

/**
 * @brief A string is a sequence of characters with a fixed size.
 */
typedef struct rg_string
{
    char  *data;
    size_t length;
} rg_string;

// --=== Macros ===--

/** Creates a new rg_string from a C string. The length is computed at runtime. */
#define RG_CSTR(s) rg_create_string_from_cstr((s))
/** Creates a new rg_string from a const C string. The length is computed at compile time. */
#define RG_CSTR_CONST(s)                                                      \
    (rg_string)                                                               \
    {                                                                         \
        .data = sizeof(s) <= 1 ? NULL : (char *) (s), .length = sizeof(s) - 1 \
    }

#define RG_EMPTY_STRING RG_CSTR_CONST("")

#define CONCAT_IMPL(a, b) a ## b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define RG_MACRO_VAR(name) CONCAT(name, __LINE__)

/**
 * @brief Evaluate start, then the content of the block, then end
 * @code
 * DEFER(begin(), end()) {
 *     // Do something in between
 * }
 * @endcode
 */
#define DEFER(start, end)  for (int RG_MACRO_VAR(_i_) = ((start), 0); !RG_MACRO_VAR(_i_); (RG_MACRO_VAR(_i_) += 1, (end)))

/**
 * @brief Converts the string s to a C string v. After the end of the block, v is freed.
 * @warning Since it is a block macro, you shouldn't use it in a unbraced block.
 * @example Good:
 * @code
 * if (condition) {
 *     RG_AS_CSTR(s, v) {
 *         // Do something with v
 *     }
 * }
 * @endcode
 * @example Bad:
 * @code
 * if (condition) RG_AS_CSTR(s, v) {
 *     // Since v is declared before the block, the block will be outside of the if
 * }
 * @endcode
 */
#define RG_AS_CSTR(s, v) char* v = NULL; DEFER((v) = rg_string_to_cstr(s), free(v))

// --=== Functions ===--

/**
 * Creates a new rg_string from a C string. The length is computed at runtime.
 * @param cstr The C string to create the rg_string from.
 * @return The new rg_string.
 * */
rg_string rg_create_string_from_cstr(const char *cstr);

/**
 * Clones a rg_string
 * @param string The string to clone.
 * @return a new rg_string with the same data and length, but at a different memory location.
 */
rg_string rg_clone_string(rg_string string);

/**
 * Checks if the given rg_string is empty (equal to EMPTY_STRING).
 * @param string the string to check.
 * @return true if the string is empty, false otherwise.
 */
static inline bool rg_string_is_empty(rg_string string)
{
    return string.length == 0;
}

/**
 * Creates a new string containing the concatenation of a and b. a and b are not modified nor freed.
 * @param a a rg_string.
 * @param b another rg_string.
 * @return a new rg_string containing the concatenation of a and b.
 */
rg_string rg_string_concat(rg_string a, rg_string b);

/**
 * Check if two strings are equal.
 * @param a a rg_string.
 * @param b another rg_string.
 * @return true if a and b are equal, false otherwise.
 */
bool rg_string_equals(rg_string a, rg_string b);

/**
 * Convert a rg_string to a C string (null terminated).
 * @param string the rg_string to convert.
 * @return a C string equivalent to the rg_string, or NULL if there is an error.
 * @warning The returned string must be freed.
 */
char *rg_string_to_cstr(rg_string string);

/**
 * Find the first occurrence of a character in a rg_string.
 * @param string the rg_string to search in.
 * @param c the character to find.
 * @return the index of the first occurrence of c in string, or -1 if c is not found.
 */
ssize_t rg_string_find_char(rg_string string, char c);

/**
 * Find the last occurrence of a character in a rg_string.
 * @param string the rg_string to search in.
 * @param c the character to find.
 * @return the index of the last occurrence of c in string, or -1 if c is not found.
 */
ssize_t rg_string_find_char_reverse(rg_string string, char c);

/**
 * Gets the character at the given index in a rg_string.
 * @param string the rg_string to get the character from.
 * @param index the index of the character to get.
 * @return the character at the given index, or -1 if the index is out of bounds.
 */
static inline char rg_string_get_char(rg_string string, size_t index)
{
    if (index >= 0 && index < string.length)
    {
        return string.data[index];
    }

    return -1;
}

/**
 * Gets a substring of a rg_string.
 * @param string the rg_string to get the substring from.
 * @param start index of the first character of the substring.
 * @param end index of the last character of the substring.
 * @return a new rg_string containing the substring, or RG_EMPTY_STRING if the indices are out of bounds.
 * @warning The substring points to the same memory as the original rg_string, so it must not be freed.
 */
rg_string rg_string_get_substring(rg_string string, size_t start, size_t end);

/**
 * Returns the index of the last char of a string.
 * @param string the rg_string to get the last char from.
 * @return the index of the last char of the string, or 0 if the string is empty.
 */
static inline size_t rg_string_end(rg_string string)
{
    if (string.length == 0)
    {
        return 0;
    }

    return string.length - 1;
}