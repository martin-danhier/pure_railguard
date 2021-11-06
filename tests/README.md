# Tests

In this directory, various tests are defined for the railguard library.

They allow:
- to detect errors
- to test previous errors that were fixed to see if they do not reappear
- to give examples of how to use the library

They can be run on GitHub Actions.

## How to run the tests

Simply run the ``unit_tests`` target in CMake.

## How to create a test

The tests use a minimalist custom testing framework inspired by Google Test.

To create a test, simply create a new header file in one of the test directory,
and start with the following:

```c
#pragma once

#include "../framework/test_framework.h"

TEST(<test_name>) {
    // Test code
}
```

Then, add an import to the test file in the [main.c](main.c) file.

It will automatically be added to the test list.

Inside a test, you can use various macros. They exist in two categories:
- **Assertions**: check if a condition is true. If it is false, do not continue the execution.
- **Expectations**: check if a condition is true. If it is false, continue the execution.

Typically, use expectations everywhere, except if a false condition could imply segmentation faults later.
For example, if a pointer is followed in the test, assert that the pointer is not null beforehand.

The macros are the following:
- ``ASSERT_TRUE(condition)``: check if a condition is true.
- ``ASSERT_FALSE(condition)``: check if a condition is false.
- ``ASSERT_NULL(pointer)``: check if a pointer is null.
- ``ASSERT_NOT_NULL(pointer)``: check if a pointer is not null.

The equivalent exists for expectations, for example ``EXPECT_TRUE``.

## How to check if we are in a test from the library

``UNIT_TESTS`` is defined when in test mode.