// Place the contents of the file inside a guard, so that only one implementation is generated
#ifdef RENDERER_VULKAN

#include "railguard/core/renderer.h"
#include <railguard/utils/event_sender.h>
#include <railguard/utils/storage.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <volk.h>
// Needs to be after volk
#include <railguard/utils/io.h>
#include <railguard/utils/memory.h>

#include <vk_mem_alloc.h>

// --==== SETTINGS ====--

// region Settings

/// \brief Number of frames that can be rendered at the same time. Set to 2 for double buffering, or 3 for triple buffering.
#define NB_OVERLAPPING_FRAMES   3
#define VULKAN_API_VERSION      VK_API_VERSION_1_1
#define WAIT_FOR_FENCES_TIMEOUT 1000000000
#define SEMAPHORE_TIMEOUT       1000000000
#define RENDER_STAGE_COUNT      2

// endregion

// --==== STRUCTS ====--

// region Struct definitions

// = Buffers and image =

typedef struct rg_allocated_buffer
{
    VmaAllocation allocation;
    VkBuffer      buffer;
    uint32_t      size;
} rg_allocated_buffer;

typedef struct rg_allocated_image
{
    VmaAllocation allocation;
    VkImage       image;
    VkImageView   image_view;
} rg_allocated_image;

// = Vulkan API related =

typedef struct rg_queue
{
    uint32_t family_index;
    VkQueue  queue;
} rg_queue;

typedef struct rg_swapchain
{
    bool           enabled;
    VkSwapchainKHR vk_swapchain;
    VkExtent2D     viewport_extent;
    // Swapchain images
    uint32_t           image_count;
    VkSurfaceFormatKHR swapchain_image_format;
    // Present mode
    VkPresentModeKHR              present_mode;
    VkSurfaceTransformFlagBitsKHR transform;
    // All of these are arrays of image_count items:
    VkImage       *swapchain_images;
    VkImageView   *swapchain_image_views;
    VkFramebuffer *swapchain_image_framebuffers;
    // Depth image
    VkFormat           depth_image_format;
    rg_allocated_image depth_image;
    // Target window
    rg_window          *target_window;
    rg_event_handler_id window_resize_event_handler_id;
    VkSurfaceKHR        surface;
    // Store the built pipelines
    // We store them in the swapchain because it depends on the extent of the window
    rg_hash_map *pipelines;
    uint64_t     built_effects_version;
    // Render stages
    rg_array render_stages;
    // Pointer to renderer so that we can go back to it from the swapchain
    rg_renderer *renderer;
} rg_swapchain;

typedef struct rg_passes
{
    VkRenderPass geometry_pass;
    VkRenderPass lighting_pass;
} rg_passes;

typedef struct rg_frame_data
{
    VkCommandPool   command_pool;
    VkCommandBuffer command_buffer;
    VkSemaphore     present_semaphore;
    VkSemaphore     render_semaphore;
    VkFence         render_fence;
} rg_frame_data;

// = Material system =

// region Material system

typedef struct rg_shader_module
{
    VkShaderModule  vk_module;
    rg_shader_stage stage;
} rg_shader_module;

typedef struct rg_shader_effect
{
    /** Render stages in which this effect can be used */
    rg_render_stage_kind render_stage_kind;
    /** Array of shader module ID. Shader stages of the pipeline, in order. */
    rg_array         shader_stages;
    VkPipelineLayout pipeline_layout;
} rg_shader_effect;

typedef struct rg_material_template
{
    /**
     * Array of shader effect IDs. Available effects for this template.
     * Given a render stage kind, the first corresponding effect will be the one used.
     * */
    rg_array shader_effects;
} rg_material_template;

typedef struct rg_material
{
    /** Template this material is based on. Defines the available shader effects for this material. */
    rg_material_template_id material_template_id;
    rg_vector               models_using_material;
} rg_material;

typedef struct rg_model
{
    /** Material used by this model. */
    rg_material_id material_id;
    rg_vector      instances;

} rg_model;

typedef struct rg_render_node
{
    rg_model_id model_id;
} rg_render_node;

// endregion

// = Render stages =

typedef struct rg_render_batch
{
    size_t     offset;
    size_t     count;
    VkPipeline pipeline;
} rg_render_batch;

typedef struct rg_render_stage
{
    rg_render_stage_kind kind;
    rg_allocated_buffer  indirect_buffer;
    rg_vector            batches;
} rg_render_stage;

// = Global renderer =

typedef struct rg_renderer
{
    VkDevice         device;
    VkInstance       instance;
    VkPhysicalDevice physical_device;
    rg_queue         graphics_queue;
    VmaAllocator     allocator;
    rg_passes        passes;
#ifdef USE_VK_VALIDATION_LAYERS
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
    /**
     * @brief Fixed-count array containing the swapchains.
     * We place them in a array to be able to efficiently iterate through them. And because their number won't change, we can safely
     * send pointers to individual swapchains around.
     */
    rg_array swapchains;

    // Frame management

    // Counter of frame since the start of the renderer
    uint64_t      current_frame_number;
    rg_frame_data frames[NB_OVERLAPPING_FRAMES];

    // Define the storages for the material system
    rg_storage *shader_modules;
    rg_storage *shader_effects;
    rg_storage *material_templates;
    rg_storage *materials;
    rg_storage *models;
    rg_storage *render_nodes;

    // Number incremented at each created shader effect
    // It is stored in the swapchain when effects are built
    // If the number in the swapchain is different, we need to rebuild the pipelines
    uint64_t effects_version;
} rg_renderer;

// endregion

// --==== UTILS FUNCTIONS ====--

// region Error handling functions

rg_string rg_renderer_vk_result_to_str(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS: return RG_CSTR_CONST("VK_SUCCESS");
        case VK_ERROR_INITIALIZATION_FAILED: return RG_CSTR_CONST("VK_ERROR_INITIALIZATION_FAILED");
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return RG_CSTR_CONST("VK_ERROR_NATIVE_WINDOW_IN_USE_KHR");
        case VK_TIMEOUT: return RG_CSTR_CONST("VK_TIMEOUT");
        default:
            return RG_CSTR_CONST(
                "N"); // We just need a character that is different from VK_ ... because I don't want to convert the number to string
    }
}

void vk_check(VkResult result, const char *error_message)

{
    if (result != VK_SUCCESS)
    {
        if (result == VK_TIMEOUT)
        {
            printf("[Vulkan Warning] A Vulkan function call returned VK_TIMEOUT !\n");
        }
        else
        {
            // Pretty print error
            rg_string result_str = rg_renderer_vk_result_to_str(result);
            if (result_str.data[0] == 'N')
            {
                fprintf(stderr, "[Vulkan Error] A Vulkan function call returned VkResult = %d !\n", result);
            }
            else
            {
                fprintf(stderr, "[Vulkan Error] A Vulkan function call returned %s !\n", result_str.data);
            }
            // Optional custom error message precision
            if (error_message != NULL)
            {
                fprintf(stderr, "Precision: %s\n", error_message);
            }
            exit(1);
        }
    }
}

void rg_renderer_check(bool result, const char *error_message)
{
    if (!result)
    {
        fprintf(stderr, "[Error] %s\n", error_message);
        exit(1);
    }
}

// endregion

// region Extensions and layers functions

/**
 * @brief Checks if the given extensions are supported
 * @param desired_extensions is an array of desired_extensions_count layer names
 * @param desired_extensions_count is the number of extension names to check
 * @return true if every extension is supported, false otherwise.
 */
bool rg_renderer_check_instance_extension_support(rg_string *desired_extensions, uint32_t desired_extensions_count)
{
    // Get the number of available desired_extensions
    uint32_t available_extensions_count = 0;
    vk_check(vkEnumerateInstanceExtensionProperties(NULL, &available_extensions_count, VK_NULL_HANDLE), NULL);
    // Create an array with enough room and fetch the available desired_extensions
    rg_array available_extensions = rg_create_array(available_extensions_count, sizeof(VkExtensionProperties));
    vk_check(vkEnumerateInstanceExtensionProperties(NULL, &available_extensions_count, available_extensions.data), NULL);

    // For each desired extension, rg_renderer_check if it is available
    bool valid = true;
    for (uint32_t i = 0; i < desired_extensions_count && valid; i++)
    {
        bool found = false;

        // Search available until the desired is found or not
        for (uint32_t j = 0; j < available_extensions_count && !found; j++)
        {
            if (rg_string_equals(desired_extensions[i],
                                 RG_CSTR(((VkExtensionProperties *) available_extensions.data)[j].extensionName)))
            {
                found = true;
            }
        }

        // Stop looping if nothing was found
        if (!found)
        {
            valid = false;
        }
    }

    // We must clean the array
    rg_destroy_array(&available_extensions);

    return valid;
}

/**
 * @brief Checks if the given extensions are supported
 * @param desired_extensions is an array of desired_extensions_count layer names (null terminated strings)
 * @param desired_extensions_count is the number of extension names to check
 * @return true if every extension is supported, false otherwise.
 */
bool rg_renderer_check_device_extension_support(VkPhysicalDevice physical_device,
                                                rg_string       *desired_extensions,
                                                uint32_t         desired_extensions_count)
{
    // Get the number of available desired_extensions
    uint32_t available_extensions_count = 0;
    vk_check(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &available_extensions_count, VK_NULL_HANDLE), NULL);
    // Create an array with enough room and fetch the available desired_extensions
    rg_array available_extensions = rg_create_array(available_extensions_count, sizeof(VkExtensionProperties));
    vk_check(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &available_extensions_count, available_extensions.data),
             NULL);

    // For each desired extension, rg_renderer_check if it is available
    bool valid = true;
    for (uint32_t i = 0; i < desired_extensions_count && valid; i++)
    {
        bool found = false;

        // Search available until the desired is found or not
        for (uint32_t j = 0; j < available_extensions_count && !found; j++)
        {
            if (rg_string_equals(desired_extensions[i],
                                 RG_CSTR(((VkExtensionProperties *) available_extensions.data)[j].extensionName)))
            {
                found = true;
            }
        }

        // Stop looping if nothing was found
        if (!found)
        {
            valid = false;
        }
    }

    // We must clean the array
    rg_destroy_array(&available_extensions);

    return valid;
}

