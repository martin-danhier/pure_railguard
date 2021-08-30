// Place the contents of the file inside a guard, so that only one implementation is generated
#ifdef RENDERER_VULKAN

#include "railguard/rendering/renderer.h"
#include <railguard/rendering/window.h>
#include <railguard/utils/arrays.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <volk.h>
#if USE_VK_VALIDATION_LAYERS
#include <string.h>
#endif

// --==== STRUCTS ====--

typedef struct rg_queue_families
{
    uint32_t graphics_queue_family;
} rg_queue_families;

typedef struct rg_renderer
{
    VkDevice          device;
    VkInstance        instance;
    VkPhysicalDevice  physical_device;
    VkQueue           graphics_queue;
    rg_queue_families queue_families;
#ifdef USE_VK_VALIDATION_LAYERS
    VkDebugUtilsMessengerEXT debug_messenger;
#endif
} rg_renderer;

// --==== UTILS FUNCTIONS ====--

// region Error handling functions

void vk_check(VkResult result, const char *error_message)
{
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "[Vulkan Error] Got VkResult = %d !\n", result);
        if (error_message != NULL)
        {
            fprintf(stderr, "%s\n", error_message);
        }
        exit(1);
    }
}

void rg_check(bool result, const char *error_message)
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
 * @param desired_extensions is an array of desired_extensions_count layer names (null terminated strings)
 * @param desired_extensions_count is the number of extension names to check
 * @return true if every extension is supported, false otherwise.
 */
bool check_instance_extension_support(const char *const *desired_extensions, uint32_t desired_extensions_count)
{
    // Get the number of available desired_extensions
    uint32_t available_extensions_count = 0;
    vk_check(vkEnumerateInstanceExtensionProperties(NULL, &available_extensions_count, VK_NULL_HANDLE), NULL);
    // Create an array with enough room and fetch the available desired_extensions
    rg_array available_extensions = rg_create_array(available_extensions_count, sizeof(VkExtensionProperties));
    vk_check(vkEnumerateInstanceExtensionProperties(NULL, &available_extensions_count, available_extensions.data), NULL);

    // For each desired extension, rg_check if it is available
    bool valid = true;
    for (uint32_t i = 0; i < desired_extensions_count && valid; i++)
    {
        bool found = false;

        // Search available until the desired is found or not
        for (uint32_t j = 0; j < available_extensions_count && !found; j++)
        {
            if (strcmp(desired_extensions[i], ((VkExtensionProperties *) available_extensions.data)[j].extensionName) == 0)
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
bool check_device_extension_support(VkPhysicalDevice   physical_device,
                                    const char *const *desired_extensions,
                                    uint32_t           desired_extensions_count)
{
    // Get the number of available desired_extensions
    uint32_t available_extensions_count = 0;
    vk_check(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &available_extensions_count, VK_NULL_HANDLE), NULL);
    // Create an array with enough room and fetch the available desired_extensions
    rg_array available_extensions = rg_create_array(available_extensions_count, sizeof(VkExtensionProperties));
    vk_check(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &available_extensions_count, available_extensions.data),
             NULL);

    // For each desired extension, rg_check if it is available
    bool valid = true;
    for (uint32_t i = 0; i < desired_extensions_count && valid; i++)
    {
        bool found = false;

        // Search available until the desired is found or not
        for (uint32_t j = 0; j < available_extensions_count && !found; j++)
        {
            if (strcmp(desired_extensions[i], ((VkExtensionProperties *) available_extensions.data)[j].extensionName) == 0)
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
bool check_layer_support(const char *const *desired_layers, uint32_t desired_layers_count)
{
    // Get the number of available desired_layers
    uint32_t available_layers_count = 0;
    vk_check(vkEnumerateInstanceLayerProperties(&available_layers_count, VK_NULL_HANDLE), NULL);
    // Create an array with enough room and fetch the available desired_layers
    rg_array available_layers = rg_create_array(available_layers_count, sizeof(VkLayerProperties));
    vk_check(vkEnumerateInstanceLayerProperties(&available_layers_count, available_layers.data), NULL);

    // For each desired layer, rg_check if it is available
    bool valid = true;
    for (uint32_t i = 0; i < desired_layers_count && valid; i++)
    {
        bool found = false;

        // Search available until the desired is found or not
        for (uint32_t j = 0; j < available_layers_count && !found; j++)
        {
            if (strcmp(desired_layers[i], ((VkLayerProperties *) available_layers.data)[j].layerName) == 0)
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
VKAPI_ATTR VkBool32 VKAPI_CALL *debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                         VkDebugUtilsMessageTypeFlagsEXT             message_types,
                                                         const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                         void                                       *user_data)
{
    // Inspired by VkBootstrap's default debug messenger. (Made by Charles Giessen)

    // Get severity
    char *str_severity;
    switch (message_severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: str_severity = "VERBOSE"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: str_severity = "ERROR"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: str_severity = "WARNING"; break;
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
uint32_t rate_physical_device(VkPhysicalDevice device)
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
        score += 2000;
    }

    // The bigger, the better
    score += device_properties.limits.maxImageDimension2D;

    // The device needs to support the following device extensions, otherwise it is unusable
#define REQUIRED_DEVICE_EXT_COUNT 1
    const char *required_device_extensions[REQUIRED_DEVICE_EXT_COUNT] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    bool extensions_are_supported = check_device_extension_support(device, required_device_extensions, REQUIRED_DEVICE_EXT_COUNT);

    // Reset score if the extension are not supported because it is mandatory
    if (!extensions_are_supported)
    {
        score = 0;
    }

    return score;
}
// endregion

// --==== RENDERER ====--

rg_renderer *rg_create_renderer(rg_window *window, const char *application_name, rg_version application_version)
{
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
        .apiVersion    = VK_API_VERSION_1_1,
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
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        // Callback
        .pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT) debug_messenger_callback,
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
        uint32_t         score          = rate_physical_device(checked_device);

        if (score > current_max_score)
        {
            // New best device found, save it.
            // We don't need to keep the previous one, since we definitely won't choose it.
            current_max_score         = 0;
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
    vkGetPhysicalDeviceQueueFamilyProperties(renderer->physical_device, &queue_family_properties_count, queue_family_properties.data);

    // Find the queue families that we need
    for (uint32_t i = 0; i < queue_family_properties_count; i++)
    {
        VkQueueFamilyProperties family_properties = ((VkQueueFamilyProperties *) queue_family_properties.data)[i];

        // Save the graphics queue family
        if (family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            renderer->queue_families.graphics_queue_family = i;
        }
    }

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
        .queueFamilyIndex = renderer->queue_families.graphics_queue_family,
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

    // Get created queues
    vkGetDeviceQueue(renderer->device, renderer->queue_families.graphics_queue_family, 0, &renderer->graphics_queue);

    // endregion

    // --=== Swapchains ===--



    return renderer;
}

void rg_destroy_renderer(rg_renderer **renderer)
{
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

#endif