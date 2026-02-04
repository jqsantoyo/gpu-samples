
#define UNICODE
#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>
#pragma once


#define GUARD(x)  if (!(x))              {  printf("Error: "#x); return 0; }
#define GUARDV(x) if ((x != VK_SUCCESS)) {  printf("Error: "#x); return 0; }

namespace gpu {

    bool enumerateInstanceLayerProperties(std::vector<VkLayerProperties>& v);
    bool enumerateInstanceExtensionProperties(std::vector<VkExtensionProperties>& v);
    bool enumeratePhysicalDevices(VkInstance instance, std::vector<VkPhysicalDevice>& v);
    bool enumerateDeviceExtensionProperties(VkPhysicalDevice device, const char* layerName, std::vector<VkExtensionProperties>& v);
    void getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device, std::vector<VkQueueFamilyProperties>& v);
    bool getPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR>& v);
    bool getPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<VkPresentModeKHR>& v);
    bool getSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& v);

    bool validateLayers(
        const std::vector<const char*>& requiredLayers,
        const std::vector<const char*>& requiredExtensionsWin,
        const std::vector<const char*>& requiredExtensionsMac,
        std::vector<const char*>& layers,
        std::vector<const char*>& extensions
    );

    bool createInstance(
        const char* name,
        uint32_t version,
        const std::vector<const char*>& layers,
        const std::vector<const char*>& extensions,
        VkInstance& instance
    );

    bool createSurface(VkInstance instance, void* window, VkSurfaceKHR& surface);
    
    struct PhysicalDeviceData {
        VkPhysicalDevice physicalDevice;
        uint32_t gIdx;
        uint32_t pIdx;
        VkSurfaceFormatKHR format;
        VkPresentModeKHR presentMode;
        VkExtent2D extent;
        uint32_t imageCount;
        VkSurfaceTransformFlagBitsKHR currTransform;
    };

    bool selectPhysicalDevice(
        VkInstance instance,
        VkSurfaceKHR surface,
        const std::vector<const char*>& extensions,
        PhysicalDeviceData& data
    );

    struct ComputePhysicalDeviceWrap {
        VkPhysicalDevice physicalDevice;
        uint32_t cIdx;
    };
    bool selectComputePhysicalDevice(VkInstance instance, const std::vector<const char*>& extensions, ComputePhysicalDeviceWrap& w);

    bool createDevice(VkPhysicalDevice pd, uint32_t gIdx, uint32_t pIdx, VkDevice& device, VkQueue& gQ, VkQueue& pQ);
    bool createComputeDevice(VkPhysicalDevice pd, uint32_t cIdx, VkDevice& device, VkQueue& cQ);


    struct SwapchainCtx {
        VkSwapchainKHR              swapchain;
        std::vector<VkImage>        images;
        VkFormat                    imageFormat;
        VkExtent2D                  extent;
        std::vector<VkImageView>    imageViews;
        std::vector<VkFramebuffer>  framebuffers;
    };

    bool createSwapchain(VkDevice device, VkSurfaceKHR surface, const PhysicalDeviceData& data, SwapchainCtx& swapchainCtx);


    bool createCommandObjects();

    
    struct Shader {
        const char* name;
        VkShaderModule module;
        std::vector<uint8_t> data;
    };

    bool loadShader(VkDevice device, const char* dir, const char* name, Shader& shader);
}
