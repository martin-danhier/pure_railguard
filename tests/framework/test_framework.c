/**
 * @file Simple lightweight testing framework for C or C++ projects, inspired by Google Tests API.
 * @author Martin Danhier
 */

#include "test_framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN64
#include <windows.h>
#endif

// Macros

// On Windows, without Windows Terminal, ANSI color codes are not supported
#ifdef WIN64

// On Windows, we need to store the previous color in order to reset it
HANDLE TF_CONSOLE_HANDLE          = 0;
WORD   TF_DEFAULT_COLOR_ATTRIBUTE = 0;

// Inits the stored variables
#define TF_INIT_FORMATTING                                                        \
    do                                                                            \
    {                                                                             \
        TF_CONSOLE_HANDLE                      = GetStdHandle(STD_OUTPUT_HANDLE); \
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo = {};                              \
        GetConsoleScreenBufferInfo(TF_CONSOLE_HANDLE, &consoleInfo);              \
        TF_DEFAULT_COLOR_ATTRIBUTE = consoleInfo.wAttributes;                     \
    } while (0)

// Applies the various colors and settings
#define TF_FORMAT_BOLD_RED   SetConsoleTextAttribute(TF_CONSOLE_HANDLE, 0xC)
#define TF_FORMAT_BOLD_GREEN SetConsoleTextAttribute(TF_CONSOLE_HANDLE, 0xA)
#define TF_FORMAT_RED        SetConsoleTextAttribute(TF_CONSOLE_HANDLE, 0x4)
#define TF_FORMAT_YELLOW     SetConsoleTextAttribute(TF_CONSOLE_HANDLE, 0x6)
#define TF_FORMAT_BOLD       SetConsoleTextAttribute(TF_CONSOLE_HANDLE, 0xF)
#define TF_FORMAT_RESET      SetConsoleTextAttribute(TF_CONSOLE_HANDLE, TF_DEFAULT_COLOR_ATTRIBUTE)

#else

// On civilized terminals, we just need to print an ANSI color code.
#define TF_INIT_FORMATTING
#define TF_FORMAT_BOLD_RED   printf("\033[31;1m")
#define TF_FORMAT_BOLD_GREEN printf("\033[32;1m")
#define TF_FORMAT_RED        printf("\033[31m")
#define TF_FORMAT_YELLOW     printf("\033[93m")
#define TF_FORMAT_BOLD       printf("\033[1m")
#define TF_FORMAT_RESET      printf("\033[0m")
#endif

// Types

typedef enum tf_error_severity
{
    TF_ERROR,
    TF_WARNING
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

bool tf_linked_list_next(tf_linked_list_it *it)
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

    // Determine the maximum length of the test name, to align the print
    size_t max_length = 0;
    tf_linked_list_it it = tf_linked_list_iterator(&manager->registered_tests);
    while (tf_linked_list_next(&it))
    {
        tf_test *test = it.value;
        size_t  length = strlen(test->name);
        if (length > max_length)
        {
            max_length = length;
        }
    }

    // For each test
    it = tf_linked_list_iterator(&manager->registered_tests);
    while (tf_linked_list_next(&it))
    {
        tf_test *current_test = it.value;

        // Print progress
        TF_FORMAT_BOLD;
        printf("[Test %llu/%llu] \"%s\"", it.index + 1, manager->registered_tests.count, current_test->name);
        TF_FORMAT_RESET;

        // Init its context
        current_test->context = tf_create_context();
        // Run the test
        current_test->pfn_test(current_test->context);

        // Print result
        // Add spaces to align the test name
        for (size_t i = 0; i < max_length - strlen(current_test->name); i++)
        {
            printf(" ");
        }

        printf("     ---> ");
        if (current_test->context->errors.count == 0)
        {
            TF_FORMAT_BOLD_GREEN;
            printf("PASSED");
        }
        else
        {
            TF_FORMAT_BOLD_RED;
            printf("FAILED");
            failed_counter++;
        }
        TF_FORMAT_RESET;
        printf("\n");

        // Print errors if there is any
        tf_linked_list_it err_it = tf_linked_list_iterator(&current_test->context->errors);
        while (tf_linked_list_next(&err_it))
        {
            tf_error *current_error = err_it.value;

            // Convert the severity to string
            printf("\t- [");
            switch (current_error->severity)
            {
                case TF_ERROR:
                    TF_FORMAT_RED;
                    printf("Error");
                    break;
                case TF_WARNING:
                    TF_FORMAT_YELLOW;
                    printf("Warning");
                    break;
            }
            TF_FORMAT_RESET;

            printf("] %s:%llu\n\t%s\n", current_error->file, current_error->line_number, current_error->message);
        }
    }

    // Print summary
    if (failed_counter == 0)
    {
        TF_FORMAT_BOLD_GREEN;
        printf("\n-> All tests passed successfully.\n");
        TF_FORMAT_RESET;
        return true;
    }
    else
    {
        TF_FORMAT_BOLD_RED;
        printf("\n-> %llu/%llu tests failed. See the errors above.\n", failed_counter, manager->registered_tests.count);
        TF_FORMAT_RESET;
        return false;
    }
}

