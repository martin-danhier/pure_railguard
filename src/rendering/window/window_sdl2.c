#ifdef WINDOW_SDL2
// To prevent linking bugs, we handle the main wrapper of SDL ourselves.
// We define this constant to let it know
#define SDL_MAIN_HANDLED

#include "railguard/rendering/window.h"
#include <railguard/utils/arrays.h>

#include <SDL2/SDL.h>
#include <stdio.h>
#ifdef RENDERER_VULKAN
#include <SDL2/SDL_vulkan.h>
#endif

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

typedef struct rg_window
{
    SDL_Window *sdl_window;
    rg_extent_2d extent;
} rg_window;

rg_window *rg_create_window(rg_extent_2d extent, const char *title)
{
    // Initialize window
    rg_window *window = malloc(sizeof(rg_window));

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

    return window;
}

void rg_destroy_window(rg_window **window)
{
    // Destroy SDL Window
    SDL_DestroyWindow((*window)->sdl_window);

    // Free the renderer
    free(*window);

    // Since we took a double pointer in parameter, we can also set it to null
    // Thus, the user doesn't have to do it themselves
    *window = NULL;
}

#ifdef RENDERER_VULKAN
rg_array rg_window_get_required_vulkan_extensions(rg_window *window, unsigned int extra_array_size)
{
    // Get the number of required extensions
    uint32_t required_extensions_count = 0;

    sdl_check(SDL_Vulkan_GetInstanceExtensions(window->sdl_window, &required_extensions_count, NULL));

    // Create an array with that number and fetch said extensions
    // We add the extra_array_size to allow the caller to add its own extensions at the end of the array
    rg_array required_extensions = rg_create_array(required_extensions_count + extra_array_size, sizeof(char*));

    sdl_check(SDL_Vulkan_GetInstanceExtensions(window->sdl_window, &required_extensions_count, required_extensions.data));

    // Return the array
    return required_extensions;
}

VkSurfaceKHR rg_window_get_vulkan_surface(rg_window *window, VkInstance vulkan_instance)
{
    VkSurfaceKHR surface = NULL;
    SDL_Vulkan_CreateSurface(window->sdl_window, vulkan_instance, &surface);
    return surface;
}
#endif

#endif