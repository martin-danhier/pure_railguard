#include "framework/test_framework.h"
#include <railguard/utils/memory.h>

// Import the test files
// The editor says that they are unused, but they are actually used by the RUN_ALL_TESTS macro
#include "utils/test_hash_map.h"
#include "utils/test_struct_map.h"
#include "utils/test_vector.h"
#include "utils/test_io.h"
#include "utils/test_storage.h"
#include "utils/test_event_sender.h"
#include "utils/test_string.h"
#include "utils/math.h"
#include "core/window.h"
#include "core/renderer.h"

// Entry point for the tests
int main(int argc, char** argv)
{
    int result;

#ifdef MEMORY_CHECKS
    // Initialize the memory watcher
    rg_mem_watcher_init();
#endif

    // If there is a test name in the arguments, run only that test
    if (argc > 1)
    {
        // Run the test
        result = RUN_TEST(argv[1]);
    }
    else
    {
        // Run all the tests
        result = RUN_ALL_TESTS();
    }
#ifdef MEMORY_CHECKS

    // Print the results of the memory watcher
    if (!rg_mem_watcher_print_leaks()) {
        result = 1;
    }


    // Cleanup the memory watcher
    rg_mem_watcher_cleanup();
#endif

    return result;
}
