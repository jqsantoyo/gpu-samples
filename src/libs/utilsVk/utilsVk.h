#include <vulkan/vulkan.h>
#include <vector>
#pragma once


#define GUARD(x)  if (!(x))              {  printf("Error: "#x"\n"); return 0; }
#define GUARDV(x) if ((x != VK_SUCCESS)) {  printf("Error: "#x"\n"); return 0; }

namespace gpu::vk {




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
        const std::vector<const char*>& ext
    );
    void terminate();

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
    void terminate();
    bool info(VkPhysicalDevice pd, SurfaceInfo& surfaceInfo);
private:
    VkInstance instance;
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// PHYSICAL DEVICE

struct PhysicalDeviceData {
    VkPhysicalDeviceProperties           props;
    VkPhysicalDeviceFeatures             features;
    VkPhysicalDeviceMemoryProperties     memProps;
    std::vector<VkQueueFamilyProperties> queueFamilies;
    std::vector<VkExtensionProperties>   devExtensions;
    void print();
};

bool getPhysicalDevices(VkInstance instance, std::vector<PhysicalDeviceData>& phyDevices);

class PhysicalDevice {
public:
    VkPhysicalDevice physicalDevice;
    uint32_t gIdx;
    uint32_t pIdx;
    uint32_t cIdx;
    uint32_t uploadMem;
    uint32_t deviceMem;

    bool init(VkInstance instance, const std::vector<const char*>& extensions, bool graphics, bool compute, Surface* surface);

};






////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVICE
class Device {
public:
    VkDevice device;
    uint32_t gIdx;
    uint32_t pIdx;
    uint32_t cIdx;
    VkQueue gQ;
    VkQueue pQ;
    VkQueue cQ;
    uint32_t uploadMem;
    uint32_t deviceMem;
    bool init(PhysicalDevice& physicalDevice, bool graphical, bool compute);
    void terminate();
};





////////////////////////////////////////////////////////////////////////////////////////////////////
// SWAPCHAIN

class Swapchain {
public:
    bool init(VkDevice device, Surface* surface, VkPhysicalDevice physicalDevice, uint32_t gIdx, uint32_t pIdx, VkQueue pQueue);
    void terminate();
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
    bool load(VkDevice device, const char* name, VkShaderStageFlagBits stage);
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
    void terminate();
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
// MESH REGISTRY

struct Mesh {
    VkBuffer        buffer;
    VkDeviceMemory  memory;
    VkBuffer        idxBuffer;
    VkDeviceMemory  idxMemory;
};

class MeshRegistry {
public:
    bool init(Device* device);
    void terminate();
    bool addMesh(float* data, uint32_t size, uint16_t* idxData, uint32_t idxSize, int& meshIdx);
    Mesh& getMesh(int i);
private:
    Device*           device;
    VkBuffer          uploadBuffer;
    VkDeviceMemory    uploadMemory;
    VkCommandPool     cmdPool;
    VkCommandBuffer   cmdBuffer;
    std::vector<Mesh> meshes;
};




}


