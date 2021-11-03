#include "framework/test_framework.h"
// Import the test files
// The editor says that they are unused, but they are actually used by the RUN_ALL_TESTS macro
#include "utils/test_hash_map.h"
#include "utils/test_struct_map.h"
#include "utils/test_vector.h"
#include "utils/test_io.h"
#include "utils/test_storage.h"
#include "utils/test_event_sender.h"
#include "core/window.h"
#include "utils/test_string.h"

// Entry point for the tests
int main(void)
{
    return RUN_ALL_TESTS();
}
