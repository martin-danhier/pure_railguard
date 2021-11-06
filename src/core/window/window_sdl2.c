#ifdef WINDOW_SDL2
// To prevent linking bugs, we handle the main wrapper of SDL ourselves.
// We define this constant to let it know
#define SDL_MAIN_HANDLED

#include "railguard/core/window.h"
#include <railguard/utils/arrays.h>

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef RENDERER_VULKAN
#include <railguard/utils/event_sender.h>
#include <railguard/utils/string.h>

#include <SDL2/SDL_vulkan.h>
#endif

// --==== TYPES ====--
typedef struct rg_window
{
    SDL_Window      *sdl_window;
    rg_extent_2d     extent;
    rg_event_sender *resize_event;
} rg_window;

// --==== UTILS FUNCTIONS ====--

void sdl_check(SDL_bool result)
{
    if (result != SDL_TRUE)
    {
        fprintf(stderr, "[SDL Error] Got SDL_FALSE !\n");
        exit(1);
    }
}

// --==== WINDOW MANAGER ====--

void rg_start_window_manager()
{
    // Init SDL2
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);
}

void rg_stop_window_manager()
{
    // Clean SDL
    SDL_Quit();
}

// --==== WINDOW ====--

rg_window *rg_create_window(rg_extent_2d extent, const char *title)
{
    // Initialize window
    rg_window *window = malloc(sizeof(rg_window));
    if (window == NULL)
    {
        return NULL;
    }

    // Init event senders
    window->resize_event = rg_create_event_sender();
    if (window->resize_event == NULL)
    {
        free(window);
        return NULL;
    }

    // Save other info
    window->extent = extent;

    // Init SDL2
    SDL_WindowFlags windowFlags;

    // We need to know which renderer to use
#ifdef RENDERER_VULKAN
    windowFlags = SDL_WINDOW_VULKAN;
#endif

    window->sdl_window = SDL_CreateWindow(title,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          (int32_t) extent.width,
                                          (int32_t) extent.height,
                                          windowFlags);

    // If one of the created fields is NULL, free everything and return NULL
    if (window->resize_event == NULL || window->sdl_window == NULL)
    {
        if (window->resize_event)
            rg_destroy_event_sender(&window->resize_event);
        if (window->sdl_window)
            SDL_DestroyWindow(window->sdl_window);
        free(window);
        return NULL;
    }

    // Everything is exists, apply more settings to the window
    SDL_SetWindowResizable(window->sdl_window, SDL_TRUE);

    return window;
}

void rg_destroy_window(rg_window **window)
{
    // Destroy SDL Window
    SDL_DestroyWindow((*window)->sdl_window);

    // Destroy event senders
    rg_destroy_event_sender(&(*window)->resize_event);

    // Free the renderer
    free(*window);

    // Since we took a double pointer in parameter, we can also set it to null
    // Thus, the user doesn't have to do it themselves
    *window = NULL;
}

double rg_window_compute_delta_time(rg_window *window, uint64_t *current_frame_time)
{
    // Update frame time counter
    uint64_t previous_frame_time = *current_frame_time;
    *current_frame_time          = SDL_GetPerformanceCounter();

    // Delta time = counter since last frame (tics) / counter frequency (tics / second)
    //            = tics / (tics / second)
    //            = tics * second / tics
    //            = second
    // -> time in second elapsed since last frame
    return ((double) (*current_frame_time - previous_frame_time)) / ((double) SDL_GetPerformanceFrequency());
}

bool rg_window_handle_events(rg_window *window)
{
    SDL_Event event;
    bool      should_quit = false;

    // Handle all events in a queue
    while (SDL_PollEvent(&event) != 0)
    {
        // Window event
        if (event.type == SDL_WINDOWEVENT) {

            // Window resized
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                // Send a resize event
                rg_window_resize_event_data event_data = {
                    .new_extent = {
                        .width = event.window.data1,
                        .height = event.window.data2,
                    }
                };
                rg_event_sender_send_event(window->resize_event, &event_data);
            }

        }
        // Quit
        else if (event.type == SDL_QUIT)
        {
            should_quit = true;
        }
    }

    return should_quit;
}

rg_extent_2d rg_window_get_current_extent(rg_window *window) {
    return window->extent;
}

// Resize event

rg_event_handler_id rg_window_resize_event_subscribe(rg_window *window, rg_event_handler handler)
{
    return rg_event_sender_register_listener(window->resize_event, handler);
}

void rg_window_resize_event_unsubscribe(rg_window *window, rg_event_handler_id handler_id) {
    rg_event_sender_unregister_listener(window->resize_event, handler_id);
}

#ifdef RENDERER_VULKAN
rg_array rg_window_get_required_vulkan_extensions(rg_window *window, unsigned int extra_array_size)
{
    if (window == NULL) {
        return (rg_array) {
            .count = 0,
            .data = NULL
        };
    }

    // Get the number of required extensions
    uint32_t required_extensions_count = 0;

    sdl_check(SDL_Vulkan_GetInstanceExtensions(window->sdl_window, &required_extensions_count, NULL));

    // Create an array with that number and fetch said extensions
    // We add the extra_array_size to allow the caller to add its own extensions at the end of the array
    rg_array required_extensions = rg_create_array_zeroed(required_extensions_count + extra_array_size, sizeof(char*));

    sdl_check(SDL_Vulkan_GetInstanceExtensions(window->sdl_window, &required_extensions_count, required_extensions.data));

    // Convert that array to a rg_string array
    rg_array string_array = rg_string_array_from_cstr_array(required_extensions.data, required_extensions.count);

    // Free the first array
    rg_destroy_array(&required_extensions);

    // Return the array
    return string_array;
}

VkSurfaceKHR rg_window_get_vulkan_surface(rg_window *window, VkInstance vulkan_instance)
{
    VkSurfaceKHR surface = NULL;
    SDL_Vulkan_CreateSurface(window->sdl_window, vulkan_instance, &surface);
    return surface;
}
#endif

#endif