#pragma once

#include <stdint.h>

// --==== Types ====--

typedef struct rg_version {
    unsigned int major;
    int minor;
    int patch;
} rg_version;

/**
 * The renderer is an opaque struct that contains all of the data used for rendering.
 * It exact contents depend on the used graphics API, which is why it is opaque an only handled with pointers.
 *
 * The renderer is used in all functions that concern the rendering.
 */
typedef struct rg_renderer rg_renderer;

typedef void *rg_surface;

// Also forward declare the window type to avoid include
typedef struct rg_window rg_window;
typedef struct rg_extent_2d rg_extent_2d;

// --==== Defines ====--

#define RG_ENGINE_VERSION \
    (rg_version)          \
    {                     \
        0, 1, 0           \
    }

// --==== Renderer ====--

/**
 * Creates a new rg_renderer, the core struct that manages the whole rendering system.
 * @param window Main window that will be rendered to.
 * @param application_name Name of the application / game.
 * @param application_version Version of the application / game.
 * @param swapchain_capacity The renderer can hold a constant number of different swapchains.
 * This number needs to be determined early on (e.g. nb of windows, nb of swapchains needed for XR...).
 * @return An opaque handle for the renderer.
 */
rg_renderer *rg_create_renderer(rg_window *window, const char *application_name, rg_version application_version,
                                uint32_t swapchain_capacity);

rg_surface rg_get_window_surface(rg_renderer *renderer, rg_window *window);

/**
 * Creates a new swapchain that will contain the images that will be renderer to, then presented.
 * @param renderer Handle to the renderer.
 * @param swapchain_index Index of the swapchain slot where this swapchain will be created. Must be empty and smaller than the swapchain capacity.
 * @param surface Surface that this swapchain will present to.
 * @param extent Extent of the surface.
 */
void
rg_renderer_create_swapchain(rg_renderer *renderer, uint32_t swapchain_index, rg_surface surface, rg_extent_2d extent);

void rg_renderer_recreate_swapchain(rg_renderer *renderer, uint32_t swapchain_index);

void rg_destroy_renderer(rg_renderer **renderer);
