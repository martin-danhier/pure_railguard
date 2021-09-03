// Place the contents of the file inside a guard, so that only one implementation is generated
#ifdef RENDERER_VULKAN

#include "railguard/rendering/renderer.h"
#include <railguard/rendering/window.h>
#include <railguard/utils/arrays.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <volk.h>
// Needs to be after volk
#include <vk_mem_alloc.h>

#if USE_VK_VALIDATION_LAYERS

#include <string.h>

#endif

// --==== SETTINGS ====--

// region Settings

/// \brief Number of frames that can be rendered at the same time. Set to 2 for double buffering, or 3 for triple buffering.
#define NB_OVERLAPPING_FRAMES 3

#define VULKAN_API_VERSION VK_API_VERSION_1_1

// endregion

// --==== STRUCTS ====--

// region Struct definitions

// = Buffers and image =

typedef struct rg_allocated_buffer {
    VmaAllocation allocation;
    VkBuffer buffer;
    uint32_t size;
} rg_allocated_buffer;

typedef struct rg_allocated_image {
    VmaAllocation allocation;
    VkImage image;
    VkImageView image_view;
} rg_allocated_image;

// = Vulkan API related =

typedef struct rg_queue {
    uint32_t family_index;
    VkQueue queue;
} rg_queue;

typedef struct rg_swapchain {
    bool enabled;
    VkSwapchainKHR vk_swapchain;
    VkExtent2D viewport_extent;
    // Swapchain images
    uint32_t image_count;
    VkSurfaceFormatKHR swapchain_image_format;
    // All of these are arrays of image_count items:
    VkImage *swapchain_images;
    VkImageView *swapchain_image_views;
    VkFramebuffer *swapchain_image_framebuffers;
    // Depth image
    VkFormat depth_image_format;
    rg_allocated_image depth_image;

} rg_swapchain;

typedef struct rg_passes {
    VkRenderPass geometry_pass;
    VkRenderPass lighting_pass;
} rg_passes;

// = Global renderer =

typedef struct rg_renderer {
    VkDevice device;
    VkInstance instance;
    VkPhysicalDevice physical_device;
    rg_queue graphics_queue;
    VmaAllocator allocator;
    // TODO use a dynamic structure to be able to store a variable amount of surface for deletion
    // Maybe create a sub struct specialized to keep track of objects for deletion
    VkSurfaceKHR surface;
#ifdef USE_VK_VALIDATION_LAYERS
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
    /**
     * @brief Fixed-size array containing the swapchains.
     * We place them in a array to be able to efficiently iterate through them. And because their number won't change, we can safely
     * send pointers to individual swapchains around.
     */
    rg_array swapchains;
} rg_renderer;

// endregion

// --==== UTILS FUNCTIONS ====--

// region Error handling functions

const char *vk_result_to_str(VkResult result) {
    switch (result) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        default:
            return "N"; // We just need a character that is different from VK_ ... because I don't want to convert the number to string
    }

}

void vk_check(VkResult result, const char *error_message) {
    if (result != VK_SUCCESS) {
        // Pretty print error
        const char *result_str = vk_result_to_str(result);
        if (result_str[0] == 'N') {
            fprintf(stderr, "[Vulkan Error] A Vulkan function call returned VkResult = %d !\n", result);
        } else {
            fprintf(stderr, "[Vulkan Error] A Vulkan function call returned %s !\n", result_str);
        }
        // Optional custom error message precision
        if (error_message != NULL) {
            fprintf(stderr, "Precision: %s\n", error_message);
        }
        exit(1);
    }
}

void rg_check(bool result, const char *error_message) {
    if (!result) {
        fprintf(stderr, "[Error] %s\n", error_message);
        exit(1);
    }
}

// endregion

// region Extensions and layers functions

/**
 * @brief Checks if the given extensions are supported
 * @param desired_extensions is an array of desired_extensions_count layer names (null terminated strings)
 * @param desired_extensions_count is the number of extension names to check
 * @return true if every extension is supported, false otherwise.
 */
