/**
 * @file Simple custom testing framework for C or C++ projects, inspired by Google Tests API.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macros

#define TF_FORMAT_BOLD_RED   "\033[31;1m"
#define TF_FORMAT_BOLD_GREEN "\033[32;1m"
#define TF_FORMAT_RED        "\033[31m"
#define TF_FORMAT_YELLOW     "\033[93m"
#define TF_FORMAT_BOLD       "\033[1m"
#define TF_FORMAT_RESET      "\033[0m"

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
#define RUN_ALL_TESTS()                        \
    do                                         \
    {                                          \
        tf_manager_run_all_tests(&TF_MANAGER); \
                                               \
    } while (0);

// Types

typedef enum tf_error_severity
{
    ERROR,
    WARNING
} tf_error_severity;

typedef struct tf_error
{
    tf_error_severity severity;
    size_t            line_number;
    const char       *message;
    const char       *file;
} tf_error;

typedef struct tf_linked_list_element
{
    struct tf_linked_list_element *next;
    void                          *data;
} tf_linked_list_element;

typedef struct tf_linked_list
{
    tf_linked_list_element *head;
    tf_linked_list_element *tail;
    size_t                  count;
} tf_linked_list;

typedef struct tf_linked_list_it
{
    tf_linked_list_element *current;
    size_t                  index;
    void                   *value;
} tf_linked_list_it;

typedef struct tf_context
{
    tf_linked_list errors;
} tf_context;

typedef void (*tf_test_function)(tf_context *);

typedef struct tf_test
{
    const char      *name;
    tf_context      *context;
    tf_test_function pfn_test;
} tf_test;

typedef struct tf_test_manager
{
    tf_linked_list registered_tests;
} tf_test_manager;

// Types functions

void tf_linked_list_push(tf_linked_list *linked_list, void *value, size_t value_size)
{
    // Allocate element
    void                   *value_alloc = NULL;
    tf_linked_list_element *element     = calloc(1, sizeof(tf_linked_list_element));
    bool                    success     = element != NULL;

    // Allocate value
    if (success)
    {
        value_alloc = calloc(1, value_size);
        success     = value_alloc != NULL;
    }

    // Copy value
    if (success)
    {
        success = memcpy(value_alloc, value, value_size) != NULL;
    }

    // Handle any error above
    if (!success)
    {
        fprintf(stderr, "Test Framework: an error occurred while creating an error.");
        abort();
    }

    // Save data to element
    element->data = value_alloc;

    // Empty: add it to head
    if (linked_list->count == 0)
    {
        linked_list->head = element;
    }
    // Not empty: add it in the tail's next field
    else
    {
        linked_list->tail->next = element;
    }

    linked_list->tail = element;
    linked_list->count++;
}

tf_linked_list_it tf_linked_list_iterator(tf_linked_list *linked_list)
{
    return (tf_linked_list_it) {
        .current = linked_list->head,
        .value   = NULL,
        .index   = -1,
    };
}

bool tf_error_list_next(tf_linked_list_it *it)
{
    if (it->current != NULL)
    {
        it->index++;

        // For the index 0, we want to return the head, so don't go to the next one yet
        if (it->index > 0)
        {
            it->current = it->current->next;
        }

        // Get value
        if (it->current != NULL)
        {
            it->value = it->current->data;
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}

void tf_linked_list_clear(tf_linked_list *linked_list)
{
    // Free values in linked list
    tf_linked_list_element *current = linked_list->head;
    while (current != NULL)
    {
        tf_linked_list_element *next = current->next;
        if (current->data != NULL)
        {
            free(current->data);
        }
        free(current);
        current = next;
    }

    // Clear the error list itself
    linked_list->head  = NULL;
    linked_list->tail  = NULL;
    linked_list->count = 0;
}

void tf_context_add_error(tf_context *context, tf_error *error)
{
    tf_linked_list_push(&context->errors, error, sizeof(tf_error));
}

tf_context *tf_create_context(void)
{
    tf_context *context = calloc(1, sizeof(tf_context));
    return context;
}

void tf_delete_context(tf_context *context)
{
    tf_linked_list_clear(&context->errors);
    free(context);
}

bool tf_manager_run_all_tests(tf_test_manager *manager)
{
    size_t failed_counter = 0;

    printf("Starting testing for %llu tests...\n\n", manager->registered_tests.count);

    // For each test
    tf_linked_list_it it = tf_linked_list_iterator(&manager->registered_tests);
    while (tf_error_list_next(&it))
    {
        tf_test *current_test = it.value;

        // Print progress
        printf("%s[Test %llu/%llu] \"%s\"%s",
               TF_FORMAT_BOLD,
               it.index + 1,
               manager->registered_tests.count,
               current_test->name,
               TF_FORMAT_RESET);

        // Init its context
        current_test->context = tf_create_context();
        // Run the test
        current_test->pfn_test(current_test->context);

        // Print result
        if (current_test->context->errors.count == 0)
        {
            printf("\t ---> %sPASSED%s\n", TF_FORMAT_BOLD_GREEN, TF_FORMAT_RESET);
        }
        else
        {
            printf("\t ---> %sFAILED%s\n", TF_FORMAT_BOLD_RED, TF_FORMAT_RESET);
            failed_counter++;
        }

        // Print errors if there is any
        tf_linked_list_it err_it = tf_linked_list_iterator(&current_test->context->errors);
        while (tf_error_list_next(&err_it))
        {
            tf_error *current_error = err_it.value;

            // Convert the severity to string
            const char *severity_str;
            switch (current_error->severity)
            {
                case ERROR: severity_str = TF_FORMAT_RED "Error" TF_FORMAT_RESET; break;
                case WARNING: severity_str = TF_FORMAT_YELLOW "Warning" TF_FORMAT_RESET; break;
            }

            printf("\t- [%s] %s:%llu\n\t%s\n", severity_str, current_error->file, current_error->line_number, current_error->message);
        }
    }

    // Print summary
    if (failed_counter == 0)
    {
        printf("\n%s-> All test passed successfully.%s\n", TF_FORMAT_BOLD_GREEN, TF_FORMAT_RESET);
        return true;
    }
    else
    {
        printf("\n%s-> %llu/%llu tests failed. See the errors above.%s\n",
               TF_FORMAT_BOLD_RED,
               failed_counter,
               manager->registered_tests.count,
               TF_FORMAT_RESET);
        return false;
    }
}

// Asserts

void tf_assert_common(tf_context *context, size_t line_number, const char *file, bool condition, const char *message)
{
    if (condition != true)
    {
        // Save error in context
        tf_error error = {
            .severity    = ERROR,
            .line_number = line_number,
            .file        = file,
            .message     = message,
        };
        tf_context_add_error(context, &error);
    }
}

#define ASSERT_TRUE(condition) tf_assert_true(___context___, __LINE__, __FILE__, (condition))
void tf_assert_true(tf_context *context, size_t line_number, const char *file, bool condition)
{
    tf_assert_common(context, line_number, file, condition, "Assertion failed. Expected [true], got [false].");
}

#define ASSERT_FALSE(condition) tf_assert_false(___context___, __LINE__, __FILE__, (condition))
void tf_assert_false(tf_context *context, size_t line_number, const char *file, bool condition)
{
    tf_assert_common(context, line_number, file, !condition, "Assertion failed. Expected [false], got [true].");
}

// Global

tf_test_manager TF_MANAGER = {};

void tf_register_test(const char *name, tf_test_function pfn_test)
{
    tf_test test = {
        .pfn_test = pfn_test,
        .context  = NULL,
        .name     = name,
    };
    tf_linked_list_push(&TF_MANAGER.registered_tests, &test, sizeof(tf_test));
}