void tf_clear_manager(tf_test_manager *manager)
{
    // Destroy the contexts
    tf_linked_list_it it = tf_linked_list_iterator(&manager->registered_tests);
    while (tf_linked_list_next(&it))
    {
        tf_test *current_test = it.value;
        tf_delete_context(current_test->context);
    }

    // Clear the list
    tf_linked_list_clear(&manager->registered_tests);
}

// Asserts

bool tf_assert_common(tf_context *context, size_t line_number, const char *file, bool condition, const char *message, bool recoverable)
{
    if (condition != true)
    {
        // Save error in context
        tf_error error = {
            .severity    = TF_ERROR,
            .line_number = line_number,
            .file        = file,
            .message     = message,
        };
        tf_context_add_error(context, &error);

        // Return a bool telling it the execution can continue
        return recoverable;
    }
    return true;
}

bool tf_assert_true(tf_context *context, size_t line_number, const char *file, bool condition, bool recoverable)
{
    return tf_assert_common(context,
                            line_number,
                            file,
                            condition,
                            recoverable ? "Condition failed. Expected [true], got [false]."
                                        : "Assertion failed. Expected [true], got [false]. Unable to continue execution.",
                            recoverable);
}

bool tf_assert_false(tf_context *context, size_t line_number, const char *file, bool condition, bool recoverable)
{
    return tf_assert_common(context,
                            line_number,
                            file,
                            !condition,
                            recoverable ? "Condition failed. Expected [true], got [false]."
                                        : "Assertion failed. Expected [true], got [false]. Unable to continue execution.",
                            recoverable);
}

bool tf_assert_not_null(tf_context *context, size_t line_number, const char *file, void *pointer, bool recoverable)
{
    return tf_assert_common(context,
                            line_number,
                            file,
                            pointer != NULL,
                            recoverable ? "Condition failed. Got [NULL], expected something else."
                                        : "Assertion failed. Got [NULL], expected something else. Unable to continue execution.",
                            recoverable);
}

bool tf_assert_null(tf_context *context, size_t line_number, const char *file, void *pointer, bool recoverable)
{
    return tf_assert_common(context,
                            line_number,
                            file,
                            pointer == NULL,
                            recoverable ? "Condition failed. Got [NULL], expected something else."
                                        : "Assertion failed. Got [NULL], expected something else. Unable to continue execution.",
                            recoverable);
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

int tf_main(void)
{
    TF_INIT_FORMATTING;

    bool result = tf_manager_run_all_tests(&TF_MANAGER);
    tf_clear_manager(&TF_MANAGER);

    // Return an error if at least one test failed
    if (!result)
    {
        return 1;
    }
    return 0;
}