#if USE_VK_VALIDATION_LAYERS
/**
 * @brief Checks if the given validation layers are supported
 * @param desired_layers is an array of desired_layers_count layer names (null terminated strings)
 * @param desired_layers_count is the number of layer names to check
 * @return true if every layer is supported, false otherwise.
 */
bool rg_renderer_check_layer_support(rg_string *desired_layers, uint32_t desired_layers_count)
{
    // Get the number of available desired_layers
    uint32_t available_layers_count = 0;
    vk_check(vkEnumerateInstanceLayerProperties(&available_layers_count, VK_NULL_HANDLE), NULL);
    // Create an array with enough room and fetch the available desired_layers
    rg_array available_layers = rg_create_array(available_layers_count, sizeof(VkLayerProperties));
    vk_check(vkEnumerateInstanceLayerProperties(&available_layers_count, available_layers.data), NULL);

    // For each desired layer, check if it is available
    bool valid = true;
    for (uint32_t i = 0; i < desired_layers_count && valid; i++)
    {
        bool found = false;

        // Search available until the desired is found or not
        for (uint32_t j = 0; j < available_layers_count && !found; j++)
        {
            if (rg_string_equals(desired_layers[i], RG_CSTR(((VkLayerProperties *) available_layers.data)[j].layerName)))
            {
                found = true;
            }
        }

        // Stop looping if nothing was found
        if (!found)
        {
            valid = false;
        }
    }

    // We must clean the array
    rg_destroy_array(&available_layers);

    return valid;
}

/**
 * Callback for the vulkan debug messenger
 * @param message_severity Severity of the message
 * @param message_types Type of the message
 * @param callback_data Additional data concerning the message
 * @param user_data User data passed to the debug messenger
 * @return
 */
VkBool32 *rg_renderer_debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                               VkDebugUtilsMessageTypeFlagsEXT             message_types,
                                               const VkDebugUtilsMessengerCallbackDataEXT *callback_data)
{
    // Inspired by VkBootstrap's default debug messenger. (Made by Charles Giessen)

    // Get severity
    char *str_severity = NULL;
    switch (message_severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: str_severity = "VERBOSE"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: str_severity = "TF_ERROR"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: str_severity = "TF_WARNING"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: str_severity = "INFO"; break;
        default: str_severity = "UNKNOWN"; break;
    }

    // Get type
    char *str_type;
    switch (message_types)
    {
        case 7: str_type = "General | Validation | Performance"; break;
        case 6: str_type = "Validation | Performance"; break;
        case 5: str_type = "General | Performance"; break;
        case 4: str_type = "Performance"; break;
        case 3: str_type = "General | Validation"; break;
        case 2: str_type = "Validation"; break;
        case 1: str_type = "General"; break;
        default: str_type = "Unknown"; break;
    }

    // Print the message to stderr if it is an error.
    if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        fprintf(stderr, "[%s: %s]\n%s\n", str_severity, str_type, callback_data->pMessage);
    }
    else
    {
        printf("[%s: %s]\n%s\n", str_severity, str_type, callback_data->pMessage);
    }

    return VK_FALSE;
}

#endif

// endregion

// region Physical device functions

/**
 * @brief Computes a score for the given physical device.
 * @param device is the device to evaluate.
 * @return the score of that device. A bigger score means that the device is better suited.
 */
uint32_t rg_renderer_rate_physical_device(VkPhysicalDevice device)
{
    uint32_t score = 0;

    // Get properties and features of that device
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures   device_features;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    vkGetPhysicalDeviceFeatures(device, &device_features);

    // Prefer discrete gpu when available
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 10000;
    }

    // The bigger, the better
    score += device_properties.limits.maxImageDimension2D;

    // The device needs to support the following device extensions, otherwise it is unusable
#define REQUIRED_DEVICE_EXT_COUNT 1
    rg_string required_device_extensions[REQUIRED_DEVICE_EXT_COUNT] = {
        RG_CSTR_CONST(VK_KHR_SWAPCHAIN_EXTENSION_NAME),
    };
    bool extensions_are_supported =
        rg_renderer_check_device_extension_support(device, required_device_extensions, REQUIRED_DEVICE_EXT_COUNT);

    // Reset score if the extension are not supported because it is mandatory
    if (!extensions_are_supported)
    {
        score = 0;
    }

    // printf("GPU: %s | Score: %d\n", name, score);

    return score;
}
// endregion

// region Format functions

VkSurfaceFormatKHR rg_select_surface_format(rg_renderer *renderer, rg_window *window)
{
    // Get surface
    VkSurfaceKHR       surface        = rg_window_get_vulkan_surface(window, renderer->instance);
    VkSurfaceFormatKHR surface_format = {VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    // Get the available formats
    uint32_t available_format_count = 0;
    vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device, surface, &available_format_count, VK_NULL_HANDLE), NULL);
    rg_array available_formats = rg_create_array(available_format_count, sizeof(VkSurfaceFormatKHR));
    vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device, surface, &available_format_count, available_formats.data),
             NULL);

    // Desired formats, by order of preference
#define DESIRED_FORMAT_COUNT 2
    const VkSurfaceFormatKHR desired_formats[DESIRED_FORMAT_COUNT] = {
        {
            VK_FORMAT_B8G8R8A8_SRGB,
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        },
        {
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        },
    };

    // Get the first desired format available
    bool found = false;
    for (uint32_t i = 0; i < available_format_count && !found; i++)
    {
        VkSurfaceFormatKHR *available_format = ((VkSurfaceFormatKHR *) available_formats.data) + i;

        for (uint32_t j = 0; j < DESIRED_FORMAT_COUNT && !found; j++)
        {
            if (available_format->format == desired_formats[j].format && available_format->colorSpace == desired_formats[j].colorSpace)
            {
                surface_format = desired_formats[j];
                found          = true;
            }
        }
    }
    rg_renderer_check(found, "Couldn't find an appropriate format for the surface.");

    // Cleanup format selection
    rg_destroy_array(&available_formats);
    vkDestroySurfaceKHR(renderer->instance, surface, NULL);

    return surface_format;
}

// endregion

// region Window events handling

// Forward declare functions we will need in handlers

void rg_renderer_recreate_swapchain(rg_swapchain *swapchain, rg_extent_2d new_extent);

// Callback called when the window is resized
void rg_renderer_handle_window_resize_event(rg_window_resize_event_data *data, rg_swapchain *swapchain)
{
    // Recreate swap chain
    rg_renderer_recreate_swapchain(swapchain, data->new_extent);
}

// endregion

// region Allocator

VmaAllocator rg_renderer_create_allocator(VkInstance instance, VkDevice device, VkPhysicalDevice physical_device)
{
    VmaVulkanFunctions vulkan_functions = {
        .vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory                        = vkAllocateMemory,
        .vkFreeMemory                            = vkFreeMemory,
        .vkMapMemory                             = vkMapMemory,
        .vkUnmapMemory                           = vkUnmapMemory,
        .vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory                      = vkBindBufferMemory,
        .vkBindImageMemory                       = vkBindImageMemory,
        .vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements,
        .vkCreateBuffer                          = vkCreateBuffer,
        .vkDestroyBuffer                         = vkDestroyBuffer,
        .vkCreateImage                           = vkCreateImage,
        .vkDestroyImage                          = vkDestroyImage,
        .vkCmdCopyBuffer                         = vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2KHR,
        .vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2KHR,
        .vkBindBufferMemory2KHR                  = vkBindBufferMemory2KHR,
        .vkBindImageMemory2KHR                   = vkBindImageMemory2KHR,
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR,
    };
    VmaAllocatorCreateInfo allocator_create_info = {
        .instance         = instance,
        .device           = device,
        .physicalDevice   = physical_device,
        .pVulkanFunctions = &vulkan_functions,
    };

    VmaAllocator allocator = VK_NULL_HANDLE;
    vmaCreateAllocator(&allocator_create_info, &allocator);

    return allocator;
}

void rg_renderer_destroy_allocator(VmaAllocator *allocator)
{
    vmaDestroyAllocator(*allocator);
    *allocator = VK_NULL_HANDLE;
}

rg_allocated_buffer rg_renderer_create_buffer(VmaAllocator       allocator,
                                              size_t             allocation_size,
                                              VkBufferUsageFlags buffer_usage,
                                              VmaMemoryUsage     memory_usage)
{
    // We use VMA for now. We can always switch to a custom allocator later if we want to.
    rg_allocated_buffer buffer = {
        .size = allocation_size,
    };

    // Create the buffer using VMA
    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        // Buffer info
        .size  = allocation_size,
        .usage = buffer_usage,
    };

    // Create an allocation info
    VmaAllocationCreateInfo allocation_create_info = {
        .usage = memory_usage,
    };

    // Create the buffer
    vk_check(
        vmaCreateBuffer(allocator, &buffer_create_info, &allocation_create_info, &buffer.buffer, &buffer.allocation, VK_NULL_HANDLE),
        "Couldn't allocate buffer");

    return buffer;
}

void rg_renderer_destroy_buffer(VmaAllocator allocator, rg_allocated_buffer *buffer)
{
    // Ignore if already destroyed
    if (buffer->buffer == VK_NULL_HANDLE)
    {
        return;
    }

    vmaDestroyBuffer(allocator, buffer->buffer, buffer->allocation);
    buffer->buffer     = VK_NULL_HANDLE;
    buffer->allocation = VK_NULL_HANDLE;
    buffer->size       = 0;
}

