#include "framework/test_framework.h"
#include <railguard/utils/arrays.h>

TEST(array) {
    ASSERT_TRUE(false);
}

// Entry point for the tests
int main(void)
{
    tf_context *context = tf_create_context();
    test_array(context);


    return 0;
}