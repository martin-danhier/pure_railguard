#include <railguard/rendering/device/RenderDevice.h>
#include <railguard/rendering/device/RenderDeviceVulkan.h>
#include <volk.h>
#include <VkBootstrap.h>
#include <iostream>

// --=== UTILS FUNCTIONS ===--

#define VK_CHECK(x)                                                     \
    do                                                                  \
    {                                                                   \
        VkResult err = x;                                               \
        if (err)                                                        \
        {                                                               \
            std::cerr << "Detected Vulkan error: " << err << std::endl; \
            abort();                                                    \
        }                                                               \
    } while (0)


// --=== HEADER IMPLEMENTATION ===--

namespace railguard::rendering
{
    railguard::rendering::RenderDeviceVulkan::RenderDeviceVulkan()
    {
        // First, we need to initialise Volk
        VK_CHECK(volkInitialize());

        // Create Vulkan instance
        vkb::InstanceBuilder vkbInstanceBuilder;
#ifdef USE_VK_VALIDATION_LAYERS
        // Enable validation layers and debug messenger in debug mode
        vkbInstanceBuilder.request_validation_layers(true).enable_validation_layers().use_default_debug_messenger();
#endif

        // Get required SDL extensions
        auto requiredSDLExtensions = initInfo.windowManager.GetRequiredVulkanExtensions();

        vkbInstanceBuilder.set_app_name("My wonderful game")
        .set_app_version(0, 1, 0)
        .set_engine_version(0, 1, 0)
        .require_api_version(1, 1, 0)
        .set_engine_name("Railguard");

        // Add sdl extensions
        for (const char *ext : requiredSDLExtensions)
        {
            vkbInstanceBuilder.enable_extension(ext);
        }

        // Build instance
        auto vkbInstance = vkbInstanceBuilder.build().value();
        _instance = vkbInstance.instance;
#ifdef USE_VK_VALIDATION_LAYERS
        _debugMessenger = vkbInstance.debug_messenger;
#endif

        // Load the instance in Volk
        volkLoadInstance(_instance);



    }
} // namespace railguard::rendering