rg_allocated_image rg_renderer_create_image(VmaAllocator          allocator,
                                            VkDevice              device,
                                            VkFormat              image_format,
                                            VkExtent3D            image_extent,
                                            VkImageUsageFlags     image_usage,
                                            VkImageAspectFlagBits image_aspect,
                                            VmaMemoryUsage        memory_usage)
{
    // We use VMA for now. We can always switch to a custom allocator later if we want to.
    rg_allocated_image image = {
        VK_NULL_HANDLE,
    };

    rg_renderer_check(image_extent.width >= 1 && image_extent.height >= 1 && image_extent.depth >= 1,
                      "Tried to create an image with an invalid extent. The extent must be at least 1 in each dimension.");

    // Create the image using VMA
    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        // Image info
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = image_format,
        .extent        = image_extent,
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = image_usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    // Create an allocation info
    VmaAllocationCreateInfo allocation_create_info = {
        .usage          = memory_usage,
        .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    // Create the image
    vk_check(vmaCreateImage(allocator, &image_create_info, &allocation_create_info, &image.image, &image.allocation, VK_NULL_HANDLE),
             "Couldn't allocate image");

    // Create the image view
    VkImageViewCreateInfo image_view_create_info = {
        // Struct info
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        // Image view info
        .format   = image_format,
        .image    = image.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .subresourceRange =
            {
                .aspectMask     = image_aspect,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
    };
    vk_check(vkCreateImageView(device, &image_view_create_info, NULL, &image.image_view), "Couldn't create image view");

    return image;
}

void rg_renderer_destroy_image(VmaAllocator allocator, VkDevice device, rg_allocated_image *image)
{
    // Destroy the image view first
    vkDestroyImageView(device, image->image_view, VK_NULL_HANDLE);
    image->image_view = VK_NULL_HANDLE;

    // Destroy the image next
    vmaDestroyImage(allocator, image->image, image->allocation);
    image->image      = VK_NULL_HANDLE;
    image->allocation = VK_NULL_HANDLE;
}

void *rg_renderer_map_buffer(VmaAllocator allocator, rg_allocated_buffer *buffer)
{
    void *data = NULL;
    vmaMapMemory(allocator, buffer->allocation, &data);
    return data;
}

void rg_renderer_unmap_buffer(VmaAllocator allocator, rg_allocated_buffer *buffer)
{
    vmaUnmapMemory(allocator, buffer->allocation);
}

// endregion

// region Frame functions

static inline uint64_t rg_renderer_get_current_frame_index(rg_renderer *renderer)
{
    return renderer->current_frame_number % NB_OVERLAPPING_FRAMES;
}

static inline rg_frame_data *rg_renderer_get_current_frame(rg_renderer *renderer)
{
    return &renderer->frames[rg_renderer_get_current_frame_index(renderer)];
}

static inline void rg_renderer_wait_for_fence(rg_renderer *renderer, VkFence fence)
{
    // Wait for it
    vk_check(vkWaitForFences(renderer->device, 1, &fence, VK_TRUE, WAIT_FOR_FENCES_TIMEOUT), "Couldn't wait for fence");
    // Reset it
    vk_check(vkResetFences(renderer->device, 1, &fence), "Couldn't reset fence");
}

// endregion

// --==== RENDERER ====--

// region Shaders functions

rg_shader_module_id rg_renderer_load_shader(rg_renderer *renderer, rg_string shader_path, rg_shader_stage stage)
{
    // Load binary from file
    uint32_t *code      = NULL;
    size_t    code_size = 0;
    rg_renderer_check(rg_load_file_binary(shader_path, (void **) &code, &code_size), "Couldn't load shader binary");

    // Create shader vk_module
    VkShaderModuleCreateInfo shader_module_create_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = NULL,
        .codeSize = code_size,
        .pCode    = code,
    };
    VkShaderModule vk_module = VK_NULL_HANDLE;
    vk_check(vkCreateShaderModule(renderer->device, &shader_module_create_info, NULL, &vk_module), "Couldn't create shader vk_module");

    // Free the code
    if (code != NULL)
    {
        rg_free(code);
    }

    // Create the shader vk_module
    rg_shader_module module = {
        .vk_module = vk_module,
        .stage     = stage,
    };

    // Add the shader to the storage
    rg_shader_module_id shader_id = rg_storage_push(renderer->shader_modules, &module);

    // Get the name of the shader without the beginning of the path
    size_t    i = rg_string_find_char_reverse(shader_path, '/');
    rg_string shader_name;
    if (i == -1)
    {
        shader_name = shader_path;
    }
    else
    {
        shader_name = rg_string_get_substring(shader_path, i + 1, rg_string_end(shader_path));
    }

    printf("Loaded shader \"%s\"\n", shader_name.data);

    // Return the shader id
    return shader_id;
}

void rg_renderer_destroy_shader(rg_renderer *renderer, rg_shader_module_id shader_id)
{
    // Get shader module
    rg_shader_module *module = rg_storage_get(renderer->shader_modules, shader_id);
    if (module != NULL)
    {
        // Destroy shader module
        vkDestroyShaderModule(renderer->device, module->vk_module, NULL);

        // Remove from map
        rg_storage_erase(renderer->shader_modules, shader_id);
    }
}

void rg_renderer_clear_shaders(rg_renderer *renderer)
{
    if (renderer->shader_modules != NULL)
    {
        // Destroy all shader modules
        rg_storage_it it = rg_storage_iterator(renderer->shader_modules);
        while (rg_storage_next(&it))
        {
            rg_shader_module *module = it.value;
            vkDestroyShaderModule(renderer->device, module->vk_module, NULL);
        }

        // Destroy map
        rg_destroy_storage(&renderer->shader_modules);
    }
}

rg_shader_module *rg_renderer_get_shader(rg_renderer *renderer, rg_shader_module_id shader_id)
{
    // Get shader module
    return (rg_shader_module *) rg_storage_get(renderer->shader_modules, shader_id);
}

// endregion

// region Shader effects and pipelines functions

VkPipeline rg_render_build_shader_effect(rg_renderer *renderer, VkExtent2D window_extent, rg_shader_effect *effect)
{
    // This function will take the data contained in the effect and build a pipeline with it
    // First, create all the structs we will need in the pipeline create info

    // region Create shader stages

    rg_array shader_stages = rg_create_array(effect->shader_stages.count, sizeof(VkPipelineShaderStageCreateInfo));
    for (uint32_t i = 0; i < effect->shader_stages.count; i++)
    {
        // Get shader module
        rg_shader_module *shader_module = rg_renderer_get_shader(renderer, ((rg_shader_module_id *) effect->shader_stages.data)[i]);
        rg_renderer_check(shader_module != NULL, "Couldn't get shader module");

        // Convert stage flags
        VkShaderStageFlagBits stage_flags = 0;
        switch (shader_module->stage)
        {
            case RG_SHADER_STAGE_VERTEX: stage_flags = VK_SHADER_STAGE_VERTEX_BIT; break;
            case RG_SHADER_STAGE_FRAGMENT: stage_flags = VK_SHADER_STAGE_FRAGMENT_BIT; break;
            default: rg_renderer_check(false, "Unknown shader stage");
        }

        // Create shader stage
        ((VkPipelineShaderStageCreateInfo *) shader_stages.data)[i] = (VkPipelineShaderStageCreateInfo) {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = NULL,
            .flags               = 0,
            .stage               = stage_flags,
            .module              = shader_module->vk_module,
            .pName               = "main",
            .pSpecializationInfo = NULL,
        };
    }

    // endregion

    // region Create vertex input state

    // Default for now because we don't have vertex input
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = NULL,
        .flags                           = 0,
        .vertexBindingDescriptionCount   = 0,
        .pVertexBindingDescriptions      = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions    = NULL,
    };

    // endregion

    // region Create input assembly state

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = NULL,
        .flags                  = 0,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // endregion

    // region Create viewport state

    VkViewport viewport = {
        // Start in the corner
        .x = 0.0f,
        .y = 0.0f,
        // Scale to the size of the window
        .width  = (float) window_extent.width,
        .height = (float) window_extent.height,
        // Depth range is 0.0f to 1.0f
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        // Start in the corner
        .offset = (VkOffset2D) {0, 0},
        // Scale to the size of the window
        .extent = (VkExtent2D) {window_extent.width, window_extent.height},
    };

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = NULL,
        .flags         = 0,
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissor,
    };

    // endregion

    // region Create rasterization state

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext            = NULL,
        .flags            = 0,
        .depthClampEnable = VK_FALSE,
        // Keep the primitive in the rasterization stage
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        // No backface culling
        .cullMode  = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        // No depth bias
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
        // Width of the line
        .lineWidth = 1.0f,
    };

    // endregion

    // region Create multisample state

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                 = NULL,
        .flags                 = 0,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0f,
        .pSampleMask           = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    };

    // endregion

    // region Create color blend state

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
        .blendEnable    = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    // No blending
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = NULL,
        .flags           = 0,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments    = &color_blend_attachment_state,
        .blendConstants  = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    // endregion

    // region Create depth stencil state

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = NULL,
        .flags                 = 0,
        .depthTestEnable       = VK_FALSE,
        .depthWriteEnable      = VK_FALSE,
        .depthCompareOp        = VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = false,
        .stencilTestEnable     = false,
        .minDepthBounds        = 0.0f,
        .maxDepthBounds        = 1.0f,
    };

    // endregion

    // region Get render pass

    VkRenderPass render_pass = VK_NULL_HANDLE;
    switch (effect->render_stage_kind)
    {
        case RG_RENDER_STAGE_KIND_GEOMETRY: render_pass = renderer->passes.geometry_pass; break;
        case RG_RENDER_STAGE_KIND_LIGHTING: render_pass = renderer->passes.lighting_pass; break;
        default: rg_renderer_check(false, "Invalid render stage kind");
    }

    // endregion

    // region Create pipeline

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = NULL,
        .flags               = 0,
        .stageCount          = shader_stages.count,
        .pStages             = shader_stages.data,
        .pVertexInputState   = &vertex_input_state_create_info,
        .pInputAssemblyState = &input_assembly_state_create_info,
        .pTessellationState  = NULL,
        .pViewportState      = &viewport_state_create_info,
        .pRasterizationState = &rasterization_state_create_info,
        .pMultisampleState   = &multisample_state_create_info,
        .pDepthStencilState  = &depth_stencil_state_create_info,
        .pColorBlendState    = &color_blend_state_create_info,
        .pDynamicState       = NULL,
        .layout              = effect->pipeline_layout,
        .renderPass          = render_pass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };
    VkPipeline pipeline = VK_NULL_HANDLE;
    vk_check(vkCreateGraphicsPipelines(renderer->device, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline),
             "Failed to create pipeline");

    // endregion

    // Clean up
    rg_destroy_array(&shader_stages);

    return pipeline;
}

void rg_renderer_build_out_of_date_effects(rg_swapchain *swapchain)
{
    if (swapchain->built_effects_version < swapchain->renderer->effects_version)
    {
        // Build all effects
        rg_storage_it it = rg_storage_iterator(swapchain->renderer->shader_effects);
        while (rg_storage_next(&it))
        {
            // Don't build a pipeline that is already built
            rg_hash_map_get_result get_result = rg_hash_map_get(swapchain->pipelines, it.id);
            if (!get_result.exists)
            {
                // Store the pipeline with the same id as the effect
                // That way, we can easily find the pipeline of a given effect
                rg_hash_map_set(swapchain->pipelines,
                                it.id,
                                (rg_hash_map_value_t) {
                                    .as_ptr = rg_render_build_shader_effect(swapchain->renderer, swapchain->viewport_extent, it.value),
                                });
            }
        }

        // Update the version
        swapchain->built_effects_version = swapchain->renderer->effects_version;
    }
}

