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

#define TEST(test_name) void test_##test_name(tf_context *___context___); void test_##test_name(tf_context *___context___)

// Types

typedef struct tf_test_manager {

} tf_test_manager;

typedef enum tf_error_severity
{
    ERROR,
    WARNING
} tf_error_severity;

typedef struct tf_error
{
    struct tf_error  *next;
    tf_error_severity severity;
    size_t            line_number;
    const char       *message;
    const char       *file;
    const char             *expected;
    bool              expected_is_dynamic;
    const char             *actual;
    bool              actual_is_dynamic;
} tf_error;

typedef struct tf_linked_list_element {
    struct tf_linked_list_element *next;
    void *data;
} tf_linked_list_element;

typedef struct tf_error_list
{
    tf_error *head;
    tf_error *tail;
    size_t    count;
} tf_error_list;

typedef struct tf_error_list_it
{
    tf_error *current;
    size_t    index;
} tf_error_list_it;

typedef struct tf_context
{
    tf_error_list errors;
} tf_context;

// Globals




// Types functions

void tf_run_

void tf_error_list_push(tf_error_list *error_list, tf_error *error)
{
    // Allocate error
    tf_error *created_error = calloc(1, sizeof(tf_error));
    created_error           = memcpy(created_error, error, sizeof(tf_error));
    if (created_error == NULL)
    {
        fprintf(stderr, "Test Framework: an error occurred while creating an error.");
        abort();
    }
    created_error->next = NULL;

    // Empty: add it to head
    if (error_list->count == 0)
    {
        error_list->head = created_error;
    }
    // Not empty: add it in the tail's next field
    else
    {
        error_list->tail->next = created_error;
    }

    error_list->tail = created_error;
    error_list->count++;
}

tf_error_list_it tf_error_list_iterator(tf_error_list *error_list)
{
    return (tf_error_list_it) {
        .current = error_list->head,
        .index   = -1,
    };
}

bool tf_error_list_next(tf_error_list_it *it)
{
    if (it->current != NULL)
    {
        it->index++;

        // For the index 0, we want to return the head, so don't go to the next one yet
        if (it->index > 0)
        {
            it->current = it->current->next;
        }

        return true;
    }

    return false;
}

void tf_error_list_clear(tf_error_list *error_list)
{
    // Free values in linked list
    tf_error *current = error_list->head;
    while (current != NULL)
    {
        tf_error *next = current->next;
        free(current);
        current = next;
    }

    // Clear the error list itself
    error_list->head  = NULL;
    error_list->tail  = NULL;
    error_list->count = 0;
}

void tf_context_add_error(tf_context *context, tf_error *error)
{
    tf_error_list_push(&context->errors, error);
}

tf_context *tf_create_context(void)
{
    tf_context *context = calloc(1, sizeof(tf_context));
    return context;
}

void tf_delete_context(tf_context *context)
{
    tf_error_list_clear(&context->errors);
    free(context);
}

// Asserts

void tf_assert_common(tf_context *context,
                      size_t      line_number,
                      const char *file,
                      bool        condition,
                      const char *actual,
                      bool        actual_is_dynamic,
                      const char *expected,
                      bool        expected_is_dynamic)
{
    if (condition != true)
    {
        // Save error in context
        tf_error error = {
            .severity            = ERROR,
            .line_number         = line_number,
            .file                = file,
            .message             = "Assertion failed",
            .actual              = actual,
            .actual_is_dynamic   = actual_is_dynamic,
            .expected            = expected,
            .expected_is_dynamic = expected_is_dynamic,
        };
        tf_context_add_error(context, &error);
    }
}

#define ASSERT_TRUE(condition) tf_assert_true(___context___, __LINE__, __FILE__, (condition))
void tf_assert_true(tf_context *context, size_t line_number, const char *file, bool condition)
{
    tf_assert_common(context, line_number, file, condition, "false", false, "true", false);
}

#define ASSERT_FALSE(condition) tf_assert_false(___context___, __LINE__, __FILE__, (condition))
void tf_assert_false(tf_context *context, size_t line_number, const char *file, bool condition)
{
    tf_assert_common(context, line_number, file, !condition, "true", false, "false", false);
}