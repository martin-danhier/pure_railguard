/**
 * @file Simple lightweight testing framework for C or C++ projects, inspired by Google Tests API.
 * @author Martin Danhier
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

// --- Macros ---

// First macro: add the counter macro
#define TEST(test_name) _TF_TEST1(test_name, __COUNTER__)
// Second: get the value of the counter
#define _TF_TEST1(test_name, id) _TF_TEST2(test_name, id)
// Third: generate the function definition and register it
#define _TF_TEST2(test_name, id)                                                                \
void                                  __test_##test_name##_##id(tf_context *___context___); \
__attribute__((__constructor__)) void __pretest_init_##test_name##_##id()                   \
{                                                                                           \
tf_register_test(#test_name, &__test_##test_name##_##id);                               \
}                                                                                           \
void __test_##test_name##_##id(tf_context *___context___)

// Macro to run all tests
#define RUN_ALL_TESTS() tf_main()

// Macros for assertions

#define ASSERT_TRUE(condition) tf_assert_true(___context___, __LINE__, __FILE__, (condition))
#define ASSERT_FALSE(condition) tf_assert_false(___context___, __LINE__, __FILE__, (condition))
#define ASSERT_NOT_NULL(pointer) tf_assert_not_null(___context___, __LINE__, __FILE__, (pointer))
#define ASSERT_NULL(pointer) tf_assert_null(___context___, __LINE__, __FILE__, (pointer))

// --- Types ---

typedef struct tf_context tf_context;

typedef void (*tf_test_function)(tf_context *);

// --- Functions ---

int tf_main(void);

void tf_register_test(const char *name, tf_test_function pfn_test);

void tf_assert_true(tf_context *context, size_t line_number, const char *file, bool condition);

void tf_assert_false(tf_context *context, size_t line_number, const char *file, bool condition);

void tf_assert_not_null(tf_context *context, size_t line_number, const char*file, void *pointer);

void tf_assert_null(tf_context *context, size_t line_number, const char*file, void *pointer);