void rg_renderer_clear_pipelines(rg_swapchain *swapchain)
{
    // Destroy all pipelines
    rg_hash_map_it it = rg_hash_map_iterator(swapchain->pipelines);
    while (rg_hash_map_next(&it))
    {
        vkDestroyPipeline(swapchain->renderer->device, it.value.as_ptr, NULL);
    }

    // Clear the map
    rg_hash_map_clear(swapchain->pipelines);

    // Reset the version
    swapchain->built_effects_version = 0;
}

void rg_renderer_recreate_pipelines(rg_swapchain *swapchain)
{
    // Destroy all pipelines
    rg_renderer_clear_pipelines(swapchain);

    // Build all effects
    rg_renderer_build_out_of_date_effects(swapchain);
}

void rg_renderer_destroy_pipeline(rg_swapchain *swapchain, rg_shader_effect_id effect_id)
{
    if (swapchain->enabled)
    {
        // Get the pipeline
        rg_hash_map_get_result get_result = rg_hash_map_get(swapchain->pipelines, effect_id);
        if (get_result.exists)
        {
            vkDestroyPipeline(swapchain->renderer->device, get_result.value.as_ptr, NULL);
            rg_hash_map_erase(swapchain->pipelines, effect_id);
        }
    }
}

rg_shader_effect_id rg_renderer_create_shader_effect(rg_renderer         *renderer,
                                                     rg_shader_module_id *stages,
                                                     uint32_t             stage_count,
                                                     rg_render_stage_kind render_stage_kind)
{
    rg_renderer_check(stage_count > 0, "Attempted to create a shader effect with 0 stages");
    rg_renderer_check(renderer != NULL, NULL);
    rg_renderer_check(stages != NULL, NULL);

    // Create shader effect
    rg_shader_effect shader_effect;
    shader_effect.render_stage_kind = render_stage_kind;

    // Create array of stages
    shader_effect.shader_stages = rg_create_array(stage_count, sizeof(rg_shader_module_id));
    rg_renderer_check(memcpy(shader_effect.shader_stages.data, stages, stage_count * sizeof(rg_shader_module_id)) != NULL,
                      "Couldn't copy shader stages");

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = NULL,
        .flags                  = 0,
        .setLayoutCount         = 0,
        .pSetLayouts            = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL,
    };
    shader_effect.pipeline_layout = VK_NULL_HANDLE;
    vk_check(vkCreatePipelineLayout(renderer->device, &pipeline_layout_create_info, NULL, &shader_effect.pipeline_layout),
             "Couldn't create pipeline layout");

    // Store effect
    rg_shader_effect_id effect_id = rg_storage_push(renderer->shader_effects, &shader_effect);
    if (effect_id == RG_STORAGE_NULL_ID)
    {
        rg_renderer_check(false, "Couldn't store shader effect");
    }

    // Increment the version so that the swapchains rebuild the pipelines
    renderer->effects_version++;

    return effect_id;
}

void rg_renderer_clear_shader_effects(rg_renderer *renderer)
{
    if (renderer->shader_effects != NULL)
    {
        // Clean content of storage
        rg_storage_it it = rg_storage_iterator(renderer->shader_effects);
        while (rg_storage_next(&it))
        {
            rg_shader_effect *effect = it.value;

            vkDestroyPipelineLayout(renderer->device, effect->pipeline_layout, NULL);
            rg_destroy_array(&effect->shader_stages);
        }

        // Clean storage itself
        rg_destroy_storage(&renderer->shader_effects);
    }
}

void rg_renderer_destroy_shader_effect(rg_renderer *renderer, rg_shader_effect_id effect_id)
{
    // Get effect
    rg_shader_effect *effect = rg_storage_get(renderer->shader_effects, effect_id);
    if (effect != NULL)
    {
        // Destroy effect
        vkDestroyPipelineLayout(renderer->device, effect->pipeline_layout, NULL);
        rg_destroy_array(&effect->shader_stages);

        // Remove from map
        rg_storage_erase(renderer->shader_effects, effect_id);

        // Destroy the pipelines
        for (uint32_t i = 0; i < renderer->swapchains.count; i++)
        {
            rg_renderer_destroy_pipeline(&((rg_swapchain *) renderer->swapchains.data)[i], effect_id);
        }
    }
}

// endregion

// region Frame functions

static inline void rg_wait_for_current_fence(rg_renderer *renderer)
{
    rg_renderer_wait_for_fence(renderer, rg_renderer_get_current_frame(renderer)->render_fence);
}

void rg_renderer_wait_for_all_fences(rg_renderer *renderer)
{
    // Get fences in array
    VkFence fences[NB_OVERLAPPING_FRAMES];
    for (uint64_t i = 0; i < NB_OVERLAPPING_FRAMES; i++)
    {
        fences[i] = renderer->frames[i].render_fence;
    }

    vk_check(vkWaitForFences(renderer->device, NB_OVERLAPPING_FRAMES, fences, VK_TRUE, WAIT_FOR_FENCES_TIMEOUT),
             "Failed to wait for fences");
}

VkCommandBuffer rg_renderer_begin_recording(rg_renderer *renderer)
{
    // Get current frame
    rg_frame_data *frame = rg_renderer_get_current_frame(renderer);

    // Reset command buffer
    vk_check(vkResetCommandBuffer(frame->command_buffer, 0), NULL);

    // Begin command buffer
    VkCommandBufferBeginInfo begin_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = VK_NULL_HANDLE,
        .pInheritanceInfo = VK_NULL_HANDLE,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vk_check(vkBeginCommandBuffer(frame->command_buffer, &begin_info), NULL);

    return frame->command_buffer;
}

void rg_renderer_end_recording_and_submit(rg_renderer *renderer)
{
    // Get current frame
    rg_frame_data *frame = rg_renderer_get_current_frame(renderer);

    // End command buffer
    vk_check(vkEndCommandBuffer(frame->command_buffer), NULL);

    // Submit command buffer
    VkPipelineStageFlags wait_stage  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo         submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = VK_NULL_HANDLE,
        // Wait until the image to render is ready
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &frame->present_semaphore,
        // Pipeline stage
        .pWaitDstStageMask = &wait_stage,
        // Link the command buffer
        .commandBufferCount = 1,
        .pCommandBuffers    = &frame->command_buffer,
        // Signal the render semaphore
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &frame->render_semaphore,
    };
    vk_check(vkQueueSubmit(renderer->graphics_queue.queue, 1, &submit_info, frame->render_fence), NULL);
}

// endregion

// region Swapchain functions
void rg_init_swapchain_inner(rg_renderer *renderer, rg_swapchain *swapchain, rg_extent_2d extent)
{
    // region Swapchain creation

    // Save extent
    swapchain->viewport_extent = (VkExtent2D) {
        .width  = extent.width,
        .height = extent.height,
    };

    // Create the swapchain
    VkSwapchainCreateInfoKHR swapchain_create_info = {
        // Struct info
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        // Present options
        .presentMode = swapchain->present_mode,
        // Image options
        .surface          = swapchain->surface,
        .preTransform     = swapchain->transform,
        .imageExtent      = swapchain->viewport_extent,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .minImageCount    = swapchain->image_count,
        .imageFormat      = swapchain->swapchain_image_format.format,
        .imageColorSpace  = swapchain->swapchain_image_format.colorSpace,
        .clipped          = VK_TRUE,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .imageArrayLayers = 1,
        // For now, we use the same queue for rendering and presenting. Maybe in the future, we will want to change that.
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    vk_check(vkCreateSwapchainKHR(renderer->device, &swapchain_create_info, NULL, &swapchain->vk_swapchain),
             "Couldn't create swapchain.");

    // endregion

    // region Images and image views

    // Get the images
    uint32_t effective_image_count = 0;
    vkGetSwapchainImagesKHR(renderer->device, swapchain->vk_swapchain, &effective_image_count, VK_NULL_HANDLE);
    swapchain->swapchain_images = rg_malloc(sizeof(VkImage) * effective_image_count);
    vkGetSwapchainImagesKHR(renderer->device, swapchain->vk_swapchain, &effective_image_count, swapchain->swapchain_images);

    // Update the image count based on how many were effectively created
    swapchain->image_count = effective_image_count;

    // Create image views for those images
    VkImageViewCreateInfo image_view_create_info = {
        // Struct info
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        // Image info
        .format   = swapchain->swapchain_image_format.format,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
    };

    // Allocate memory for it
    swapchain->swapchain_image_views = rg_malloc(sizeof(VkImageView) * swapchain->image_count);

    // Create the image views
    for (uint32_t i = 0; i < swapchain->image_count; i++)
    {
        image_view_create_info.image = swapchain->swapchain_images[i];
        vk_check(vkCreateImageView(renderer->device, &image_view_create_info, NULL, &swapchain->swapchain_image_views[i]),
                 "Couldn't create image views for the swapchain images.");
    }
    // endregion

    // region Depth image creation

    VkExtent3D depth_image_extent = {
        .width  = swapchain->viewport_extent.width,
        .height = swapchain->viewport_extent.height,
        .depth  = 1,
    };
    swapchain->depth_image_format = VK_FORMAT_D32_SFLOAT;

    swapchain->depth_image = rg_renderer_create_image(renderer->allocator,
                                                      renderer->device,
                                                      swapchain->depth_image_format,
                                                      depth_image_extent,
                                                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                      VK_IMAGE_ASPECT_DEPTH_BIT,
                                                      VMA_MEMORY_USAGE_GPU_ONLY);

    // endregion

    // region Framebuffers creation

    swapchain->swapchain_image_framebuffers = rg_malloc(sizeof(VkFramebuffer) * swapchain->image_count);

    VkFramebufferCreateInfo framebuffer_create_info = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = renderer->passes.lighting_pass,
        .attachmentCount = 1,
        .width           = extent.width,
        .height          = extent.height,
        .layers          = 1,
    };

    for (uint32_t i = 0; i < swapchain->image_count; i++)
    {
        framebuffer_create_info.pAttachments = &swapchain->swapchain_image_views[i];
        vk_check(vkCreateFramebuffer(renderer->device, &framebuffer_create_info, NULL, &swapchain->swapchain_image_framebuffers[i]),
                 "Couldn't create framebuffers for the swapchain images.");
    }

    // endregion
}

