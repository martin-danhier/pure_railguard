#pragma once

#include <cstdint>
#include <railguard/rendering/device/RenderDevice.h>
using std::uint32_t;

// Forward declarations
#define VK_DEFINE_HANDLE(object) typedef struct object##_T *(object);
VK_DEFINE_HANDLE(VkDebugUtilsMessengerEXT)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkPhysicalDeviceProperties)
VK_DEFINE_HANDLE(VkQueue)
VK_DEFINE_HANDLE(VkRenderPass)

namespace railguard::rendering
{
    class RenderDeviceVulkan : public RenderDevice
    {
      private:
        VkDevice _device                                     = nullptr;
        VkInstance _instance                                 = nullptr;
        VkPhysicalDevice _physicalDevice                     = nullptr;
        VkPhysicalDeviceProperties _physicalDeviceProperties = nullptr;
        VkQueue _graphicsQueue                               = nullptr;
        VkRenderPass _mainRenderPass                         = nullptr;
        uint32_t _graphicsQueueFamily                        = 0;
#ifdef USE_VK_VALIDATION_LAYERS
        VkDebugUtilsMessengerEXT _debugMessenger = nullptr;
#endif
      public:
        RenderDeviceVulkan();
    };

} // namespace railguard::rendering