#include <railguard/rendering/renderer.h>
#include <railguard/rendering/window.h>

int main() {
    // Start window manager
    rg_start_window_manager();

    // Create window
    rg_extent_2d window_extent = {500, 500};
    rg_window *window = rg_create_window(window_extent, "My wonderful game");

    // Create renderer
    rg_renderer *renderer = rg_create_renderer(window, "My wonderful game", (rg_version) {0, 1, 0}, 1);

    // Create swapchain for window
    rg_surface surface = rg_get_window_surface(renderer, window);
    rg_renderer_create_swapchain(renderer, 0, surface, window_extent);


    // Cleanup
    rg_destroy_renderer(&renderer);
    rg_destroy_window(&window);

    // Stop window manager
    rg_stop_window_manager();
    return 0;
}