void rg_renderer_add_window(rg_renderer *renderer, uint32_t window_index, rg_window *window)
{
    // Prevent access to the positions that are outside the swapchain array
    rg_renderer_check(window_index < renderer->swapchains.count, "Swapchain index out of range.");

    // Get the swapchain in the renderer
    rg_swapchain *swapchain = ((rg_swapchain *) renderer->swapchains.data) + window_index;

    // Ensure that there is not a live swapchain here already
    rg_renderer_check(!swapchain->enabled,
                      "Attempted to create a swapchain in a slot where there was already an active one. To recreate a swapchain, see "
                      "rg_renderer_recreate_swapchain.");

    // Save renderer to be able to access it from events
    swapchain->renderer = renderer;

    // region Window & Surface

    // Get the window's surface
    swapchain->target_window = window;
    swapchain->surface       = rg_window_get_vulkan_surface(window, renderer->instance);

    // Check that the surface is supported
    VkBool32 surface_supported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(renderer->physical_device,
                                         renderer->graphics_queue.family_index,
                                         swapchain->surface,
                                         &surface_supported);
    rg_renderer_check(surface_supported, "The chosen GPU is unable to render to the given surface.");

    // endregion

    // region Present mode

    // Choose a present mode
    uint32_t available_present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->physical_device,
                                              swapchain->surface,
                                              &available_present_modes_count,
                                              VK_NULL_HANDLE);
    rg_array available_present_modes = rg_create_array(available_present_modes_count, sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->physical_device,
                                              swapchain->surface,
                                              &available_present_modes_count,
                                              available_present_modes.data);

    bool present_mode_found = false;
#define DESIRED_PRESENT_MODES_COUNT 2
    VkPresentModeKHR desired_present_modes[DESIRED_PRESENT_MODES_COUNT] = {
        VK_PRESENT_MODE_MAILBOX_KHR,
        VK_PRESENT_MODE_FIFO_KHR,
    };
    // Find a suited present mode
    for (uint32_t i = 0; i < available_present_modes_count && !present_mode_found; i++)
    {
        VkPresentModeKHR mode = ((VkPresentModeKHR *) available_present_modes.data)[i];
        for (uint32_t j = 0; j < DESIRED_PRESENT_MODES_COUNT && !present_mode_found; j++)
        {
            // Found a match ! Take it
            if (mode == desired_present_modes[j])
            {
                swapchain->present_mode = mode;
                present_mode_found      = true;
            }
        }
    }
    rg_destroy_array(&available_present_modes);
    rg_renderer_check(present_mode_found, "Couldn't find a supported present mode for this surface.");

    // endregion

    // region Image count selection

    // Check the surface capabilities
    VkSurfaceCapabilitiesKHR surface_capabilities = {0};
    vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->physical_device, swapchain->surface, &surface_capabilities), NULL);

    // For the image count, take the minimum plus one. Or, if the minimum is equal to the maximum, take that value.
    uint32_t image_count = surface_capabilities.minImageCount + 1;
    if (image_count > surface_capabilities.maxImageCount)
    {
        image_count = surface_capabilities.maxImageCount;
    }
    swapchain->image_count = image_count;
    swapchain->transform   = surface_capabilities.currentTransform;

    // endregion

    swapchain->pipelines             = rg_create_hash_map();
    swapchain->built_effects_version = 0;

    swapchain->swapchain_image_format = rg_select_surface_format(renderer, window);

    // Get window extent
    rg_extent_2d extent = rg_window_get_current_extent(window);

    // Create the swapchain and the images
    rg_init_swapchain_inner(renderer, swapchain, extent);

    // region Init render stages

    // Stages are hardcoded for now
    // Using a dynamic array allows us to define a parameter to this function to define the stages
    swapchain->render_stages = rg_create_array(RENDER_STAGE_COUNT, sizeof(rg_render_stage));

    // Init stages
    for (uint32_t i = 0; i < RENDER_STAGE_COUNT; i++)
    {
        rg_render_stage *stage = &((rg_render_stage *) swapchain->render_stages.data)[i];

        // Init vector
        rg_create_vector(5, sizeof(rg_render_batch), &stage->batches);

        // Empty buffer (will be created later)
        stage->indirect_buffer = (rg_allocated_buffer) {
            .buffer     = VK_NULL_HANDLE,
            .allocation = NULL,
            .size       = 0,
        };
    }
    (((rg_render_stage *) swapchain->render_stages.data)[0]).kind = RG_RENDER_STAGE_KIND_GEOMETRY;
    (((rg_render_stage *) swapchain->render_stages.data)[1]).kind = RG_RENDER_STAGE_KIND_LIGHTING;

    // endregion

    // region Register to window events

    swapchain->window_resize_event_handler_id =
        rg_window_resize_event_subscribe(window,
                                         (rg_event_handler) {
                                             .pfn_handler = (rg_event_handler_function) rg_renderer_handle_window_resize_event,
                                             .user_data   = swapchain,
                                         });
    rg_renderer_check(swapchain->window_resize_event_handler_id != RG_EVENT_HANDLER_NULL_ID,
                      "Couldn't subscribe to window resize events.");

    // endregion

    // Enable it
    swapchain->enabled = true;
}

void rg_destroy_swapchain_inner(rg_swapchain *swapchain)
{
    // Destroys a Vulkan Swapchain, but not everything in the rg_swapchain
    // For example, the surface is not deleted
    // The image arrays stay allocated

    rg_renderer *renderer = swapchain->renderer;

    // Destroy the framebuffers
    for (uint32_t i = 0; i < swapchain->image_count; i++)
    {
        vkDestroyFramebuffer(renderer->device, swapchain->swapchain_image_framebuffers[i], NULL);
    }

    // Destroy depth image
    rg_renderer_destroy_image(renderer->allocator, renderer->device, &swapchain->depth_image);

    // Destroy swapchain images
    for (uint32_t i = 0; i < swapchain->image_count; i++)
    {
        // Delete the image views
        vkDestroyImageView(renderer->device, swapchain->swapchain_image_views[i], NULL);
        swapchain->swapchain_image_views[i] = VK_NULL_HANDLE;
        // The deletion of the images is handled by the swapchain
    }

    // Destroy swapchain itself
    vkDestroySwapchainKHR(renderer->device, swapchain->vk_swapchain, NULL);
    swapchain->vk_swapchain = VK_NULL_HANDLE;

    // Clean the arrays
    rg_free(swapchain->swapchain_image_framebuffers);
    rg_free(swapchain->swapchain_image_views);
    rg_free(swapchain->swapchain_images);
}

void rg_destroy_swapchain(rg_swapchain *swapchain)
{
    // Unregister window events
    rg_window_resize_event_unsubscribe(swapchain->target_window, swapchain->window_resize_event_handler_id);

    // Destroy render stages
    for (uint32_t i = 0; i < swapchain->render_stages.count; i++)
    {
        rg_render_stage *stage = &((rg_render_stage *) swapchain->render_stages.data)[i];
        rg_destroy_vector(&stage->batches);
        rg_renderer_destroy_buffer(swapchain->renderer->allocator, &stage->indirect_buffer);
    }
    rg_destroy_array(&swapchain->render_stages);

    // Cleanup
    rg_destroy_swapchain_inner(swapchain);

    // Destroy the surface
    vkDestroySurfaceKHR(swapchain->renderer->instance, swapchain->surface, NULL);

    // Destroy the pipelines
    rg_renderer_clear_pipelines(swapchain);
    rg_destroy_hash_map(&swapchain->pipelines);

    // Disable it
    swapchain->enabled = false;
}

void rg_renderer_recreate_swapchain(rg_swapchain *swapchain, rg_extent_2d new_extent)
{
    // Ensure that there is a swapchain to recreate
    rg_renderer_check(swapchain->enabled,
                      "Attempted to recreate an non-existing swapchain. "
                      "Use rg_renderer_add_window to create a new one instead.");

    rg_renderer *renderer = swapchain->renderer;

    // Wait for all fences
    rg_renderer_wait_for_all_fences(renderer);

    // Destroy the old swapchain
    rg_destroy_swapchain_inner(swapchain);

    // Create the new swapchain
    rg_init_swapchain_inner(renderer, swapchain, new_extent);

    // Recreate the pipelines
    rg_renderer_recreate_pipelines(swapchain);
}

uint32_t rg_renderer_get_next_swapchain_image(rg_swapchain *swapchain)
{
    // Get current frame
    rg_frame_data *current_frame = rg_renderer_get_current_frame(swapchain->renderer);

    // Get the next image
    uint32_t next_image_index = 0;
    vk_check(vkAcquireNextImageKHR(swapchain->renderer->device,
                                   swapchain->vk_swapchain,
                                   SEMAPHORE_TIMEOUT,
                                   current_frame->present_semaphore,
                                   VK_NULL_HANDLE,
                                   &next_image_index),
             NULL);

    return next_image_index;
}
// endregion

// region Material templates functions

rg_material_template_id
    rg_renderer_create_material_template(rg_renderer *renderer, rg_shader_effect_id *available_effects, uint32_t effect_count)
{
    // Basic checks
    rg_renderer_check(effect_count > 0, "Attempted to create a material template with 0 effects");
    rg_renderer_check(renderer != NULL, NULL);
    rg_renderer_check(available_effects != NULL, NULL);

    // Create material template
    rg_material_template material_template = {
        .shader_effects = rg_create_array(effect_count, sizeof(rg_shader_effect_id)),
    };
    rg_renderer_check(material_template.shader_effects.data != NULL, "Couldn't create array of shader effects");

    // Copy effects
    rg_renderer_check(memcpy(material_template.shader_effects.data, available_effects, effect_count * sizeof(rg_shader_effect_id))
                          != NULL,
                      "Couldn't copy shader effects");

    // Store material template
    rg_material_template_id template_id = rg_storage_push(renderer->material_templates, &material_template);
    rg_renderer_check(template_id != RG_STORAGE_NULL_ID, "Couldn't store material template");

    return template_id;
}

