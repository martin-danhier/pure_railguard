#pragma once

#pragma once

#include "../framework/test_framework.h"
#include <railguard/core/renderer.h>
#include <railguard/core/window.h>

TEST(Renderer_Init)
{
    // Create a window
    rg_window *window = rg_create_window((rg_extent_2d) {800, 600}, "Renderer test");
    ASSERT_NOT_NULL(window);

    // Create a renderer
    rg_renderer *renderer = rg_create_renderer(window, "Renderer test app", (rg_version) {0, 0, 1}, 1);
    ASSERT_NOT_NULL(renderer);

    // Clean up
    rg_destroy_renderer(&renderer);
    EXPECT_NULL(renderer);

    rg_destroy_window(&window);
    EXPECT_NULL(window);
}

TEST(Renderer_MaterialSystem)
{
    // Create a window
    rg_window *window = rg_create_window((rg_extent_2d) {800, 600}, "Renderer test");
    ASSERT_NOT_NULL(window);

    // Create a renderer
    rg_renderer *renderer = rg_create_renderer(window, "Renderer test app", (rg_version) {0, 0, 1}, 1);
    ASSERT_NOT_NULL(renderer);

    // Load shaders
    rg_shader_module_id vertex_shader_id =
        rg_renderer_load_shader(renderer, RG_CSTR_CONST("resources/shaders/test.vert.spv"), RG_SHADER_STAGE_VERTEX);
    EXPECT_TRUE(vertex_shader_id != RG_STORAGE_NULL_ID);
    rg_shader_module_id fragment_shader_id =
        rg_renderer_load_shader(renderer, RG_CSTR_CONST("resources/shaders/test.frag.spv"), RG_SHADER_STAGE_FRAGMENT);
    EXPECT_TRUE(fragment_shader_id != RG_STORAGE_NULL_ID);

    // Create a shader effect
    rg_shader_module_id stages[2]        = {vertex_shader_id, fragment_shader_id};
    rg_shader_effect_id shader_effect_id = rg_renderer_create_shader_effect(renderer, stages, 2, RG_RENDER_STAGE_KIND_GEOMETRY);
    EXPECT_TRUE(shader_effect_id != RG_STORAGE_NULL_ID);

    // Create a material template
    rg_material_template_id material_template_id = rg_renderer_create_material_template(renderer, &shader_effect_id, 1);
    EXPECT_TRUE(material_template_id != RG_STORAGE_NULL_ID);

    // Create a material
    rg_material_id material_id = rg_renderer_create_material(renderer, material_template_id);
    EXPECT_TRUE(material_id != RG_STORAGE_NULL_ID);

    // Create a model
    rg_model_id model_id = rg_renderer_create_model(renderer, material_id);
    EXPECT_TRUE(model_id != RG_STORAGE_NULL_ID);

    // Create a render node
    rg_render_node_id render_node_id = rg_renderer_create_render_node(renderer, model_id);
    EXPECT_TRUE(render_node_id != RG_STORAGE_NULL_ID);

    // Clean up
    rg_destroy_renderer(&renderer);
    EXPECT_NULL(renderer);

    rg_destroy_window(&window);
    EXPECT_NULL(window);
}