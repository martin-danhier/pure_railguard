#include <railguard/core/engine.h>
#include <railguard/utils/memory.h>

int main(void)
{
    int result = 0;

#ifdef MEMORY_CHECKS
    // Initialize the memory watcher
    rg_mem_watcher_init();
#endif

    // Init the engine
    rg_engine *engine = rg_create_engine();

    // Main loop
    rg_engine_run_main_loop(engine);

    // Cleanup
    rg_destroy_engine(&engine);

#ifdef MEMORY_CHECKS
    // Print the results of the memory watcher
    if (!rg_mem_watcher_print_leaks())
    {
        result = 1;
    }

    // Cleanup the memory watcher
    rg_mem_watcher_cleanup();
#endif

    return result;
}