/**
 * Destroys the given material template.
 * @param renderer the renderer to use
 * @param material_template_id the id of the material template to destroy
 */
void rg_renderer_destroy_material_template(rg_renderer *renderer, rg_material_template_id material_template_id)
{
    // Get material template
    rg_material_template *template = rg_storage_get(renderer->material_templates, material_template_id);
    if (template != NULL)
    {
        // Destroy array
        rg_destroy_array(&template->shader_effects);

        // Remove from map
        rg_storage_erase(renderer->material_templates, material_template_id);
    }
}

void rg_renderer_clear_material_templates(rg_renderer *renderer)
{
    if (renderer->material_templates != NULL)
    {
        // Clean content of storage
        rg_storage_it it = rg_storage_iterator(renderer->material_templates);
        while (rg_storage_next(&it))
        {
            rg_material_template *template = it.value;

            rg_destroy_array(&template->shader_effects);
        }

        // Clean storage itself
        rg_destroy_storage(&renderer->material_templates);
    }
}

// endregion

// region Materials functions

rg_material_id rg_renderer_create_material(rg_renderer *renderer, rg_material_template_id material_template_id)
{
    // Basic checks
    rg_renderer_check(renderer != NULL, NULL);
    rg_renderer_check(material_template_id != RG_STORAGE_NULL_ID, NULL);

    // Create material
    rg_material material = {
        .material_template_id = material_template_id,
        .models_using_material =
            {
                .data     = NULL,
                .count    = 0,
                .capacity = 0,
            },
    };
    // Init vector
    rg_renderer_check(rg_create_vector(10, sizeof(rg_model_id), &material.models_using_material), NULL);

    // Store material
    rg_material_id material_id = rg_storage_push(renderer->materials, &material);
    rg_renderer_check(material_id != RG_STORAGE_NULL_ID, "Couldn't store material");

    return material_id;
}

void rg_renderer_destroy_material(rg_renderer *renderer, rg_material_id material_id)
{
    // Get material
    rg_material *material = rg_storage_get(renderer->materials, material_id);
    if (material != NULL)
    {
        // Destroy vector
        rg_destroy_vector(&material->models_using_material);

        // Remove from map
        rg_storage_erase(renderer->materials, material_id);
    }
}

void rg_renderer_clear_materials(rg_renderer *renderer)
{
    if (renderer->materials != NULL)
    {
        // Clean content of storage
        rg_storage_it it = rg_storage_iterator(renderer->materials);
        while (rg_storage_next(&it))
        {
            rg_material *material = it.value;

            rg_destroy_vector(&material->models_using_material);
        }

        // Clean storage itself
        rg_destroy_storage(&renderer->materials);
    }
}

bool rg_renderer_material_register_model(rg_renderer *renderer, rg_material_id material_id, rg_model_id model_id)
{
    // Get the material
    rg_material *material = rg_storage_get(renderer->materials, material_id);
    if (material != NULL)
    {
        rg_vector_push_back(&material->models_using_material, &model_id);

        return true;
    }

    return false;
}

bool rg_renderer_material_unregister_model(rg_renderer *renderer, rg_material_id material_id, rg_model_id model_id)
{
    // Get the material
    rg_material *material = rg_storage_get(renderer->materials, material_id);
    if (material != NULL)
    {
        bool found = false;

        // Find the model
        rg_vector_it it = rg_vector_iterator(&material->models_using_material);
        while (rg_vector_next(&it))
        {
            rg_model_id *id = it.value;

            if (*id == model_id)
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            size_t index      = it.index;
            size_t last_index = rg_vector_last_index(&material->models_using_material);

            if (index < last_index)
            {
                // Copy last element to deleted slot
                rg_vector_copy(&material->models_using_material, last_index, index);
            }

            // Remove last element
            rg_vector_pop_back(&material->models_using_material);
        }

        return true;
    }

    return false;
}

// endregion

// region Model functions

rg_model_id rg_renderer_create_model(rg_renderer *renderer, rg_material_id material_id)
{
    // Basic checks
    rg_renderer_check(renderer != NULL, NULL);
    rg_renderer_check(material_id != RG_STORAGE_NULL_ID, NULL);

    // Create model
    rg_model model = {
        .material_id = material_id,
        .instances =
            {
                .data     = NULL,
                .count    = 0,
                .capacity = 0,
            },
    };

    // Init vector
    rg_renderer_check(rg_create_vector(10, sizeof(rg_render_node_id), &model.instances), NULL);

    // Store model
    rg_model_id model_id = rg_storage_push(renderer->models, &model);
    rg_renderer_check(model_id != RG_STORAGE_NULL_ID, "Couldn't store model");

    // Register it in the material
    rg_renderer_material_register_model(renderer, material_id, model_id);

    return model_id;
}

void rg_renderer_destroy_model(rg_renderer *renderer, rg_model_id model_id)
{
    // Get model
    rg_model *model = rg_storage_get(renderer->models, model_id);
    if (model != NULL)
    {
        // Destroy vector
        rg_destroy_vector(&model->instances);

        // Remove from map
        rg_storage_erase(renderer->models, model_id);

        // Unregister it
        rg_renderer_material_unregister_model(renderer, model->material_id, model_id);
    }
}

void rg_renderer_clear_models(rg_renderer *renderer)
{
    if (renderer->models != NULL)
    {
        // Clean content of storage
        rg_storage_it it = rg_storage_iterator(renderer->models);
        while (rg_storage_next(&it))
        {
            rg_model *model = it.value;

            rg_destroy_vector(&model->instances);
        }

        // Clean storage itself
        rg_destroy_storage(&renderer->models);
    }
}

void rg_renderer_model_register_instance(rg_renderer *renderer, rg_model_id model_id, rg_render_node_id render_node_id)
{
    // Get model
    rg_model *model = rg_storage_get(renderer->models, model_id);
    if (model != NULL)
    {
        rg_vector_push_back(&model->instances, &render_node_id);
    }
}

void rg_renderer_model_unregister_instance(rg_renderer *renderer, rg_model_id model_id, rg_render_node_id render_node_id)
{
    // Get model
    rg_model *model = rg_storage_get(renderer->models, model_id);
    if (model != NULL)
    {
        bool found = false;

        // Find the model
        rg_vector_it it = rg_vector_iterator(&model->instances);
        while (rg_vector_next(&it))
        {
            rg_render_node_id *id = it.value;

            if (*id == render_node_id)
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            size_t index      = it.index;
            size_t last_index = rg_vector_last_index(&model->instances);

            if (index < last_index)
            {
                // Copy last element to deleted slot
                rg_vector_copy(&model->instances, last_index, index);
            }

            // Remove last element
            rg_vector_pop_back(&model->instances);
        }
    }
}

// endregion

// region Render nodes functions

rg_render_node_id rg_renderer_create_render_node(rg_renderer *renderer, rg_model_id model_id)
{
    // Basic checks
    rg_renderer_check(renderer != NULL, NULL);
    rg_renderer_check(model_id != RG_STORAGE_NULL_ID, NULL);

    // Create render node
    rg_render_node node = {
        .model_id = model_id,
    };

    // Store render node
    rg_render_node_id render_node_id = rg_storage_push(renderer->render_nodes, &node);

    // Register it in the model
    rg_renderer_model_register_instance(renderer, model_id, render_node_id);

    return render_node_id;
}

void rg_renderer_destroy_render_node(rg_renderer *renderer, rg_render_node_id render_node_id)
{
    // Get render node
    rg_render_node *node = rg_storage_get(renderer->render_nodes, render_node_id);
    if (node != NULL)
    {
        // Unregister it
        rg_renderer_model_unregister_instance(renderer, node->model_id, render_node_id);

        // Remove from storage
        rg_storage_erase(renderer->render_nodes, render_node_id);
    }
}

void rg_renderer_clear_render_nodes(rg_renderer *renderer)
{
    if (renderer->render_nodes != NULL)
    {
        // Clean storage itself
        rg_destroy_storage(&renderer->render_nodes);
    }
}

// endregion

// region Stages functions

void rg_renderer_update_stage_cache(rg_swapchain *swapchain)
{
    rg_renderer *renderer = swapchain->renderer;

    // For each stage
    for (uint32_t i = 0; i < swapchain->render_stages.count; i++)
    {
        // Get stage
        rg_render_stage *stage = &((rg_render_stage *) swapchain->render_stages.data)[i];

        // Clear cache
        rg_vector_clear(&stage->batches);

        // Find the models using the materials using a template using an effect matching the stage
        rg_vector models = {0, 0, 0, NULL, 0};
        rg_renderer_check(rg_create_vector(5, sizeof(rg_model_id), &models), NULL);

        rg_storage_it effects_it = rg_storage_iterator(renderer->shader_effects);
        while (rg_storage_next(&effects_it))
        {
            // If the effect supports that kind, add it
            if (((rg_shader_effect *) effects_it.value)->render_stage_kind == stage->kind)
            {
                // For each material template
                rg_storage_it material_templates_it = rg_storage_iterator(renderer->material_templates);
                while (rg_storage_next(&material_templates_it))
                {
                    // If the material template has that effect, add it
                    rg_material_template *template = material_templates_it.value;
                    for (uint32_t j = 0; j < template->shader_effects.count; j++)
                    {
                        if (((rg_shader_effect_id *) template->shader_effects.data)[j] == effects_it.id)
                        {
                            // For each material
                            rg_storage_it materials_it = rg_storage_iterator(renderer->materials);
                            while (rg_storage_next(&materials_it))
                            {
                                // If the material has that template, add it
                                rg_material *material = materials_it.value;
                                if (material->material_template_id == material_templates_it.id)
                                {
                                    // Get the pipeline for that effect
                                    rg_hash_map_get_result pipeline_get_result = rg_hash_map_get(swapchain->pipelines, effects_it.id);
                                    rg_renderer_check(pipeline_get_result.exists, NULL);

                                    // Add a batch
                                    rg_render_batch batch = {
                                        .count    = material->models_using_material.count,
                                        .offset   = models.count,
                                        .pipeline = pipeline_get_result.value.as_ptr,
                                    };
                                    rg_vector_push_back(&stage->batches, &batch);

                                    // Add the models using that material
                                    rg_vector_extend(&models,
                                                     &material->models_using_material.data,
                                                     material->models_using_material.count);

                                    break;
                                }
                            }

                            break;
                        }
                    }
                }
            }
        }

        // If there is something to render
        if (models.count > 0)
        {
            // Prepare draw indirect commands
            const VkBufferUsageFlags indirect_buffer_usage =
                VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            const VmaMemoryUsage indirect_buffer_memory_usage  = VMA_MEMORY_USAGE_CPU_TO_GPU;
            const size_t         required_indirect_buffer_size = models.count * sizeof(VkDrawIndirectCommand);

            // If it does not exist, create it
            if (stage->indirect_buffer.buffer == VK_NULL_HANDLE)
            {
                stage->indirect_buffer = rg_renderer_create_buffer(renderer->allocator,
                                                                   required_indirect_buffer_size,
                                                                   indirect_buffer_usage,
                                                                   indirect_buffer_memory_usage);
            }
            // If it exists but isn't big enough, recreate it
            else if (stage->indirect_buffer.size < required_indirect_buffer_size)
            {
                rg_renderer_destroy_buffer(renderer->allocator, &stage->indirect_buffer);
                stage->indirect_buffer = rg_renderer_create_buffer(renderer->allocator,
                                                                   required_indirect_buffer_size,
                                                                   indirect_buffer_usage,
                                                                   indirect_buffer_memory_usage);
            }

            // At this point, we have an indirect buffer big enough to hold the commands we want to register

            // Register commands
            VkDrawIndirectCommand *indirect_commands = rg_renderer_map_buffer(renderer->allocator, &stage->indirect_buffer);

            for (uint32_t j = 0; j < models.count; j++)
            {
                rg_model *model = rg_vector_get_element(&models, j);

                indirect_commands[j].vertexCount   = 3; // TODO when mesh is added
                indirect_commands[j].firstVertex   = 0;
                indirect_commands[j].instanceCount = 1; // TODO when instances are added
                indirect_commands[j].firstInstance = 0;
            }
            rg_renderer_unmap_buffer(renderer->allocator, &stage->indirect_buffer);
        }

        // Clean up
        rg_destroy_vector(&models);
    }
}

