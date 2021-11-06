#pragma once

// --==== Types ====--

#include <stdbool.h>
#include <stdint.h>
typedef struct rg_extent_2d
{
    unsigned int width;
    unsigned int height;
} rg_extent_2d;

typedef struct rg_window rg_window;
typedef struct rg_event_handler rg_event_handler;
typedef uint32_t rg_event_handler_id;

// Event data
typedef struct rg_window_resize_event_data {
    rg_extent_2d new_extent;
} rg_window_resize_event_data;

// Also forward declare to avoid includes
typedef struct rg_array rg_array;

// --==== Window Manager ====--

void rg_start_window_manager();
void rg_stop_window_manager();

// --==== Window ====--

rg_window *rg_create_window(rg_extent_2d extent, const char *title);
void       rg_destroy_window(rg_window **window);

/**
 * Updates the frame time counter and computes the delta time.
 * @param window The window in use.
 * @param current_frame_time counter used by the window to track the time elapsed since last frame. Mutated by a call to this function.
 * @warning The frame time is not necessarily in a time unit. Avoid using it outside of a call to this function.
 * @return The delta time, which is the time elapsed since last call to this function, in seconds.
 */
double rg_window_compute_delta_time(rg_window *window, uint64_t *current_frame_time);

bool rg_window_handle_events(rg_window *window);

// Getters
rg_extent_2d rg_window_get_current_extent(rg_window *window);

// Events
rg_event_handler_id rg_window_resize_event_subscribe(rg_window *window, rg_event_handler handler);
void rg_window_resize_event_unsubscribe(rg_window *window, rg_event_handler_id handler_id);

#ifdef RENDERER_VULKAN
rg_array rg_window_get_required_vulkan_extensions(rg_window *window, unsigned int extra_array_size);

typedef struct VkInstance_T   *VkInstance;
typedef struct VkSurfaceKHR_T *VkSurfaceKHR;
VkSurfaceKHR rg_window_get_vulkan_surface(rg_window *window, VkInstance vulkan_instance);
#endif