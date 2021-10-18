#include <railguard/core/engine.h>

int main(void)
{
    // Init the engine
    rg_engine *engine = rg_create_engine();

    // Main loop
    rg_engine_run_main_loop(engine);

    // Cleanup
    rg_destroy_engine(&engine);

    return 0;
}