void rg_renderer_draw_from_cache(rg_render_stage *stage, VkCommandBuffer cmd)
{
    const uint32_t draw_stride = sizeof(VkDrawIndirectCommand);

    VkPipeline bound_pipeline = VK_NULL_HANDLE;

    // For each batch
    rg_vector_it it = rg_vector_iterator(&stage->batches);
    while (rg_vector_next(&it))
    {
        rg_render_batch *batch = it.value;

        // If the pipeline is different from the last one, bind it
        if (bound_pipeline != batch->pipeline)
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, batch->pipeline);
            bound_pipeline = batch->pipeline;
        }

        // Draw batch of commands
        uint32_t draw_offset = draw_stride * batch->offset;
        vkCmdDrawIndirect(cmd, stage->indirect_buffer.buffer, draw_offset, batch->count, draw_stride);
    }
}

// endregion

// region Renderer functions

rg_renderer *rg_create_renderer(rg_window  *example_window,
                                const char *application_name,
                                rg_version  application_version,
                                uint32_t    window_capacity)
{
    // Create a renderer in the heap
    // This is done to keep the renderer opaque in the header
    // Use calloc to set all bytes to 0
    rg_renderer *renderer = rg_calloc(1, sizeof(rg_renderer));
    if (renderer == NULL)
    {
        return NULL;
    }

    // Initialize volk
    vk_check(volkInitialize(), "Couldn't initialize Volk.");

    // --=== Instance creation ===--

    // region Instance creation

    // Set required extensions
    uint32_t extra_extension_count = 0;
#ifdef USE_VK_VALIDATION_LAYERS
    extra_extension_count += 1;
#endif

    // Get the extensions that the window manager needs
    rg_array required_extensions = rg_window_get_required_vulkan_extensions(example_window, extra_extension_count);

    // Add other extensions in the extra slots
    uint32_t extra_ext_index = required_extensions.count - extra_extension_count;
#ifdef USE_VK_VALIDATION_LAYERS
    ((rg_string *) required_extensions.data)[extra_ext_index++] = RG_CSTR_CONST(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    rg_renderer_check(rg_renderer_check_instance_extension_support(required_extensions.data, required_extensions.count),
                      "Not all required Vulkan extensions are supported.");

    // Convert to cstr array
    rg_array required_extension_cstrs = rg_string_array_to_cstr_array(required_extensions.data, required_extensions.count);

    // Get the validations layers if needed
#ifdef USE_VK_VALIDATION_LAYERS
#define ENABLED_LAYERS_COUNT 1
    rg_string required_validation_layers[ENABLED_LAYERS_COUNT] = {
        RG_CSTR_CONST("VK_LAYER_KHRONOS_validation"),
    };
    rg_renderer_check(rg_renderer_check_layer_support(required_validation_layers, ENABLED_LAYERS_COUNT),
                      "Vulkan validation layers requested, but not available.");
    // Convert to cstrs
    rg_array required_validation_layers_cstrs = rg_string_array_to_cstr_array(required_validation_layers, ENABLED_LAYERS_COUNT);
#endif

    VkApplicationInfo applicationInfo = {
        // Struct infos
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = VK_NULL_HANDLE,
        // Engine infos
        .apiVersion    = VULKAN_API_VERSION,
        .engineVersion = VK_MAKE_VERSION(RG_ENGINE_VERSION.major, RG_ENGINE_VERSION.minor, RG_ENGINE_VERSION.patch),
        .pEngineName   = "Railguard",
        // Application infos
        .applicationVersion = VK_MAKE_VERSION(application_version.major, application_version.minor, application_version.patch),
        .pApplicationName   = application_name,
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        // Struct infos
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,

        // App info
        .pApplicationInfo = &applicationInfo,

        // Extensions
        .enabledExtensionCount   = required_extension_cstrs.count,
        .ppEnabledExtensionNames = required_extension_cstrs.data,

    // Validation layers
#ifdef USE_VK_VALIDATION_LAYERS
        .enabledLayerCount   = ENABLED_LAYERS_COUNT,
        .ppEnabledLayerNames = required_validation_layers_cstrs.data,
#else
        .enabledLayerCount   = 0,
        .ppEnabledLayerNames = NULL,
#endif

    };
    vk_check(vkCreateInstance(&instanceCreateInfo, NULL, &renderer->instance), "Couldn't create instance.");

    // Cleanup instance creation
    rg_destroy_array(&required_extensions);
    rg_destroy_array(&required_extension_cstrs);
#ifdef USE_VK_VALIDATION_LAYERS
    rg_destroy_array(&required_validation_layers_cstrs);
#endif

    // Register instance in Volk
    volkLoadInstance(renderer->instance);

    // Create debug messenger
#ifdef USE_VK_VALIDATION_LAYERS
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {
        // Struct info
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = VK_NULL_HANDLE,
        // Message settings
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        // Callback
        .pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT) rg_renderer_debug_messenger_callback,
    };
    vk_check(vkCreateDebugUtilsMessengerEXT(renderer->instance, &debug_messenger_create_info, NULL, &renderer->debug_messenger),
             "Couldn't create debug messenger");

#endif

    // endregion

    // --=== Physical device and queue families selection ===--

    // region Physical device and queue families selection

    // Get the number of available devices
    uint32_t available_physical_devices_count = 0;
    vkEnumeratePhysicalDevices(renderer->instance, &available_physical_devices_count, NULL);

    // Create an array big enough to hold everything and get the devices themselves
    rg_array available_physical_devices = rg_create_array(available_physical_devices_count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(renderer->instance, &available_physical_devices_count, available_physical_devices.data);

    // Find the best physical device
    // For that, we will assign each device a score and keep the best one
    uint32_t current_max_score = 0;
    for (uint32_t i = 0; i < available_physical_devices_count; i++)
    {
        VkPhysicalDevice checked_device = ((VkPhysicalDevice *) available_physical_devices.data)[i];
        uint32_t         score          = rg_renderer_rate_physical_device(checked_device);

        if (score > current_max_score)
        {
            // New best device found, save it.
            // We don't need to keep the previous one, since we definitely won't choose it.
            current_max_score         = score;
            renderer->physical_device = checked_device;
        }
    }

    // There is a problem if the device is still null: it means none was found.
    rg_renderer_check(renderer->physical_device != VK_NULL_HANDLE, "No suitable GPU was found.");

    // Cleanup physical device selection
    rg_destroy_array(&available_physical_devices);

    // Log chosen GPU
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(renderer->physical_device, &physical_device_properties);
    printf("Suitable GPU found: %s\n", physical_device_properties.deviceName);

    // Get queue families
    uint32_t queue_family_properties_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queue_family_properties_count, VK_NULL_HANDLE);
    rg_array queue_family_properties = rg_create_array(queue_family_properties_count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queue_family_properties_count, queue_family_properties.data);

    // Find the queue families that we need
    bool found_graphics_queue = false;

    for (uint32_t i = 0; i < queue_family_properties_count; i++)
    {
        VkQueueFamilyProperties family_properties = ((VkQueueFamilyProperties *) queue_family_properties.data)[i];

        // Save the graphics queue family_index
        if (family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            renderer->graphics_queue.family_index = i;
            found_graphics_queue                  = true;
        }
    }

    rg_renderer_check(found_graphics_queue, "Unable to find a graphics queue family_index.");

    // Cleanup queue families selection
    rg_destroy_array(&queue_family_properties);

    // endregion

    // --=== Logical device and queues creation ===--

    // region Device and queues creation

    // Define the parameters for the graphics queue
    float                   graphics_queue_priority    = 1.0f;
    VkDeviceQueueCreateInfo graphics_queue_create_info = {
        // Struct infos
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        // Queue info
        .queueCount       = 1,
        .queueFamilyIndex = renderer->graphics_queue.family_index,
        .pQueuePriorities = &graphics_queue_priority,
    };

    const char *required_device_extensions[REQUIRED_DEVICE_EXT_COUNT] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    // Create device
    VkDeviceCreateInfo device_create_info = {
        // Struct info
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        // Queues
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos    = &graphics_queue_create_info,
        // Extensions
        .enabledExtensionCount   = REQUIRED_DEVICE_EXT_COUNT,
        .ppEnabledExtensionNames = required_device_extensions,
        // Layers (deprecated for device, so we set it to NULL)
        .enabledLayerCount   = 0,
        .ppEnabledLayerNames = NULL,
    };
    vk_check(vkCreateDevice(renderer->physical_device, &device_create_info, NULL, &renderer->device),
             "Couldn't create logical device.");

    // Load device in volk
    volkLoadDevice(renderer->device);

    // Get created queues
    vkGetDeviceQueue(renderer->device, renderer->graphics_queue.family_index, 0, &renderer->graphics_queue.queue);

    // endregion

    // --=== Allocator ===--

    renderer->allocator = rg_renderer_create_allocator(renderer->instance, renderer->device, renderer->physical_device);

    // --=== Swapchains ===--

    // We need to create an array big enough to hold all the swapchains.
    // Use the zeroed variant because we do not initialize it, and we want it clean because to destroy the renderer, we check the
    // enabled field, and it must be zero by default.
    renderer->swapchains = rg_create_array_zeroed(window_capacity, sizeof(rg_swapchain));

    // --=== Render passes ===--

    // region Render passes creation

    // Choose a swapchain_image_format for the given example window
    // For now we will assume that all swapchain will use that swapchain_image_format
    VkSurfaceFormatKHR swapchain_image_format = rg_select_surface_format(renderer, example_window);

    // Create geometric render pass

    VkAttachmentReference   attachment_references[3] = {0};
    VkAttachmentDescription attachments[3]           = {{0}, {0}, {0}};

    // Position color buffer
    attachments[0] = (VkAttachmentDescription) {
        // Operators
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        // Layout
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        // No MSAA
        .samples = VK_SAMPLE_COUNT_1_BIT,
        // Format. We need high precision for position
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
    };
    attachment_references[0] = (VkAttachmentReference) {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    // Normal color buffer. Same format as position
    attachments[1]           = attachments[0];
    attachment_references[1] = (VkAttachmentReference) {
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    // Color + specular buffers
    attachments[2]           = attachments[0];
    attachments[2].format    = VK_FORMAT_R8G8B8A8_UINT;
    attachment_references[2] = (VkAttachmentReference) {
        .attachment = 2,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    // Group attachments in an array
    VkSubpassDescription subpass_description = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        // Color attachments
        .colorAttachmentCount = 3,
        .pColorAttachments    = attachment_references,
    };
    VkRenderPassCreateInfo render_pass_create_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext           = VK_NULL_HANDLE,
        .attachmentCount = 3,
        .pAttachments    = attachments,
        .subpassCount    = 1,
        .pSubpasses      = &subpass_description,
    };
    vk_check(vkCreateRenderPass(renderer->device, &render_pass_create_info, NULL, &renderer->passes.geometry_pass),
             "Couldn't create geometry render pass");

    // Create lighting render pass
    VkAttachmentDescription lighting_attachment = {
        // Operators
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        // Layout
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        // No MSAA
        .samples = VK_SAMPLE_COUNT_1_BIT,
        // Format. We need high precision for position
        .format = swapchain_image_format.format,
    };
    VkAttachmentReference lighting_attachment_reference = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments    = &lighting_attachment_reference;
    render_pass_create_info.attachmentCount  = 1;
    render_pass_create_info.pAttachments     = &lighting_attachment;
    vk_check(vkCreateRenderPass(renderer->device, &render_pass_create_info, NULL, &renderer->passes.lighting_pass),
             "Couldn't create lighting render pass");

    // endregion

    // --=== Init various storages ===--

    // Init shader_modules map
    renderer->shader_modules     = rg_create_storage(sizeof(rg_shader_module));
    renderer->shader_effects     = rg_create_storage(sizeof(rg_shader_effect));
    renderer->material_templates = rg_create_storage(sizeof(rg_material_template));
    renderer->materials          = rg_create_storage(sizeof(rg_material));
    renderer->models             = rg_create_storage(sizeof(rg_model));
    renderer->render_nodes       = rg_create_storage(sizeof(rg_render_node));
    renderer->effects_version    = 0;

    // --=== Init frames ===--

    // region Init frames

    renderer->current_frame_number = 1;

    // Define create infos
    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = VK_NULL_HANDLE,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = renderer->graphics_queue.family_index,
    };

    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
    };

    // For each frame
    for (uint32_t i = 0; i < NB_OVERLAPPING_FRAMES; i++)
    {
        // Create command pool
        vk_check(vkCreateCommandPool(renderer->device, &command_pool_create_info, NULL, &renderer->frames[i].command_pool),
                 "Couldn't create command pool");

        // Create command buffers
        VkCommandBufferAllocateInfo command_buffer_allocate_info = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = VK_NULL_HANDLE,
            .commandPool        = renderer->frames[i].command_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        vk_check(vkAllocateCommandBuffers(renderer->device, &command_buffer_allocate_info, &renderer->frames[i].command_buffer),
                 "Couldn't allocate command buffers");

        // Create fence
        vk_check(vkCreateFence(renderer->device, &fence_create_info, NULL, &renderer->frames[i].render_fence),
                 "Couldn't create fence");

        // Create semaphores
        vk_check(vkCreateSemaphore(renderer->device, &semaphore_create_info, NULL, &renderer->frames[i].present_semaphore),
                 "Couldn't create semaphore");
        vk_check(vkCreateSemaphore(renderer->device, &semaphore_create_info, NULL, &renderer->frames[i].render_semaphore),
                 "Couldn't create semaphore");
    }

    // endregion

    return renderer;
}

