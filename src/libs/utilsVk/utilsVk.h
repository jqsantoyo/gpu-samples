
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




////////////////////////////////////////////////////////////////////////////////////////////////////
// UTLITIES

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




////////////////////////////////////////////////////////////////////////////////////////////////////
// INSTANCE

class Instance {
public:
    VkInstance                instance;
    std::vector<const char*>  layers;
    std::vector<const char*>  extensions;

    bool init(
        const char* name,
        uint32_t version,
        bool graphical,
        const std::vector<const char*>& extWin,
        const std::vector<const char*>& extMac
    );
    void deinit();

private:
    VkDebugUtilsMessengerEXT            debugMessenger;
    PFN_vkCreateDebugUtilsMessengerEXT  createDebugMessengerFn;
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessengerFn;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// SURFACE

struct SurfaceInfo {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkPresentModeKHR>   presentModes;
    std::vector<VkSurfaceFormatKHR> formats;
};

class Surface {
public:
    VkSurfaceKHR surface;

    bool init(VkInstance instance, void* window);
    void deinit();
    bool info(VkPhysicalDevice pd, SurfaceInfo& surfaceInfo);
private:
    VkInstance instance;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// PHYSICAL DEVICE

class PhysicalDevice {
public:
    VkPhysicalDevice physicalDevice;
    uint32_t gIdx;
    uint32_t pIdx;
    uint32_t cIdx;
    uint32_t uploadMem;

    bool init(VkInstance instance, const std::vector<const char*>& extensions, bool graphics, bool compute, Surface* surface);

};






////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVICE
class Device {
public:
    VkDevice device;
    VkQueue gQ;
    VkQueue pQ;
    VkQueue cQ;
    uint32_t uploadMem;
    bool init(PhysicalDevice& physicalDevice, bool graphical, bool compute);
    void deinit();
};





////////////////////////////////////////////////////////////////////////////////////////////////////
// SWAPCHAIN

class Swapchain {
public:
    bool init(VkDevice device, Surface* surface, VkPhysicalDevice physicalDevice, uint32_t gIdx, uint32_t pIdx, VkQueue pQueue);
    void deinit();
    bool recreate();
    void resize();
    bool next(VkSemaphore signal);
    bool recreated();
    bool present();

    VkSwapchainKHR              swapchain;
    VkFormat                    format;
    VkPresentModeKHR            presentMode;
    VkExtent2D                  extent;
    std::vector<VkImage>        images;
    std::vector<VkImageView>    imageViews;
    uint32_t                    idx;
    VkSemaphore                 renderReady;
private:
    VkDevice                    device;
    Surface*                    surface;
    VkPhysicalDevice            physicalDevice;
    uint32_t                    gIdx;
    uint32_t                    pIdx;
    VkQueue                     pQueue;
    bool                        resizedFlag = false;
    bool                        recreatedFlag = false;
    std::vector<VkSemaphore>    renderReadyVec;
};






////////////////////////////////////////////////////////////////////////////////////////////////////
// SHADER

class Shader {
public:
    ~Shader();
    bool load(VkDevice device, const char* dir, const char* name, VkShaderStageFlagBits stage);
    VkPipelineShaderStageCreateInfo getInfo();
private:
    VkDevice device;
    const char* name;
    VkShaderModule module;
    std::vector<uint8_t> data;
    VkPipelineShaderStageCreateInfo stageInfo;
};








////////////////////////////////////////////////////////////////////////////////////////////////////
// FRAME CONTROL

struct Frame {
    VkCommandBuffer cmdBuffer;
    VkSemaphore imageReady;
};

class FrameControl {
public:
    bool init(VkDevice device, uint32_t queueIdx, VkQueue queue, int frameCount);
    void deinit();
    Frame next();
    bool begin();
    bool end(VkSemaphore renderReady);
private:
    VkDevice                      device;
    VkQueue                       queue;
    int                           frameCount;
    int                           frameIdx;
    std::vector<VkCommandPool>    cmdPool;
    std::vector<VkCommandBuffer>  cmdBuffer;
    std::vector<VkSemaphore>      imageReady;
    std::vector<VkFence>          execution;
};






////////////////////////////////////////////////////////////////////////////////////////////////////
// Buffer

class Buffer {
public:
    bool init(Device* device, float* data, uint32_t size);
    void deinit();
    Device* device;
    VkBuffer buffer;
    VkDeviceMemory memory;

};




}


