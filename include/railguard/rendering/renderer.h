#pragma once

// --==== Types ====--

typedef struct rg_version
{
    unsigned int major;
    int minor;
    int patch;
} rg_version;
typedef struct rg_renderer rg_renderer;

// Also forward declare the window type to avoid include
typedef struct rg_window rg_window;

// --==== Defines ====--

#define RG_ENGINE_VERSION \
    (rg_version)          \
    {                     \
        0, 1, 0           \
    }

// --==== Renderer ====--

rg_renderer *rg_create_renderer(rg_window *window, const char *application_name, rg_version application_version);
void rg_destroy_renderer(rg_renderer **renderer);
