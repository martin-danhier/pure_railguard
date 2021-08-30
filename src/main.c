#include <railguard/rendering/renderer.h>
#include <railguard/rendering/window.h>

int main()
{
    // Start window manager
    rg_start_window_manager();

    // Create window
    rg_window *window = rg_create_window((rg_extent_2d) {500, 500}, "My wonderful game");

    // Create renderer
    rg_renderer *renderer = rg_create_renderer(window, "My wonderful game", (rg_version) {0, 1, 0});

    // Cleanup
    rg_destroy_renderer(&renderer);
    rg_destroy_window(&window);

    // Stop window manager
    rg_stop_window_manager();
    return 0;
}