bool check_instance_extension_support(const char *const *desired_extensions, uint32_t desired_extensions_count) {
    // Get the number of available desired_extensions
    uint32_t available_extensions_count = 0;
    vk_check(vkEnumerateInstanceExtensionProperties(NULL, &available_extensions_count, VK_NULL_HANDLE), NULL);
    // Create an array with enough room and fetch the available desired_extensions
    rg_array available_extensions = rg_create_array(available_extensions_count, sizeof(VkExtensionProperties));
    vk_check(vkEnumerateInstanceExtensionProperties(NULL, &available_extensions_count, available_extensions.data),
             NULL);

    // For each desired extension, rg_check if it is available
    bool valid = true;
    for (uint32_t i = 0; i < desired_extensions_count && valid; i++) {
        bool found = false;

        // Search available until the desired is found or not
        for (uint32_t j = 0; j < available_extensions_count && !found; j++) {
            if (strcmp(desired_extensions[i], ((VkExtensionProperties *) available_extensions.data)[j].extensionName) ==
                0) {
                found = true;
            }
        }

        // Stop looping if nothing was found
        if (!found) {
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
bool check_device_extension_support(VkPhysicalDevice physical_device,
                                    const char *const *desired_extensions,
                                    uint32_t desired_extensions_count) {
    // Get the number of available desired_extensions
    uint32_t available_extensions_count = 0;
    vk_check(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &available_extensions_count, VK_NULL_HANDLE),
             NULL);
    // Create an array with enough room and fetch the available desired_extensions
    rg_array available_extensions = rg_create_array(available_extensions_count, sizeof(VkExtensionProperties));
    vk_check(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &available_extensions_count,
                                                  available_extensions.data),
             NULL);

    // For each desired extension, rg_check if it is available
    bool valid = true;
    for (uint32_t i = 0; i < desired_extensions_count && valid; i++) {
        bool found = false;

        // Search available until the desired is found or not
        for (uint32_t j = 0; j < available_extensions_count && !found; j++) {
            if (strcmp(desired_extensions[i], ((VkExtensionProperties *) available_extensions.data)[j].extensionName) ==
                0) {
                found = true;
            }
        }

        // Stop looping if nothing was found
        if (!found) {
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
bool check_layer_support(const char *const *desired_layers, uint32_t desired_layers_count) {
    // Get the number of available desired_layers
    uint32_t available_layers_count = 0;
    vk_check(vkEnumerateInstanceLayerProperties(&available_layers_count, VK_NULL_HANDLE), NULL);
    // Create an array with enough room and fetch the available desired_layers
    rg_array available_layers = rg_create_array(available_layers_count, sizeof(VkLayerProperties));
    vk_check(vkEnumerateInstanceLayerProperties(&available_layers_count, available_layers.data), NULL);

    // For each desired layer, rg_check if it is available
    bool valid = true;
    for (uint32_t i = 0; i < desired_layers_count && valid; i++) {
        bool found = false;

        // Search available until the desired is found or not
        for (uint32_t j = 0; j < available_layers_count && !found; j++) {
            if (strcmp(desired_layers[i], ((VkLayerProperties *) available_layers.data)[j].layerName) == 0) {
                found = true;
            }
        }

        // Stop looping if nothing was found
        if (!found) {
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
VKAPI_ATTR VkBool32 VKAPI_CALL *debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                         VkDebugUtilsMessageTypeFlagsEXT message_types,
                                                         const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                         void *user_data) {
    // Inspired by VkBootstrap's default debug messenger. (Made by Charles Giessen)

    // Get severity
    char *str_severity;
    switch (message_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            str_severity = "VERBOSE";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            str_severity = "ERROR";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            str_severity = "WARNING";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            str_severity = "INFO";
            break;
        default:
            str_severity = "UNKNOWN";
            break;
    }

    // Get type
    char *str_type;
    switch (message_types) {
        case 7:
            str_type = "General | Validation | Performance";
            break;
        case 6:
            str_type = "Validation | Performance";
            break;
        case 5:
            str_type = "General | Performance";
            break;
        case 4:
            str_type = "Performance";
            break;
        case 3:
            str_type = "General | Validation";
            break;
        case 2:
            str_type = "Validation";
            break;
        case 1:
            str_type = "General";
            break;
        default:
            str_type = "Unknown";
            break;
    }

    // Print the message to stderr if it is an error.
    if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        fprintf(stderr, "[%s: %s]\n%s\n", str_severity, str_type, callback_data->pMessage);
    } else {
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
uint32_t rate_physical_device(VkPhysicalDevice device) {
    uint32_t score = 0;


    // Get properties and features of that device
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    vkGetPhysicalDeviceFeatures(device, &device_features);
    const char *name = device_properties.deviceName;

    // Prefer discrete gpu when available
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 10000;
    }

    // The bigger, the better
    score += device_properties.limits.maxImageDimension2D;

    // The device needs to support the following device extensions, otherwise it is unusable
#define REQUIRED_DEVICE_EXT_COUNT 1
    const char *required_device_extensions[REQUIRED_DEVICE_EXT_COUNT] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    bool extensions_are_supported = check_device_extension_support(device, required_device_extensions,
                                                                   REQUIRED_DEVICE_EXT_COUNT);

    // Reset score if the extension are not supported because it is mandatory
    if (!extensions_are_supported) {
        score = 0;
    }


    // printf("GPU: %s | Score: %d\n", name, score);

    return score;
}
// endregion

// region Allocator

VmaAllocator create_allocator(VkInstance instance, VkDevice device, VkPhysicalDevice physical_device) {
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

void destroy_allocator(VmaAllocator *allocator) {
    vmaDestroyAllocator(*allocator);
    *allocator = VK_NULL_HANDLE;
}

rg_allocated_buffer
create_buffer(VmaAllocator allocator, size_t allocation_size, VkBufferUsageFlags buffer_usage,
              VmaMemoryUsage memory_usage) {
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
            vmaCreateBuffer(allocator, &buffer_create_info, &allocation_create_info, &buffer.buffer, &buffer.allocation,
                            VK_NULL_HANDLE),
            "Couldn't allocate buffer");

    return buffer;
}

void destroy_buffer(VmaAllocator allocator, rg_allocated_buffer *buffer) {
    vmaDestroyBuffer(allocator, buffer->buffer, buffer->allocation);
    buffer->buffer = VK_NULL_HANDLE;
    buffer->allocation = VK_NULL_HANDLE;
    buffer->size = 0;
}

rg_allocated_image create_image(VmaAllocator allocator,
                                VkDevice device,
                                VkFormat image_format,
                                VkExtent3D image_extent,
                                VkImageUsageFlags image_usage,
                                VkImageAspectFlagBits image_aspect,
                                VmaMemoryUsage memory_usage) {
    // We use VMA for now. We can always switch to a custom allocator later if we want to.
    rg_allocated_image image = {};

    rg_check(image_extent.width >= 1 && image_extent.height >= 1 && image_extent.depth >= 1,
             "Tried to create an image with an invalid extent. The extent must be at least 1 in each dimension.");

    // Create the image using VMA
    VkImageCreateInfo image_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            // Image info
            .imageType   = VK_IMAGE_TYPE_2D,
            .format      = image_format,
            .extent      = image_extent,
            .mipLevels   = 1,
            .arrayLayers = 1,
            .samples     = VK_SAMPLE_COUNT_1_BIT,
            .tiling      = VK_IMAGE_TILING_OPTIMAL,
            .usage       = image_usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    // Create an allocation info
    VmaAllocationCreateInfo allocation_create_info = {
            .usage          = memory_usage,
            .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    // Create the image
    vk_check(vmaCreateImage(allocator, &image_create_info, &allocation_create_info, &image.image, &image.allocation,
                            VK_NULL_HANDLE),
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

void destroy_image(VmaAllocator allocator, VkDevice device, rg_allocated_image *image) {
    // Destroy the image view first
    vkDestroyImageView(device, image->image_view, VK_NULL_HANDLE);
    image->image_view = VK_NULL_HANDLE;

    // Destroy the image next
    vmaDestroyImage(allocator, image->image, image->allocation);
    image->image = VK_NULL_HANDLE;
    image->allocation = VK_NULL_HANDLE;
}

// endregion

// --==== RENDERER ====--

// region Swapchain functions

void
rg_renderer_create_swapchain(rg_renderer *renderer, uint32_t swapchain_index, rg_surface surface, rg_extent_2d extent) {
    // Prevent access to the positions that are outside the swapchain array
    rg_check(swapchain_index < renderer->swapchains.size, "Swapchain index out of range.");

    // Get the swapchain in the renderer
    rg_swapchain *swapchain = renderer->swapchains.data + (swapchain_index * sizeof(rg_swapchain));

    // Ensure that there is not a live swapchain here already
    rg_check(!swapchain->enabled,
             "Attempted to create a swapchain in a slot where there was already an active one. To recreate a swapchain, see "
             "rg_renderer_recreate_swapchain.");

    // Check that the surface is supported
    VkBool32 surface_supported = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(renderer->physical_device, renderer->graphics_queue.family_index,
                                         (VkSurfaceKHR) surface, &surface_supported);
    rg_check(surface_supported, "The chosen GPU is unable to render to the given surface.");

    // region Present mode

    // Choose a present mode
    uint32_t available_present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->physical_device, (VkSurfaceKHR) surface,
                                              &available_present_modes_count, VK_NULL_HANDLE);
    rg_array available_present_modes = rg_create_array(available_present_modes_count, sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->physical_device, (VkSurfaceKHR) surface,
                                              &available_present_modes_count, available_present_modes.data);

    VkPresentModeKHR chosen_present_mode = 0;
    bool present_mode_found = false;
#define DESIRED_PRESENT_MODES_COUNT 2
    VkPresentModeKHR desired_present_modes[DESIRED_PRESENT_MODES_COUNT] = {
            VK_PRESENT_MODE_MAILBOX_KHR,
            VK_PRESENT_MODE_FIFO_KHR,
    };
    // Find a suited present mode
    for (uint32_t i = 0; i < available_present_modes_count && !present_mode_found; i++) {
        VkPresentModeKHR mode = ((VkPresentModeKHR *) available_present_modes.data)[i];
        for (uint32_t j = 0; j < DESIRED_PRESENT_MODES_COUNT && !present_mode_found; j++) {
            // Found a match ! Take it
            if (mode == desired_present_modes[j]) {
                chosen_present_mode = mode;
                present_mode_found = true;
            }
        }
    }
    rg_destroy_array(&available_present_modes);
    rg_check(present_mode_found, "Couldn't find a supported present mode for this surface.");

    // endregion

    // region Image count selection

    // Check the surface capabilities
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->physical_device, (VkSurfaceKHR) surface,
                                                       &surface_capabilities),
             NULL);

    // For the image count, take the minimum plus one. Or, if the minimum is equal to the maximum, take that value.
    uint32_t image_count = surface_capabilities.minImageCount + 1;
    if (image_count > surface_capabilities.maxImageCount) {
        image_count = surface_capabilities.maxImageCount;
    }
    swapchain->image_count = image_count;

    // endregion

    // region Format selection

    // Get the available formats
    uint32_t available_format_count = 0;
    vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device,
                                                  (VkSurfaceKHR) surface,
                                                  &available_format_count,
                                                  VK_NULL_HANDLE),
             NULL);
    rg_array available_formats = rg_create_array(available_format_count, sizeof(VkSurfaceFormatKHR));
    vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physical_device,
                                                  (VkSurfaceKHR) surface,
                                                  &available_format_count,
                                                  available_formats.data),
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
    for (uint32_t i = 0; i < available_format_count && !found; i++) {
        VkSurfaceFormatKHR *available_format = available_formats.data + (i * sizeof(VkSurfaceFormatKHR));

        for (uint32_t j = 0; j < DESIRED_FORMAT_COUNT && !found; j++) {
            if (available_format->format == desired_formats[j].format &&
                available_format->colorSpace == desired_formats[j].colorSpace) {
                swapchain->swapchain_image_format = desired_formats[j];
                found = true;
            }
        }
    }
    rg_check(found, "Couldn't find an appropriate format for the surface.");

    // Cleanup format selection
    rg_destroy_array(&available_formats);

    // endregion

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
            .presentMode = chosen_present_mode,
            // Image options
            .surface = (VkSurfaceKHR) surface,
            .preTransform = surface_capabilities.currentTransform,
            .imageExtent = swapchain->viewport_extent,
            .imageUsage      = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .minImageCount   = image_count,
            .imageFormat     = swapchain->swapchain_image_format.format,
            .imageColorSpace = swapchain->swapchain_image_format.colorSpace,
            .clipped         = VK_TRUE,
            .compositeAlpha  = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
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
    vkGetSwapchainImagesKHR(renderer->device, swapchain->vk_swapchain, &effective_image_count,
                            VK_NULL_HANDLE);
    swapchain->swapchain_images = malloc(sizeof(VkImage) * effective_image_count);
    vkGetSwapchainImagesKHR(renderer->device, swapchain->vk_swapchain, &effective_image_count,
                            swapchain->swapchain_images);

    // Update the image count based on how many were effectively created
    swapchain->image_count = effective_image_count;

    // Create image views for those images
    VkImageViewCreateInfo image_view_create_info = {
            // Struct info
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            // Image info
            .format = swapchain->swapchain_image_format.format,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
            },
    };

    // Allocate memory for it
    swapchain->swapchain_image_views = malloc(sizeof(VkImageView) * swapchain->image_count);

    // Create the image views
    for (uint32_t i = 0; i < swapchain->image_count; i++) {
        image_view_create_info.image = swapchain->swapchain_images[i];
        vk_check(vkCreateImageView(renderer->device, &image_view_create_info, NULL,
                                   &swapchain->swapchain_image_views[i]),
                 "Couldn't create image views for the swapchain images.");
    }
    // endregion

    // region Depth image creation

    VkExtent3D depth_image_extent = {
            .width = swapchain->viewport_extent.width,
            .height = swapchain->viewport_extent.height,
            .depth = 1,
    };
    swapchain->depth_image_format = VK_FORMAT_D32_SFLOAT;

    swapchain->depth_image = create_image(
            renderer->allocator,
            renderer->device,
            swapchain->depth_image_format,
            depth_image_extent,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
    );

    // endregion

    // Enable it
    swapchain->enabled = true;
}

void rg_renderer_recreate_swapchain(rg_renderer *renderer, uint32_t swapchain_index) {
    // Prevent access to the positions that are outside the swapchain array
    rg_check(swapchain_index < renderer->swapchains.size, "Swapchain index out of range.");
}

void rg_destroy_swapchain(const rg_renderer *renderer, rg_swapchain *swapchain) {
    // Destroy depth image
    destroy_image(renderer->allocator, renderer->device, &swapchain->depth_image);

    // Destroy swapchain images
    for (uint32_t i = 0; i < NB_OVERLAPPING_FRAMES; i++) {
        // Delete the image views
        vkDestroyImageView(renderer->device, swapchain->swapchain_image_views[i], NULL);
        swapchain->swapchain_image_views[i] = VK_NULL_HANDLE;
        // The deletion of the images is handled by the swapchain
    }

    // Clean the arrays
    free(swapchain->swapchain_image_views);
    free(swapchain->swapchain_images);


    // Destroy swapchain itself
    vkDestroySwapchainKHR(renderer->device, swapchain->vk_swapchain, NULL);
    swapchain->vk_swapchain = VK_NULL_HANDLE;

    // Disable it
    swapchain->enabled = false;
}

// endregion

// region Renderer functions

rg_renderer *
rg_create_renderer(rg_window *window, const char *application_name, rg_version application_version,
                   uint32_t swapchain_capacity) {
    // Create a renderer in the heap
    // This is done to keep the renderer opaque in the header
    // Use calloc to set all bytes to 0
    rg_renderer *renderer = calloc(1, sizeof(rg_renderer));

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
    rg_array required_extensions = rg_window_get_required_vulkan_extensions(window, extra_extension_count);

    // Add other extensions in the extra slots
    uint32_t extra_ext_index = required_extensions.size - extra_extension_count;
#ifdef USE_VK_VALIDATION_LAYERS
    ((char **) required_extensions.data)[extra_ext_index++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

    rg_check(check_instance_extension_support(required_extensions.data, required_extensions.size),
             "Not all required Vulkan extensions are supported.");

    // Get the validations layers if needed
#if USE_VK_VALIDATION_LAYERS
#define ENABLED_LAYERS_COUNT 1
    const char *required_validation_layers[ENABLED_LAYERS_COUNT] = {
            "VK_LAYER_KHRONOS_validation",
    };
    rg_check(check_layer_support(required_validation_layers, ENABLED_LAYERS_COUNT),
             "Vulkan validation layers requested, but not available.");
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
            .applicationVersion = VK_MAKE_VERSION(application_version.major, application_version.minor,
                                                  application_version.patch),
            .pApplicationName   = application_name,
    };

    VkInstanceCreateInfo instanceCreateInfo = {
            // Struct infos
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,

            // App info
            .pApplicationInfo = &applicationInfo,

            // Extensions
            .enabledExtensionCount   = required_extensions.size,
            .ppEnabledExtensionNames = required_extensions.data,

            // Validation layers
#if USE_VK_VALIDATION_LAYERS
            .enabledLayerCount   = ENABLED_LAYERS_COUNT,
            .ppEnabledLayerNames = required_validation_layers,
#else
            .enabledLayerCount   = 0,
            .ppEnabledLayerNames = NULL,
#endif

    };
    vk_check(vkCreateInstance(&instanceCreateInfo, NULL, &renderer->instance), "Couldn't create instance.");

    // Cleanup instance creation
    rg_destroy_array(&required_extensions);

    // Register instance in Volk
    volkLoadInstance(renderer->instance);

    // Create debug messenger
#ifdef USE_VK_VALIDATION_LAYERS
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {
            // Struct info
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = VK_NULL_HANDLE,
            // Message settings
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
            .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                               | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            // Callback
            .pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT) debug_messenger_callback,
    };
    vk_check(vkCreateDebugUtilsMessengerEXT(renderer->instance, &debug_messenger_create_info, NULL,
                                            &renderer->debug_messenger),
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
    for (uint32_t i = 0; i < available_physical_devices_count; i++) {
        VkPhysicalDevice checked_device = ((VkPhysicalDevice *) available_physical_devices.data)[i];
        uint32_t score = rate_physical_device(checked_device);

        if (score > current_max_score) {
            // New best device found, save it.
            // We don't need to keep the previous one, since we definitely won't choose it.
            current_max_score = score;
            renderer->physical_device = checked_device;
        }
    }

    // There is a problem if the device is still null: it means none was found.
    rg_check(renderer->physical_device != VK_NULL_HANDLE, "No suitable GPU was found.");

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
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queue_family_properties_count,
                                             queue_family_properties.data);

    // Find the queue families that we need
    bool found_graphics_queue = false;

    for (uint32_t i = 0; i < queue_family_properties_count; i++) {
        VkQueueFamilyProperties family_properties = ((VkQueueFamilyProperties *) queue_family_properties.data)[i];

        // Save the graphics queue family_index
        if (family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            renderer->graphics_queue.family_index = i;
            found_graphics_queue = true;
        }
    }

    rg_check(found_graphics_queue, "Unable to find a graphics queue family_index.");

    // Cleanup queue families selection
    rg_destroy_array(&queue_family_properties);

    // endregion

    // --=== Logical device and queues creation ===--

    // region Device and queues creation

    // Define the parameters for the graphics queue
    float graphics_queue_priority = 1.0f;
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

    renderer->allocator = create_allocator(renderer->instance, renderer->device, renderer->physical_device);

    // --=== Swapchains ===--

    // We need to create an array big enough to hold all the swapchains.
    // Use the zeroed variant because we do not initialize it, and we want it clean because to destroy the renderer, we check the
    // enabled field, and it must be zero by default.
    renderer->swapchains = rg_create_array_zeroed(swapchain_capacity, sizeof(rg_swapchain));

    return renderer;
}

void rg_destroy_renderer(rg_renderer **renderer) {
    // Destroy swapchains
    for (uint32_t i = 0; i < (*renderer)->swapchains.size; i++) {
        rg_swapchain *swapchain = (*renderer)->swapchains.data + (i * sizeof(rg_swapchain));
        if (swapchain->enabled) {
            rg_destroy_swapchain(*renderer, swapchain);
        }
    }

    // Destroy surface
    // TODO later use a more complex structure to be able to keep track of more than one surface
    if ((*renderer)->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR((*renderer)->instance, (VkSurfaceKHR) (*renderer)->surface, NULL);
    }

    // Destroy allocator
    destroy_allocator(&(*renderer)->allocator);

    // Destroy device
    vkDestroyDevice((*renderer)->device, NULL);

#ifdef USE_VK_VALIDATION_LAYERS
    // Destroy debug messenger
    vkDestroyDebugUtilsMessengerEXT((*renderer)->instance, (*renderer)->debug_messenger, NULL);
#endif
    // Destroy instance
    vkDestroyInstance((*renderer)->instance, NULL);

    // Free the renderer
    free(*renderer);

    // Since we took a double pointer in parameter, we can also set it to null
    // Thus, the user doesn't have to do it themselves
    *renderer = NULL;
}

rg_surface rg_get_window_surface(rg_renderer *renderer, rg_window *window) {
    // A VulkanSurfaceKHR is actually a pointer
    // But the caller doesn't know that
    // He just handles a pointer to something, labeled as rg_surface
    renderer->surface = rg_window_get_vulkan_surface(window, renderer->instance);
    return renderer->surface;
}

// endregion

#endif