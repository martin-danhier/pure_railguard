#include "railguard/core/engine.h"

#include <railguard/core/renderer.h>
#include <railguard/core/window.h>

#include <stdbool.h>
#include <stdlib.h>

// --==== Types ====--

typedef struct rg_engine
{
    rg_window   *window;
    rg_renderer *renderer;
} rg_engine;

// --==== Engine methods ====--

rg_engine *rg_create_engine(void)
{
    // Allocate engine
    rg_engine *engine = calloc(1, sizeof(rg_engine));

    // Start window manager
    rg_start_window_manager();

    // Create window
    rg_extent_2d window_extent = {500, 500};
    engine->window             = rg_create_window(window_extent, "My wonderful game");

    // Create renderer
    engine->renderer = rg_create_renderer(engine->window, "My wonderful game", (rg_version) {0, 1, 0}, 2);

    // Create swapchain for window
    rg_renderer_add_window(engine->renderer, 0, engine->window);

    // Load shaders
    rg_shader_module_id vertex_shader_id =
        rg_renderer_load_shader(engine->renderer, RG_CSTR_CONST("resources/shaders/test.vert.spv"), RG_SHADER_STAGE_VERTEX);
    rg_shader_module_id fragment_shader_id =
        rg_renderer_load_shader(engine->renderer, RG_CSTR_CONST("resources/shaders/test.frag.spv"), RG_SHADER_STAGE_FRAGMENT);

    // Create a shader effect
    rg_shader_module_id stages[2] = {vertex_shader_id, fragment_shader_id};
    rg_shader_effect_id shader_effect_id =
        rg_renderer_create_shader_effect(engine->renderer, stages, 2, RG_RENDER_STAGE_KIND_LIGHTING);

    // Create a material template
    rg_material_template_id material_template_id = rg_renderer_create_material_template(engine->renderer, &shader_effect_id, 1);

    // Create a material
    rg_material_id material_id = rg_renderer_create_material(engine->renderer, material_template_id);

    // Create a model
    rg_model_id model_id = rg_renderer_create_model(engine->renderer, material_id);

    // Create a render node
    rg_render_node_id render_node_id = rg_renderer_create_render_node(engine->renderer, model_id);

    return engine;
}

void rg_destroy_engine(rg_engine **engine)
{
    // Cleanup
    rg_destroy_renderer(&(*engine)->renderer);
    rg_destroy_window(&(*engine)->window);

    // Stop window manager
    rg_stop_window_manager();

    // Free the engine
    free(*engine);
    *engine = NULL;
}

void rg_engine_run_main_loop(rg_engine *engine)
{
    // Init variables
    bool     should_quit        = false;
    uint64_t current_frame_time = 0;
    double   delta_time         = 0.0;

    // Main loop
    while (!should_quit)
    {
        // Update delta time
        delta_time = rg_window_compute_delta_time(engine->window, &current_frame_time);

        // Handle events
        should_quit = rg_window_handle_events(engine->window);

        // Run rendering
        rg_renderer_draw(engine->renderer);
    }
}
