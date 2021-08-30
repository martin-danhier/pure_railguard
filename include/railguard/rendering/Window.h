#pragma once


// --==== Types ====--

typedef struct rg_extent_2d
{
    unsigned int width;
    unsigned int height;
} rg_extent_2d;

typedef struct rg_window rg_window;

// Also forward declare to avoid includes
typedef struct rg_array rg_array;

// --==== Window Manager ====--

void rg_start_window_manager();
void rg_stop_window_manager();

// --==== Window ====--

rg_window *rg_create_window(rg_extent_2d extent, const char *title);
void rg_destroy_window(rg_window **window);

#ifdef RENDERER_VULKAN
rg_array rg_window_get_required_vulkan_extensions(rg_window *window, unsigned int extra_array_size);
#endif
