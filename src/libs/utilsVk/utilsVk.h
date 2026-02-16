
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

    struct InstanceCtx {
        VkInstance                          instance;
        VkDebugUtilsMessengerEXT            debugMessenger;
        PFN_vkCreateDebugUtilsMessengerEXT  createDebugMessengerFn;
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessengerFn;
    };

    struct PhysicalDeviceCtx {
        VkPhysicalDevice physicalDevice;
        uint32_t gIdx;
        uint32_t pIdx;
    };
    struct ComputePhysicalDeviceWrap {
        VkPhysicalDevice physicalDevice;
        uint32_t cIdx;
    };

    struct SurfaceInfo {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkPresentModeKHR>   presentModes;
        std::vector<VkSurfaceFormatKHR> formats;
    };

    struct SwapchainCtx {
        VkSwapchainKHR              swapchain;
        VkFormat                    format;
        VkPresentModeKHR            presentMode;
        VkExtent2D                  extent;
        std::vector<VkImage>        images;
        std::vector<VkImageView>    imageViews;
        std::vector<VkFramebuffer>  framebuffers;
    };

    struct Shader {
        const char* name;
        VkShaderModule module;
        std::vector<uint8_t> data;
        VkPipelineShaderStageCreateInfo info;
    };

    struct FrameControl {
        std::vector<VkCommandPool>    cmdPool;
        std::vector<VkCommandBuffer>  cmdBuffer;
        std::vector<VkSemaphore>      imageReady;
        std::vector<VkSemaphore>      renderReady;
        std::vector<VkFence>          execution;
    };


    bool enumerateInstanceLayerProperties(std::vector<VkLayerProperties>& v);
    bool enumerateInstanceExtensionProperties(std::vector<VkExtensionProperties>& v);
    bool enumeratePhysicalDevices(VkInstance instance, std::vector<VkPhysicalDevice>& v);
    bool enumerateDeviceExtensionProperties(VkPhysicalDevice device, const char* layerName, std::vector<VkExtensionProperties>& v);
    void getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device, std::vector<VkQueueFamilyProperties>& v);
    bool getPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR>& v);
    bool getPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<VkPresentModeKHR>& v);
    bool getSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& v);
    VkResult createSemaphore(VkDevice device, VkSemaphore& semaphore);
    VkResult createFence(VkDevice device, VkFence& fence, bool signaled);
    

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
        InstanceCtx& ctx
    );
    void destroyInstance(InstanceCtx& ctx);

    bool createSurface(VkInstance instance, void* window, VkSurfaceKHR& surface);
    bool getSurfaceInfo(VkPhysicalDevice pd, VkSurfaceKHR surface, SurfaceInfo& surfaceInfo);

    bool selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*>& extensions, PhysicalDeviceCtx& pd);
    bool selectComputePhysicalDevice(VkInstance instance, const std::vector<const char*>& extensions, ComputePhysicalDeviceWrap& w);

    bool createDevice(PhysicalDeviceCtx pdCtx, VkDevice& device, VkQueue& gQ, VkQueue& pQ);
    bool createComputeDevice(VkPhysicalDevice pd, uint32_t cIdx, VkDevice& device, VkQueue& cQ);

    bool createSwapchain(VkDevice device, VkSurfaceKHR surface, const PhysicalDeviceCtx& pdCtx, SwapchainCtx& ctx);
    bool createSwapchainFramebuffers(VkDevice device, SwapchainCtx& ctx, VkRenderPass renderPass);
    bool destroySwapchain(VkDevice device, SwapchainCtx& ctx);

    bool createFrameControl(FrameControl& frameControl);
    bool deatroyFrameControl(FrameControl& frameControl);

    bool loadShader(VkDevice device, const char* dir, const char* name, Shader& shader, VkShaderStageFlagBits stage);
}
