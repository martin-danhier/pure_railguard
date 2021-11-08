#pragma once

#include <railguard/utils/string.h>

#include <stdint.h>

// --==== Types ====--

typedef struct rg_version
{
    unsigned int major;
    int          minor;
    int          patch;
} rg_version;

/** Flags for a render stage kind. */
typedef enum rg_render_stage_kind
{
    RG_RENDER_STAGE_KIND_INVALID  = 0,
    RG_RENDER_STAGE_KIND_GEOMETRY = 1,
    RG_RENDER_STAGE_KIND_LIGHTING = 2,
} rg_render_stage_kind;

typedef enum rg_shader_stage
{
    RG_SHADER_STAGE_INVALID  = 0,
    RG_SHADER_STAGE_VERTEX  = 1,
    RG_SHADER_STAGE_FRAGMENT = 2,
} rg_shader_stage;

/** An association of the shader and its kind. */
typedef struct rg_shader_module rg_shader_module;

/**
 * A shader effect defines the whole shader pipeline (what shader_modules are used, in what order, for what render stage...)
 */
typedef struct rg_shader_effect rg_shader_effect;

/** A material template groups the common base between similar materials. It can be used to create new materials. */
typedef struct rg_material_template rg_material_template;

/** Defines the appearance of a model (shader effect, texture...) */
typedef struct rg_material rg_material;

/** Abstract representation of a model that can be instantiated in the world. */
typedef struct rg_model rg_model;

/** Instance of a model */
typedef struct rg_render_node rg_render_node;

/**
 * The renderer is an opaque struct that contains all of the data used for rendering.
 * It exact contents depend on the used graphics API, which is why it is opaque an only handled with pointers.
 *
 * The renderer is used in all functions that concern the rendering.
 */
typedef struct rg_renderer rg_renderer;

// Also forward declare the window type to avoid include
typedef struct rg_window    rg_window;
typedef struct rg_extent_2d rg_extent_2d;
typedef uint32_t            rg_storage_id;

// Define aliases for the storage id, that way it is more intuitive to know what the id is referring to.
typedef rg_storage_id rg_shader_module_id;
typedef rg_storage_id rg_shader_effect_id;
typedef rg_storage_id rg_material_template_id;
typedef rg_storage_id rg_material_id;
typedef rg_storage_id rg_model_id;
typedef rg_storage_id rg_render_node_id;

// --==== Defines ====--

#define RG_ENGINE_VERSION \
    (rg_version)          \
    {                     \
        0, 1, 0           \
    }

// --==== Renderer ====--

/**
 * Creates a new rg_renderer, the core struct that manages the whole rendering system.
 * @param example_window Window that will be used to configure the renderer. It will not be linked to it - that will be done when
 * creating a swap chain. Understand it like an example window that we show the renderer so that it can see how it needs to initialize.
 * @param application_name Name of the application / game.
 * @param application_version Version of the application / game.
 * @param window_capacity The renderer can hold a constant number of different swapchains.
 * This number needs to be determined early on (e.g. nb of windows, nb of swapchains needed for XR...).
 * @return An opaque handle for the renderer.
 */
rg_renderer *rg_create_renderer(rg_window  *example_window,
                                const char *application_name,
                                rg_version  application_version,
                                uint32_t    window_capacity);

/**
 * Destroys the renderer and cleans up all resources.
 * @param renderer The renderer to destroy.
 */
void rg_destroy_renderer(rg_renderer **renderer);

/**
 * Links the window to the renderer in the given slot.
 * @param renderer Handle to the renderer.
 * @param window_index Index of the slot where this window will be linked. Must be empty and smaller than the
 * window capacity.
 * @param window Window that will be linked to the renderer.
 */
void rg_renderer_add_window(rg_renderer *renderer, uint32_t window_index, rg_window *window);

/**
 * Loads a shader from the given file. The language of the shader depends on the used backend.
 * @param renderer Handle to the renderer.
 * @param shader_path Path of the shader file.
 * @param kind Kind of the shader.
 * @return The id of the created shader.
 */
rg_shader_module_id rg_renderer_load_shader(rg_renderer *renderer, rg_string shader_path, rg_shader_stage stage);

/**
 * Creates a shader effect in the renderer.
 * @param renderer the renderer to use
 * @param stages shader_modules that will be used for this effect, in the order they will be used.
 * @param stage_count number of stages in the array.
 * @param build_after_creation If true, the effect will be built after creation.
 * @return the id of the created effect
 */
rg_shader_effect_id rg_renderer_create_shader_effect(rg_renderer         *renderer,
                                                     rg_shader_module_id *stages,
                                                     uint32_t             stage_count,
                                                     rg_render_stage_kind render_stage_kind);

/**
 * Destroys the given shader effect.
 * @param renderer the renderer to use
 * @param effect_id the id of the effect to destroy
 */
void rg_renderer_destroy_shader_effect(rg_renderer *renderer, rg_shader_effect_id effect_id);

/**
 * Creates a material template in the renderer.
 * @param renderer the renderer to use
 * @param available_effects the effects that can be used for this material
 * @param effect_count the number of effects in the array
 * @return the id of the created material template
 */
rg_material_template_id
    rg_renderer_create_material_template(rg_renderer *renderer, rg_shader_effect_id *available_effects, uint32_t effect_count);

/**
 * Destroys the given material template.
 * @param renderer the renderer to use
 * @param material_template_id the id of the material template to destroy
 */
void rg_renderer_destroy_material_template(rg_renderer *renderer, rg_material_template_id material_template_id);

/**
 * Creates a material in the renderer.
 * @param renderer the renderer to use
 * @param material_template_id the id of the material template to use
 * @return the id of the created material
 */
rg_material_id rg_renderer_create_material(rg_renderer *renderer, rg_material_template_id material_template_id);

/**
 * Destroys the given material.
 * @param renderer the renderer to use
 * @param material_id the id of the material to destroy
 */
void rg_renderer_destroy_material(rg_renderer *renderer, rg_material_id material_id);

/** Create a model in the renderer.
 * @param renderer the renderer to use
 * @param material_id the id of the material to use
 * @return the id of the created model
 */
rg_model_id rg_renderer_create_model(rg_renderer *renderer, rg_material_id material_id);

/**
 * Destroys the given model.
 * @param renderer the renderer to use
 * @param model_id the id of the model to destroy
 */
void rg_renderer_destroy_model(rg_renderer *renderer, rg_model_id model_id);

/**
 * Create a render node in the renderer.
 * @param renderer the renderer to use
 * @param model_id the id of the model to use
 * @return the id of the created render node
 */
rg_render_node_id rg_renderer_create_render_node(rg_renderer *renderer, rg_model_id model_id);

/**
 * Destroys the given render node.
 * @param renderer the renderer to use
 * @param render_node_id the id of the render node to destroy
 */
void rg_renderer_destroy_render_node(rg_renderer *renderer, rg_render_node_id render_node_id);