void rg_destroy_renderer(rg_renderer **renderer)
{
    // Wait
    rg_renderer_wait_for_all_fences(*renderer);

    // Clear frames
    for (uint32_t i = 0; i < NB_OVERLAPPING_FRAMES; i++)
    {
        // Destroy semaphores
        vkDestroySemaphore(renderer[0]->device, renderer[0]->frames[i].present_semaphore, NULL);
        vkDestroySemaphore(renderer[0]->device, renderer[0]->frames[i].render_semaphore, NULL);
        // Destroy fence
        vkDestroyFence(renderer[0]->device, renderer[0]->frames[i].render_fence, NULL);
        // Destroy command buffers
        vkFreeCommandBuffers(renderer[0]->device, renderer[0]->frames[i].command_pool, 1, &renderer[0]->frames[i].command_buffer);
        // Destroy command pool
        vkDestroyCommandPool(renderer[0]->device, renderer[0]->frames[i].command_pool, NULL);
    }

    // Clear render nodes
    rg_renderer_clear_render_nodes(*renderer);

    // Clear models
    rg_renderer_clear_models(*renderer);

    // Clear materials
    rg_renderer_clear_materials(*renderer);

    // Clear material templates
    rg_renderer_clear_material_templates(*renderer);

    // Clear shader effects
    rg_renderer_clear_shader_effects(*renderer);

    // Clear shader_modules
    rg_renderer_clear_shaders(*renderer);

    // Destroy swapchains
    for (uint32_t i = 0; i < (*renderer)->swapchains.count; i++)
    {
        rg_swapchain *swapchain = ((rg_swapchain *) (*renderer)->swapchains.data) + i;
        if (swapchain->enabled)
        {
            rg_destroy_swapchain(swapchain);
        }
    }

    // Destroy swapchain array
    rg_destroy_array(&(*renderer)->swapchains);

    // Destroy render passes
    vkDestroyRenderPass((*renderer)->device, (*renderer)->passes.geometry_pass, NULL);
    vkDestroyRenderPass((*renderer)->device, (*renderer)->passes.lighting_pass, NULL);

    // Destroy allocator
    rg_renderer_destroy_allocator(&(*renderer)->allocator);

    // Destroy device
    vkDestroyDevice((*renderer)->device, NULL);

#ifdef USE_VK_VALIDATION_LAYERS
    // Destroy debug messenger
    vkDestroyDebugUtilsMessengerEXT((*renderer)->instance, (*renderer)->debug_messenger, NULL);
#endif
    // Destroy instance
    vkDestroyInstance((*renderer)->instance, NULL);

    // Free the renderer
    rg_free(*renderer);

    // Since we took a double pointer in parameter, we can also set it to null
    // Thus, the user doesn't have to do it themselves
    *renderer = NULL;
}

void rg_renderer_draw(rg_renderer *renderer)
{
    // Get current frame
    uint64_t       current_frame_index = rg_renderer_get_current_frame_index(renderer);
    rg_frame_data *current_frame       = &renderer->frames[current_frame_index];

    // Wait for the fence
    rg_renderer_wait_for_fence(renderer, current_frame->render_fence);

    for (uint32_t i = 0; i < renderer->swapchains.count; i++)
    {
        rg_swapchain *swapchain = &((rg_swapchain *) renderer->swapchains.data)[i];

        if (swapchain->enabled)
        {
            // Update pipelines if needed
            rg_renderer_build_out_of_date_effects(swapchain);

            // Update render stages cache
            rg_renderer_update_stage_cache(swapchain);

            // Begin recording
            rg_renderer_begin_recording(renderer);

            // Get swapchain image
            uint32_t image_index = rg_renderer_get_next_swapchain_image(swapchain);

            // Geometric pass TODO

            // Lighting pass
            VkClearValue clear_value = {
                .color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}},
            };
            VkRenderPassBeginInfo render_pass_begin_info = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext = NULL,
                // Link framebuffer
                .framebuffer = swapchain->swapchain_image_framebuffers[image_index],
                // Render pass
                .renderPass = renderer->passes.lighting_pass,
                // Render area
                .renderArea = {.offset = {0, 0}, .extent = swapchain->viewport_extent},
                // Clear values
                .clearValueCount = 1,
                .pClearValues    = &clear_value,
            };
            vkCmdBeginRenderPass(current_frame->command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

            // Render
            rg_renderer_draw_from_cache(&((rg_render_stage *) swapchain->render_stages.data)[1], current_frame->command_buffer);

            vkCmdEndRenderPass(current_frame->command_buffer);


            // End recording
            rg_renderer_end_recording_and_submit(renderer);

            // Present
            VkPresentInfoKHR present_info = {
                .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext              = NULL,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores    = &current_frame->render_semaphore,
                .swapchainCount     = 1,
                .pSwapchains        = &swapchain->vk_swapchain,
                .pImageIndices      = &image_index,
            };
            vkQueuePresentKHR(renderer->graphics_queue.queue, &present_info);
        }
    }

    // Increment frame index
    renderer->current_frame_number++;
}

// endregion

#endif
