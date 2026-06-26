#include "gpu.h"
#include <vulkan/vulkan.h>
#include <utils/utils.h>
#include <cstdio>
#include <vector>
#include <inttypes.h>
#include <set>
#include <string>
#define UNICODE
#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#include <algorithm>


namespace gpu::vk {

////////////////////////////////////////////////////////////////////////////////////////////////////
/// UTILITIES //////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


#define GUARD(x)  if (!(x))              {  printf("Error: "#x"\n"); return false; }
#define GUARDV(x) if ((x != VK_SUCCESS)) {  printf("Error: "#x"\n"); return false; }
#define ASSERT(x) { assert(x); }
#define ASSERT_VK(x) { assert(x == VK_SUCCESS); }


VkPolygonMode getNativeFillMode(FillMode mode) {
    switch(mode) {
        case FillMode::Wireframe : return VK_POLYGON_MODE_LINE;
        case FillMode::Solid     : return VK_POLYGON_MODE_FILL;
    }
}

VkCullModeFlags getNativeCullMode(CullMode mode) {
    switch(mode) {
        case CullMode::None  : return VK_CULL_MODE_NONE;
        case CullMode::Front : return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back  : return VK_CULL_MODE_BACK_BIT;
    }
}

template <typename T>
const T& clamp(const T& value, const T& low, const T& high) {
    return value < low ? low : (value > high ? high : value);
}

bool enumerateInstanceLayerProperties(std::vector<VkLayerProperties>& v) {
    uint32_t n;
    GUARDV(vkEnumerateInstanceLayerProperties(&n, nullptr));
    v.resize(n);
    GUARDV(vkEnumerateInstanceLayerProperties(&n, v.data()));
    return true;
}

bool enumerateInstanceExtensionProperties(std::vector<VkExtensionProperties>& v) {
    uint32_t n;
    GUARDV(vkEnumerateInstanceExtensionProperties(nullptr, &n, nullptr));
    v.resize(n);
    GUARDV(vkEnumerateInstanceExtensionProperties(nullptr, &n, v.data()));
    return true;
}

bool enumeratePhysicalDevices(VkInstance instance, std::vector<VkPhysicalDevice>& v) {
    uint32_t n = 0;
    GUARDV(vkEnumeratePhysicalDevices(instance, &n, nullptr));
    v.resize(n);
    GUARDV(vkEnumeratePhysicalDevices(instance, &n, v.data()));
    return true;
}

bool enumerateDeviceExtensionProperties(VkPhysicalDevice device, const char* layerName, std::vector<VkExtensionProperties>& v) {
    uint32_t n = 0;
    GUARDV(vkEnumerateDeviceExtensionProperties(device, layerName, &n, nullptr));
    v.resize(n);
    GUARDV(vkEnumerateDeviceExtensionProperties(device, layerName, &n, v.data()));
    return true;
}

void getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice device, std::vector<VkQueueFamilyProperties>& v) {
    uint32_t n = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &n, nullptr);
    v.resize(n);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &n, v.data());
}

bool getPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR>& v) {
    uint32_t n = 0;
    GUARDV(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &n, nullptr));
    v.resize(n);
    GUARDV(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &n, v.data()));
    return true;
}

bool getPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<VkPresentModeKHR>& v) {
    uint32_t n = 0;
    GUARDV(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &n, nullptr));
    v.resize(n);
    GUARDV(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &n, v.data()));
    return true;
}

bool getSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& v) {
    uint32_t n = 0;
    GUARDV(vkGetSwapchainImagesKHR(device, swapchain, &n, nullptr));
    v.resize(n);
    GUARDV(vkGetSwapchainImagesKHR(device, swapchain, &n, v.data()));
    return true;
}

VkResult createSemaphore(VkDevice device, VkSemaphore& semaphore) {
    VkSemaphoreCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    return vkCreateSemaphore(device, &info, nullptr, &semaphore);
}

VkResult createFence(VkDevice device, VkFence& fence, bool signaled) {
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    return vkCreateFence(device, &info, nullptr, &fence);
}


VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    printf("Validation layers: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}











////////////////////////////////////////////////////////////////////////////////////////////////////
// SHADER

class Shader {
public:
    ~Shader() {
        vkDestroyShaderModule(device, module, nullptr);
    }
    bool load(VkDevice device, const char* name, VkShaderStageFlagBits stage) {
        this->device = device;
        std::string path = std::string(name) + ".spv";
        ASSERT(readFile(path.c_str(), data));

        VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = data.size(),
            .pCode = reinterpret_cast<uint32_t*>(data.data()),
        };
        GUARDV(vkCreateShaderModule(device, &createInfo, nullptr, &module));
        stageInfo = {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0,
            .stage               = stage,
            .module              = module,
            .pName               = "main",
            .pSpecializationInfo = nullptr,
        };
        return true;
    }
    VkPipelineShaderStageCreateInfo getInfo() {
        return stageInfo;
    }

private:
    VkDevice device;
    VkShaderModule module;
    std::vector<uint8_t> data;
    VkPipelineShaderStageCreateInfo stageInfo;
};
























////////////////////////////////////////////////////////////////////////////////////////////////////
// INSTANCE

// Obsolete
bool validateLayers(
    const std::vector<const char*>& requiredLayers,
    const std::vector<const char*>& requiredExtensionsWin,
    const std::vector<const char*>& requiredExtensionsMac,
    std::vector<const char*>& layers,
    std::vector<const char*>& extensions
) {
    const std::vector<const char*>& requiredExtensions =
    #ifdef WIN32
        requiredExtensionsWin;
    #else
        requiredExtensionsMac;
    #endif
    layers.reserve(requiredLayers.size());
    extensions.reserve(requiredExtensions.size());
    std::vector<VkLayerProperties> availableLayers;
    std::vector<VkExtensionProperties> availableInstanceExtensions;
    enumerateInstanceLayerProperties(availableLayers);
    enumerateInstanceExtensionProperties(availableInstanceExtensions);
    for (const char* x : requiredLayers) {
        bool found = false;
        for (const auto& layer : availableLayers) {
            if (strcmp(x, layer.layerName) == 0) {
                printf("  %s : found\n", x);
                found = true;
                layers.push_back(x);
                break;
            }
        }
        if (!found) {
                printf("  %s : not found\n", x);
            return false;
        }
    }
    for (const char* x : requiredExtensions) {
        bool found = false;
        for (const auto& extension : availableInstanceExtensions) {
            if (strcmp(x, extension.extensionName) == 0) {
                printf("  %s : found\n", x);
                found = true;
                extensions.push_back(x);
                break;
            }
        }
        if (!found) {
                printf("  %s : not found\n", x);
            return false;
        }
    }
    return true;
}











    






////////////////////////////////////////////////////////////////////////////////////////////////////
// PHYSICAL DEVICE

struct PhysicalDevice {
    VkPhysicalDevice                        obj;
    VkPhysicalDeviceProperties              props;
    VkPhysicalDeviceFeatures                features;
    VkPhysicalDeviceMemoryProperties        memProps;
    std::vector<VkQueueFamilyProperties>    queueFamilies;
    std::vector<VkExtensionProperties>      devExtensions;
    VkSurfaceCapabilitiesKHR                surfaceCapabilities;
    std::vector<VkPresentModeKHR>           surfacePresentModes;
    std::vector<VkSurfaceFormatKHR>         surfaceFormats;
    uint32_t                                graphicsFamilyIdx   = -1;
    uint32_t                                presentFamilyIdx    = -1;
    uint32_t                                computeFamilyIdx    = -1;
    uint32_t                                uploadMemoryIdx     = -1;
    uint32_t                                deviceMemoryIdx     = -1;

    void init(VkPhysicalDevice pd, VkSurfaceKHR surface) {
        this->obj = pd;
        vkGetPhysicalDeviceFeatures                         (obj, &features);
        vkGetPhysicalDeviceProperties                       (obj, &props);
        vkGetPhysicalDeviceMemoryProperties                 (obj, &memProps);
        getPhysicalDeviceQueueFamilyProperties              (obj, queueFamilies);
        enumerateDeviceExtensionProperties                  (obj, nullptr, devExtensions);
        ASSERT_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR (obj, surface, &surfaceCapabilities));
        ASSERT(getPhysicalDeviceSurfaceFormatsKHR           (obj, surface, surfaceFormats));
        ASSERT(getPhysicalDeviceSurfacePresentModesKHR      (obj, surface, surfacePresentModes));

        for (int i = 0; i < queueFamilies.size(); i++) {
            VkQueueFlags flags = queueFamilies[i].queueFlags;
            if (graphicsFamilyIdx == -1 && flags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamilyIdx = i;
            }
            if (computeFamilyIdx == -1 && flags & VK_QUEUE_COMPUTE_BIT) {
                computeFamilyIdx = i;
            }
            if (presentFamilyIdx == -1) {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(obj, i, surface, &presentSupport);
                if (presentSupport) {
                    presentFamilyIdx = i;
                }
            }
        }
    
        for(int i = 0; i < memProps.memoryTypeCount; i++) {
            VkMemoryType& memType = memProps.memoryTypes[i];
            if (uploadMemoryIdx == -1 &&
                memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
                memType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ) {
                uploadMemoryIdx = i;
            }
            if (deviceMemoryIdx == -1 &&
                memType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            ) {
                deviceMemoryIdx = i;
            }
        }
    }

    void print() {
        const char* deviceTypeStr = "";
        switch(props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: deviceTypeStr = "integrated device"; break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU  : deviceTypeStr = "discrete device";   break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU   : deviceTypeStr = "virtual device";    break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU           : deviceTypeStr = "cpu";            break;
            default                                    : deviceTypeStr = "unknown";        break;
        }

        printf("  Properties:\n");
        printf("     api version         : %d\n", props.apiVersion);
        printf("     driver version      : %d\n", props.driverVersion);
        printf("     vendor id           : %d\n", props.vendorID);
        printf("     device id           : %d\n", props.deviceID);
        printf("     device type         : %s\n", deviceTypeStr);
        printf("     device name         : %s\n", props.deviceName);
        printf("     pipeline cache uuid : ");
        for (int i = 0; i < VK_UUID_SIZE; i++) {
            printf(":%d", props.pipelineCacheUUID[i]);
        }
        printf("\n");
        printf("     limits              : size(%zu)\n", sizeof(props.limits));
        printf("     sparse properties   : size(%zu)\n", sizeof(props.sparseProperties));

        printf("  \nFeatures:\n");
        printf("     robustBufferAccess:                        %s\n", features.robustBufferAccess                      ? "yes" : "no");
        printf("     fullDrawIndexUint32:                       %s\n", features.fullDrawIndexUint32                     ? "yes" : "no");
        printf("     imageCubeArray:                            %s\n", features.imageCubeArray                          ? "yes" : "no");
        printf("     independentBlend:                          %s\n", features.independentBlend                        ? "yes" : "no");
        printf("     geometryShader:                            %s\n", features.geometryShader                          ? "yes" : "no");
        printf("     tessellationShader:                        %s\n", features.tessellationShader                      ? "yes" : "no");
        printf("     sampleRateShading:                         %s\n", features.sampleRateShading                       ? "yes" : "no");
        printf("     dualSrcBlend:                              %s\n", features.dualSrcBlend                            ? "yes" : "no");
        printf("     logicOp:                                   %s\n", features.logicOp                                 ? "yes" : "no");
        printf("     multiDrawIndirect:                         %s\n", features.multiDrawIndirect                       ? "yes" : "no");
        printf("     drawIndirectFirstInstance:                 %s\n", features.drawIndirectFirstInstance               ? "yes" : "no");
        printf("     depthClamp:                                %s\n", features.depthClamp                              ? "yes" : "no");
        printf("     depthBiasClamp:                            %s\n", features.depthBiasClamp                          ? "yes" : "no");
        printf("     fillModeNonSolid:                          %s\n", features.fillModeNonSolid                        ? "yes" : "no");
        printf("     depthBounds:                               %s\n", features.depthBounds                             ? "yes" : "no");
        printf("     wideLines:                                 %s\n", features.wideLines                               ? "yes" : "no");
        printf("     largePoints:                               %s\n", features.largePoints                             ? "yes" : "no");
        printf("     alphaToOne:                                %s\n", features.alphaToOne                              ? "yes" : "no");
        printf("     multiViewport:                             %s\n", features.multiViewport                           ? "yes" : "no");
        printf("     samplerAnisotropy:                         %s\n", features.samplerAnisotropy                       ? "yes" : "no");
        printf("     textureCompressionETC2:                    %s\n", features.textureCompressionETC2                  ? "yes" : "no");
        printf("     textureCompressionASTC_LDR:                %s\n", features.textureCompressionASTC_LDR              ? "yes" : "no");
        printf("     textureCompressionBC:                      %s\n", features.textureCompressionBC                    ? "yes" : "no");
        printf("     occlusionQueryPrecise:                     %s\n", features.occlusionQueryPrecise                   ? "yes" : "no");
        printf("     pipelineStatisticsQuery:                   %s\n", features.pipelineStatisticsQuery                 ? "yes" : "no");
        printf("     vertexPipelineStoresAndAtomics:            %s\n", features.vertexPipelineStoresAndAtomics          ? "yes" : "no");
        printf("     fragmentStoresAndAtomics:                  %s\n", features.fragmentStoresAndAtomics                ? "yes" : "no");
        printf("     shaderTessellationAndGeometryPointSize:    %s\n", features.shaderTessellationAndGeometryPointSize  ? "yes" : "no");
        printf("     shaderImageGatherExtended:                 %s\n", features.shaderImageGatherExtended               ? "yes" : "no");
        printf("     shaderStorageImageExtendedFormats:         %s\n", features.shaderStorageImageExtendedFormats       ? "yes" : "no");
        printf("     shaderStorageImageMultisample:             %s\n", features.shaderStorageImageMultisample           ? "yes" : "no");
        printf("     shaderStorageImageReadWithoutFormat:       %s\n", features.shaderStorageImageReadWithoutFormat     ? "yes" : "no");
        printf("     shaderStorageImageWriteWithoutFormat:      %s\n", features.shaderStorageImageWriteWithoutFormat    ? "yes" : "no");
        printf("     shaderUniformBufferArrayDynamicIndexing:   %s\n", features.shaderUniformBufferArrayDynamicIndexing ? "yes" : "no");
        printf("     shaderSampledImageArrayDynamicIndexing:    %s\n", features.shaderSampledImageArrayDynamicIndexing  ? "yes" : "no");
        printf("     shaderStorageBufferArrayDynamicIndexing:   %s\n", features.shaderStorageBufferArrayDynamicIndexing ? "yes" : "no");
        printf("     shaderStorageImageArrayDynamicIndexing:    %s\n", features.shaderStorageImageArrayDynamicIndexing  ? "yes" : "no");
        printf("     shaderClipDistance:                        %s\n", features.shaderClipDistance                      ? "yes" : "no");
        printf("     shaderCullDistance:                        %s\n", features.shaderCullDistance                      ? "yes" : "no");
        printf("     shaderFloat64:                             %s\n", features.shaderFloat64                           ? "yes" : "no");
        printf("     shaderInt64:                               %s\n", features.shaderInt64                             ? "yes" : "no");
        printf("     shaderInt16:                               %s\n", features.shaderInt16                             ? "yes" : "no");
        printf("     shaderResourceResidency:                   %s\n", features.shaderResourceResidency                 ? "yes" : "no");
        printf("     shaderResourceMinLod:                      %s\n", features.shaderResourceMinLod                    ? "yes" : "no");
        printf("     sparseBinding:                             %s\n", features.sparseBinding                           ? "yes" : "no");
        printf("     sparseResidencyBuffer:                     %s\n", features.sparseResidencyBuffer                   ? "yes" : "no");
        printf("     sparseResidencyImage2D:                    %s\n", features.sparseResidencyImage2D                  ? "yes" : "no");
        printf("     sparseResidencyImage3D:                    %s\n", features.sparseResidencyImage3D                  ? "yes" : "no");
        printf("     sparseResidency2Samples:                   %s\n", features.sparseResidency2Samples                 ? "yes" : "no");
        printf("     sparseResidency4Samples:                   %s\n", features.sparseResidency4Samples                 ? "yes" : "no");
        printf("     sparseResidency8Samples:                   %s\n", features.sparseResidency8Samples                 ? "yes" : "no");
        printf("     sparseResidency16Samples:                  %s\n", features.sparseResidency16Samples                ? "yes" : "no");
        printf("     sparseResidencyAliased:                    %s\n", features.sparseResidencyAliased                  ? "yes" : "no");
        printf("     variableMultisampleRate:                   %s\n", features.variableMultisampleRate                 ? "yes" : "no");
        printf("     inheritedQueries:                          %s\n", features.inheritedQueries                        ? "yes" : "no");

        printf("  \nMemory types:\n");
        printf("               | Flags                                                                                                                              |\n");
        printf("    idx | heap | device local | host visible | host coherent | host cached | lazy | protected | device coherent AMD | device uncached AMD | RDMA NV |\n");
        printf("    -------------------------------------------------------------------------------------------------------------------------------------------------\n");
        for (int i = 0; i < memProps.memoryTypeCount; i++) {
            VkMemoryType& type = memProps.memoryTypes[i];
            printf("    %3d | %4d | %12c | %12c | %13c | %11c | %4c | %9c | %19c | %19c | %7c\n",
                i,
                type.heapIndex,
                type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT        ? 'x' : '-',
                type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT        ? 'x' : '-',
                type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT       ? 'x' : '-',
                type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT         ? 'x' : '-',
                type.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT    ? 'x' : '-',
                type.propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT           ? 'x' : '-',
                type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD ? 'x' : '-',
                type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD ? 'x' : '-',
                type.propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV     ? 'x' : '-'
            );
        }
        printf("  \nMemory heaps: %d\n", memProps.memoryHeapCount);
        printf("                       | Flags                                                                 |\n");
        printf("    idx |         size | device local | multi instance | tile memory qcom | multi instance khr |\n");
        printf("    --------------------------------------------------------------------------------------------\n");
        for (int i = 0; i < memProps.memoryHeapCount; i++) {
            VkMemoryHeap& heap = memProps.memoryHeaps[i];
            printf("    %3d | %12" PRIu64 " | %12c | %14c | %16c | %18c\n",
                i,
                heap.size,
                heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT       ? 'x' : '-',
                heap.flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT     ? 'x' : '-',
                heap.flags & VK_MEMORY_HEAP_TILE_MEMORY_BIT_QCOM   ? 'x' : '-',
                heap.flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR ? 'x' : '-'
            );
        }

        printf("  \nQueue Families: %zu\n", queueFamilies.size());
        printf("                |  Flags                                                                                                    |\n");
        printf("    idx | Count |  Graphics |   Compute |  Transfer |    Sparse | Protected | VidDecKhr | VidEncKhr |   OptFlow | DataGraph | timestamp | min image trans |\n");
        printf("    -------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        for (int i = 0; i < queueFamilies.size(); i++) {
            VkQueueFamilyProperties& props = queueFamilies[i];
            printf("    %3d | %5d | %9c | %9c | %9c | %9c | %9c | %9c | %9c | %9c | %9c | %9d | %d x %d x %d \n",
                i,
                props.queueCount,
                props.queueFlags & VK_QUEUE_GRAPHICS_BIT         ? 'x' : '-',
                props.queueFlags & VK_QUEUE_COMPUTE_BIT          ? 'x' : '-',
                props.queueFlags & VK_QUEUE_TRANSFER_BIT         ? 'x' : '-',
                props.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT   ? 'x' : '-',
                props.queueFlags & VK_QUEUE_PROTECTED_BIT        ? 'x' : '-',
                props.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR ? 'x' : '-',
                props.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR ? 'x' : '-',
                props.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV  ? 'x' : '-',
                props.queueFlags & VK_QUEUE_DATA_GRAPH_BIT_ARM   ? 'x' : '-',
                props.timestampValidBits,
                props.minImageTransferGranularity.width,
                props.minImageTransferGranularity.height,
                props.minImageTransferGranularity.depth
            );
        }

        printf("  \nExtensions: %zu\n", devExtensions.size());
        for(int i = 0; i < devExtensions.size(); i++) {
            VkExtensionProperties& ext = devExtensions[i];
            printf("    %6d :: %s\n", ext.specVersion, ext.extensionName);
        }
    }
};















////////////////////////////////////////////////////////////////////////////////////////////////////
// SWAPCHAIN

class SwapchainVk {
public:
    bool init(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, uint32_t gIdx, uint32_t pIdx, VkQueue pQueue, uint32_t deviceMem);
    void terminate();
    bool recreate();
    void resize();
    bool next(VkSemaphore signal);
    bool recreated();
    bool present();

    VkSwapchainKHR              swapchain = VK_NULL_HANDLE;
    VkFormat                    format;
    VkPresentModeKHR            presentMode;
    VkExtent2D                  extent;
    std::vector<VkImage>        images;
    std::vector<VkImageView>    imageViews;
    VkImage                     depthImage  = VK_NULL_HANDLE;
    VkImageView                 depthView   = VK_NULL_HANDLE;
    VkDeviceMemory              depthMemory = VK_NULL_HANDLE;
    uint32_t                    idx;
    VkSemaphore                 renderReady;
private:
    VkDevice                    device;
    VkSurfaceKHR                surface;
    VkPhysicalDevice            physicalDevice;
    uint32_t                    gIdx;
    uint32_t                    pIdx;
    VkQueue                     pQueue;
    uint32_t                    deviceMem;
    bool                        resizedFlag = false;
    bool                        recreatedFlag = false;
    std::vector<VkSemaphore>    renderReadyVec;
};

bool SwapchainVk::init(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, uint32_t gIdx, uint32_t pIdx, VkQueue pQueue, uint32_t deviceMem) {
    this->device = device;
    this->surface = surface;
    this->physicalDevice = physicalDevice;
    this->gIdx = gIdx;
    this->pIdx = pIdx;
    this->pQueue = pQueue;
    this->deviceMem = deviceMem;
    recreate();
    return true;
}

void SwapchainVk::terminate() {
    if (depthView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, depthView, nullptr);
    }
    if (depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, depthImage, nullptr);
    }
    if (depthMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, depthMemory, nullptr);
    }
    for (auto v : renderReadyVec) {
        vkDestroySemaphore(device, v, nullptr);
    }
    for (auto imageView : imageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    if (swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }
}

bool SwapchainVk::recreate() {
    terminate();

    VkSurfaceCapabilitiesKHR            surfaceCapabilities;
    std::vector<VkPresentModeKHR>       surfacePresentModes;
    std::vector<VkSurfaceFormatKHR>     surfaceFormats;
    ASSERT_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));
    ASSERT(getPhysicalDeviceSurfaceFormatsKHR          (physicalDevice, surface, surfaceFormats));
    ASSERT(getPhysicalDeviceSurfacePresentModesKHR     (physicalDevice, surface, surfacePresentModes));
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    format = surfaceFormats[0].format;
    presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& x : surfaceFormats) {
        if (x.format == VK_FORMAT_B8G8R8A8_UNORM && x.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format = x.format;
        }
    }
    for (const auto& x : surfacePresentModes) {
        if (x == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = x;
        }
    }
    VkSurfaceTransformFlagBitsKHR currTransform = surfaceCapabilities.currentTransform;
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent =  surfaceCapabilities.currentExtent;
    } else {
        // VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        // actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        // actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        // extent = actualExtent;
    }
    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }
    
    bool uniqueQueue = gIdx == pIdx;
    uint32_t queueIndices[] = { gIdx, pIdx };
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface                = surface,
        .minImageCount          = imageCount,
        .imageFormat            = format,
        .imageColorSpace        = colorSpace,
        .imageExtent            = extent,
        .imageArrayLayers       = 1,
        .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode       = uniqueQueue ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount  = uniqueQueue ? 1u : 2u,
        .pQueueFamilyIndices    = queueIndices,
        .preTransform           = currTransform,
        .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode            = presentMode,
        .clipped                = VK_TRUE,
        .oldSwapchain           = VK_NULL_HANDLE,
    };
    GUARDV(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain));
    GUARD(getSwapchainImagesKHR(device, swapchain, images));
    renderReadyVec.resize(images.size());
    imageViews.resize(images.size());
    for (size_t i = 0; i < images.size(); i++) {
        VkImageViewCreateInfo createInfo = {
            .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext              = nullptr,
            .image              = images[i],
            .viewType           = VK_IMAGE_VIEW_TYPE_2D,
            .format             = format,
            .components         = {},
            .subresourceRange   = {
                .aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel    = 0,
                .levelCount      = 1,
                .baseArrayLayer  = 0,
                .layerCount      = 1,
            },
        };
        GUARDV(vkCreateImageView(device, &createInfo, nullptr, &imageViews[i]));
        GUARDV(createSemaphore(device, renderReadyVec[i]));
    }

    VkImageCreateInfo depthInfo = {
        .sType                  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType              = VK_IMAGE_TYPE_2D,
        .format                 = VK_FORMAT_D32_SFLOAT,
        .extent                 = { extent.width, extent.height, 1 },
        .mipLevels              = 1,
        .arrayLayers            = 1,
        .samples                = VK_SAMPLE_COUNT_1_BIT,
        .tiling                 = VK_IMAGE_TILING_OPTIMAL,
        .usage                  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode            = uniqueQueue ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount  = uniqueQueue ? 1u : 2u,
        .pQueueFamilyIndices    = queueIndices,
        .initialLayout          = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    ASSERT_VK(vkCreateImage(device, &depthInfo, nullptr, &depthImage));
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, depthImage, &memReq);
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReq.size,
        .memoryTypeIndex = deviceMem,
    };
    ASSERT_VK(vkAllocateMemory(device, &allocInfo, nullptr, &depthMemory));
    ASSERT_VK(vkBindImageMemory(device, depthImage, depthMemory, 0));

    
    VkImageViewCreateInfo depthViewInfo = {
        .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext              = nullptr,
        .image              = depthImage,
        .viewType           = VK_IMAGE_VIEW_TYPE_2D,
        .format             = VK_FORMAT_D32_SFLOAT,
        .components         = {},
        .subresourceRange   = {
            .aspectMask      = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel    = 0,
            .levelCount      = 1,
            .baseArrayLayer  = 0,
            .layerCount      = 1,
        },
    };
    GUARDV(vkCreateImageView(device, &depthViewInfo, nullptr, &depthView));
    
    return true;
}

void SwapchainVk::resize() {
    resizedFlag = true;
}

bool SwapchainVk::next(VkSemaphore signal) {
    VkResult res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, signal, VK_NULL_HANDLE, &idx);
    // printf("Swapchain index %d\n", idx);
    renderReady = renderReadyVec[idx];
    if (res == VK_ERROR_OUT_OF_DATE_KHR || resizedFlag) {
        resizedFlag = false;
        recreatedFlag = true;
        recreate();
        // createSwapchainFramebuffers(device, swapchainCtx, renderPass);
    } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        printf("Acquire image error\n");
        return false;
    }
    return true;
}

bool SwapchainVk::recreated() {
    bool temp = recreatedFlag;
    recreatedFlag = false;
    return temp;
}

bool SwapchainVk::present() {
    VkPresentInfoKHR presentInfo = {
        .sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount  = 1,
        .pWaitSemaphores     = &renderReady,
        .swapchainCount      = 1,
        .pSwapchains         = &swapchain,
        .pImageIndices       = &idx,
    };
    vkQueuePresentKHR(pQueue, &presentInfo);
    return true;
}






















































































////////////////////////////////////////////////////////////////////////////////////////////////////
/// INTERFACE //////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


struct QueueData {
    VkQueue     queue;
    // VkFence     fence;
    // HANDLE      fenceEvent;
    // UINT64      nextFenceValue;
};

struct CommandData {
    VkCommandPool       cmdPool;
    VkCommandBuffer     cmdBuffer;
    VkSemaphore         waitSemaphore;
    VkSemaphore         signalSemaphore;
    VkFence             fence;
    Buffer              buffer;
    uint8_t*            data;
    uint32_t            maxMemory;
    uint32_t            allocatedMemory;
    VkDescriptorSet     descriptorSet;
    Root                currentRoot = {-1};
};

struct RootData {
    VkPipelineLayout obj;
};

struct PipelineData {
    VkPipeline obj;
};


struct SwapchainData {
    VkSwapchainKHR                          swapchain;
    VkFormat                                format;
    VkPresentModeKHR                        presentMode;
    VkExtent2D                              extent;
    uint32_t                                idx;
    VkSemaphore                             renderReady;
    bool                                    resizedFlag = false;
    bool                                    recreatedFlag = false;
    std::vector<VkSemaphore>                renderReadyVec;
    // std::vector<SwapTarget>                 targets;
    std::vector<VkImage>                    images;
    std::vector<VkImageView>                imageViews;
    UINT                                    frameIdx;
};

struct BufferData {
    VkBuffer        obj;
    VkDeviceMemory  memory;
    uint8_t*        data;
    uint32_t        size;
    // BufferUsage                             usage;
    // D3D12_HEAP_TYPE                         heap;
    // D3D12_RESOURCE_STATES                   state;
    // D3D12_RESOURCE_FLAGS                    flags;
};

struct TextureData {
    VkImage     texture;
};

enum TextureViewAspect {
    TextureViewColor,
    TextureViewDepth,
    TextureViewRender
};

struct TextureViewData {
    TextureViewAspect aspect;
    VkImageView       view;
};



class GpuVk: public IGpu {

public:
    GpuVk(const GpuDesc& gpuDesc) {
        const char* applicationName = "GpuVk";
        uint32_t version = 0;
        bool graphical = true;
        bool compute   = true;
        #ifdef NDEBUG
        bool debug = false;
        #else
        bool debug = true;
        #endif
        commands .init(gpuDesc.commandCount);
        roots    .init(gpuDesc.rootCount);
        pipelines.init(gpuDesc.pipelineCount);
        buffers  .init(gpuDesc.bufferCount);



        // INSTANCE ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        uint32_t v = 0;
        vkEnumerateInstanceVersion(&v);
        printf("Supported Vulkan %u.%u.%u\n", VK_VERSION_MAJOR(v), VK_VERSION_MINOR(v), VK_VERSION_PATCH(v));

        VkInstanceCreateFlags flags = 0;
        if (debug) {
            instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        if (graphical) {
            instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
            instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        }
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{
            .sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback    = debugCallback,
        };
        VkApplicationInfo appInfo = {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName   = applicationName,
            .applicationVersion = version,
            .pEngineName        = "No Engine",
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_0,
        };
        VkInstanceCreateInfo createInfo = {
            .sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext                    = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo,
            .flags                    = flags,
            .pApplicationInfo         = &appInfo,
            .enabledLayerCount        = static_cast<uint32_t>(instanceLayers.size()),
            .ppEnabledLayerNames      = instanceLayers.data(),
            .enabledExtensionCount    = static_cast<uint32_t>(instanceExtensions.size()),
            .ppEnabledExtensionNames  = instanceExtensions.data(),
        };
        ASSERT_VK(vkCreateInstance(&createInfo, nullptr, &instance));
        if (debug) {
            createDebugMessengerFn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            destroyDebugMessengerFn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            ASSERT(createDebugMessengerFn != nullptr);
            ASSERT(destroyDebugMessengerFn != nullptr);
            ASSERT_VK(createDebugMessengerFn(instance, &debugCreateInfo, nullptr, &debugMessenger));
        }

        // SURFACE /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        #ifdef WIN32
        HINSTANCE hInstance = GetModuleHandle(nullptr);
        auto hwnd = static_cast<HWND>(gpuDesc.window);
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
            .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext     = nullptr,
            .hinstance = hInstance,
            .hwnd      = hwnd,
        };
        ASSERT_VK(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface));
        #else
        #endif



        // PHYSICAL DEVICE /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        std::vector<VkPhysicalDevice> pds;
        ASSERT(enumeratePhysicalDevices(instance, pds));
        physicalDevices.resize(pds.size());
        for (int i = 0; i < pds.size(); i++) {
            physicalDevices[i].init(pds[i], surface);
        }
        selectPhysicalDevice({ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, true, true);



        // DEVICE //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        std::vector<const char*> layers;
        std::vector<const char*> extensions;
        float queuePriority = 1.0f;
        std::set<uint32_t> queueFamilyIdcs;
    #ifndef NDEBUG
        layers.push_back("VK_LAYER_KHRONOS_validation");
    #endif
        if (graphical) {
            extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            queueFamilyIdcs.insert(selectedPhysicalDevice->graphicsFamilyIdx);
            queueFamilyIdcs.insert(selectedPhysicalDevice->presentFamilyIdx);
        }
        if (compute) {
            queueFamilyIdcs.insert(selectedPhysicalDevice->computeFamilyIdx);
        }
    
        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        for (auto idx : queueFamilyIdcs) {
            queueInfos.push_back({ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, idx, 1, &queuePriority });
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        VkDeviceCreateInfo deviceCreateInfo = {
            .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount       = static_cast<uint32_t>(queueInfos.size()),
            .pQueueCreateInfos          = queueInfos.data(),
            .enabledLayerCount          = static_cast<uint32_t>(layers.size()),
            .ppEnabledLayerNames        = layers.data(),
            .enabledExtensionCount      = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames    = extensions.data(),
            .pEnabledFeatures           = &deviceFeatures,
        };
        ASSERT_VK(vkCreateDevice(selectedPhysicalDevice->obj, &deviceCreateInfo, nullptr, &device));
        
        gIdx = selectedPhysicalDevice->graphicsFamilyIdx;
        pIdx = selectedPhysicalDevice->presentFamilyIdx;
        cIdx = selectedPhysicalDevice->computeFamilyIdx;
        if (graphical) {
            vkGetDeviceQueue(device, selectedPhysicalDevice->graphicsFamilyIdx, 0, &gQ);
            vkGetDeviceQueue(device, selectedPhysicalDevice->presentFamilyIdx,  0, &pQ);
        }
        if (compute) {
            vkGetDeviceQueue(device, selectedPhysicalDevice->computeFamilyIdx,  0, &cQ);
        }
        uploadMem = selectedPhysicalDevice->uploadMemoryIdx;
        deviceMem = selectedPhysicalDevice->deviceMemoryIdx;



        // DESCRIPTORS /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        VkDescriptorPoolSize poolSize = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = gpuDesc.commandCount,
        };
        VkDescriptorPoolCreateInfo poolCreateInfo = {
            .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets        = 1024,
            .poolSizeCount  = 1,
            .pPoolSizes     = &poolSize,
        };
        ASSERT_VK(vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool));
        
        // Define UBO set layout for uniform buffers used in command abstraction
        VkDescriptorSetLayoutBinding uboBinding = {
            .binding            = 0,
            .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        };
        VkDescriptorSetLayoutCreateInfo setLayoutInfo = {
            .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount  = 1,
            .pBindings      = &uboBinding,
        };
        ASSERT_VK(vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &uboSetLayout));



        // SWAPCHAIN/RENDERPASS ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ASSERT(swapchain.init(device, surface, selectedPhysicalDevice->obj, selectedPhysicalDevice->graphicsFamilyIdx, selectedPhysicalDevice->presentFamilyIdx, pQ, deviceMem));

        VkAttachmentDescription colorAttachment = {
            .format                  = swapchain.format,
            .samples                 = VK_SAMPLE_COUNT_1_BIT,
            .loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp                 = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };
        VkAttachmentDescription depthAttachment = {
            .format                  = VK_FORMAT_D32_SFLOAT,
            .samples                 = VK_SAMPLE_COUNT_1_BIT,
            .loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
        VkAttachmentReference colorAttachmentRef = {
            .attachment              = 0,
            .layout                  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        VkAttachmentReference depthAttachmentRef = {
            .attachment              = 1,
            .layout                  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
        VkSubpassDescription subpass = {
            .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount    = 1,
            .pColorAttachments       = &colorAttachmentRef,
            .pDepthStencilAttachment = &depthAttachmentRef,
        };
        VkSubpassDependency dependency = {
            .srcSubpass              = VK_SUBPASS_EXTERNAL,
            .dstSubpass              = 0,
            .srcStageMask            = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask            = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask           = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask           = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        };
        VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };
        VkRenderPassCreateInfo renderPassInfo = {
            .sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount         = 2,
            .pAttachments            = attachments,
            .subpassCount            = 1,
            .pSubpasses              = &subpass,
            .dependencyCount         = 1,
            .pDependencies           = &dependency,
        };
        ASSERT_VK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

        framebuffers.resize(swapchain.imageViews.size());
        for (size_t i = 0; i < swapchain.imageViews.size(); i++) {
            VkImageView attachments[] = { swapchain.imageViews[i], swapchain.depthView };
            VkFramebufferCreateInfo framebufferInfo = {
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = renderPass,
                .attachmentCount = 2,
                .pAttachments    = attachments,
                .width           = swapchain.extent.width,
                .height          = swapchain.extent.height,
                .layers          = 1,
            };
            ASSERT_VK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]));
        }
    }

    ~GpuVk() {
        terminate();
    }

    void terminate() {
        vkDeviceWaitIdle            (device);
        vkDestroyRenderPass         (device, renderPass, nullptr);
        vkDestroyDescriptorSetLayout(device, uboSetLayout,   nullptr);
        vkDestroyDescriptorPool     (device, descriptorPool, nullptr);
        for (int i = 0; i < roots.end(); i++) {
            if (!roots.exists(i)) continue;
            RootData& rootData = roots[i];
            vkDestroyPipelineLayout (device, rootData.obj, nullptr);
        }
        for (int i = 0; i < framebuffers.size(); i++) {
            vkDestroyFramebuffer    (device, framebuffers[i], nullptr);
        }
        for (int i = 0; i < commands.end(); i++) {
            if (!commands.exists(i)) continue;
            CommandData& commandData = commands[i];
            vkDestroySemaphore      (device, commandData.waitSemaphore,   nullptr);
            vkDestroySemaphore      (device, commandData.signalSemaphore, nullptr);
            vkDestroyFence          (device, commandData.fence,           nullptr);
            vkDestroyCommandPool    (device, commandData.cmdPool,         nullptr);
            destroy(commandData.buffer);
        }
        for (int i = 0; i < buffers.end(); i++) {
            if (!buffers.exists(i)) continue;
            destroy(Buffer{i});
        }
        for (int i = 0; i < pipelines.end(); i++) {
            if (!pipelines.exists(i)) continue;
            PipelineData& pipelineData = pipelines[i];
            vkDestroyPipeline       (device, pipelineData.obj, nullptr);
        }
        swapchain.terminate();
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        destroyDebugMessengerFn(instance, debugMessenger, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    void print() {
    }

    void printErrors() {
    }


    Queue createQueue(QueueType type = QueueGraphics) {
        return { -1 };
    }

    Command createCommand(QueueType type = QueueGraphics, uint32_t maxMemory = 0) {
        Command command = commands.alloc();
        CommandData& commandData = commands[command];
        VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = selectedPhysicalDevice->graphicsFamilyIdx,
        };
        ASSERT_VK(vkCreateCommandPool(device, &poolInfo, nullptr, &commandData.cmdPool));

        VkCommandBufferAllocateInfo allocInfo = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = commandData.cmdPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        ASSERT_VK(vkAllocateCommandBuffers(device, &allocInfo, &commandData.cmdBuffer));
        ASSERT_VK(createSemaphore(device, commandData.waitSemaphore));
        ASSERT_VK(createFence(device, commandData.fence, true));
    
        if (maxMemory > 0) {
            VkDescriptorSetAllocateInfo setAllocInfo = {
                .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool     = descriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts        = &uboSetLayout,
            };
            ASSERT_VK(vkAllocateDescriptorSets(device, &setAllocInfo, &commandData.descriptorSet));

            commandData.buffer = createBuffer("ubo", BufferUsage::BufferConstant, maxMemory);
            BufferData& bufferData = buffers[commandData.buffer];
            VkDescriptorBufferInfo descBufferInfo = {
                .buffer = bufferData.obj,
                .offset = 0,
                .range  = maxMemory <= 65536 ? maxMemory : 65536,
            };
            VkWriteDescriptorSet descWriteInfo = {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet           = commandData.descriptorSet,
                .dstBinding       = 0,
                .dstArrayElement  = 0,
                .descriptorCount  = 1,
                .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo       = nullptr,
                .pBufferInfo      = &descBufferInfo,
                .pTexelBufferView = nullptr,
            };
            vkUpdateDescriptorSets(device, 1, &descWriteInfo, 0, nullptr);
        }
        return command;
    }

    Root createRoot(const std::vector<RootParam>& params, const std::vector<Sampler>& samplers) {
        Root root = roots.alloc();
        RootData& rootData = roots[root];
        // A single pipeline layout for now
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount         = 1,
            .pSetLayouts            = &uboSetLayout,
            .pushConstantRangeCount = 0,
        };
        ASSERT_VK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &rootData.obj));
        return root;
    }

    Pipeline createPipeline(Root root, const char* shader) {
        return { -1 };

    }

    Pipeline createPipeline(Root root, PsoGraphicsDesc desc) {
        Pipeline pipeline = pipelines.alloc();
        PipelineData& pipelineData = pipelines[pipeline];
        RootData& rootData = roots[root];

        Shader vertShader;
        Shader fragShader;
        ASSERT(vertShader.load(device, desc.vs2, VK_SHADER_STAGE_VERTEX_BIT));
        ASSERT(fragShader.load(device, desc.ps2, VK_SHADER_STAGE_FRAGMENT_BIT));
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShader.getInfo(), fragShader.getInfo() };

        VkPolygonMode   polygonMode = getNativeFillMode(desc.fillMode);
        VkCullModeFlags cullMode    = getNativeCullMode(desc.cullMode);

        VkVertexInputBindingDescription bindingDesc[] = {
            {
                .binding = 0,
                .stride = sizeof(float) * 3,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            },
            {
                .binding = 1,
                .stride = sizeof(float) * 3,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            },
        };
        VkVertexInputAttributeDescription attributeDesc[] = {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = 0,
            },
            {
                .location = 1,
                .binding = 1,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = 0,
            }
        };
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 2,
            .pVertexBindingDescriptions      = bindingDesc,
            .vertexAttributeDescriptionCount = 2,
            .pVertexAttributeDescriptions    = attributeDesc,
        };
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };
        VkPipelineViewportStateCreateInfo viewportState = {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount  = 1,
        };
        VkPipelineRasterizationStateCreateInfo rasterizer = {
            .sType                    = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable         = VK_FALSE,
            .rasterizerDiscardEnable  = VK_FALSE,
            .polygonMode              = polygonMode,
            .cullMode                 = cullMode,
            .frontFace                = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable          = VK_FALSE,
            .lineWidth                = 1.0f,
        };
        VkPipelineMultisampleStateCreateInfo multisampling = {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable   = VK_FALSE,
        };
        VkPipelineDepthStencilStateCreateInfo depthStencil = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable       = VK_TRUE,
            .depthWriteEnable      = VK_TRUE,
            .depthCompareOp        = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable     = VK_FALSE,
            .front                 = {},
            .back                  = {},
            .minDepthBounds        = 0.0f,
            .maxDepthBounds        = 1.0f,
        };
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .blendEnable    = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
        VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable     = VK_FALSE,
            .logicOp           = VK_LOGIC_OP_COPY,
            .attachmentCount   = 1,
            .pAttachments      = &colorBlendAttachment,
            .blendConstants    = { 0, 0, 0, 0 },
        };
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };
        VkPipelineDynamicStateCreateInfo dynamicState = {
            .sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount  = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates     = dynamicStates.data(),
        };
        VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount          = 2,
            .pStages             = shaderStages,
            .pVertexInputState   = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState      = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState  = &depthStencil,
            .pColorBlendState    = &colorBlending,
            .pDynamicState       = &dynamicState,
            .layout              = rootData.obj,
            .renderPass          = renderPass,
            .subpass             = 0,
            .basePipelineHandle  = VK_NULL_HANDLE,
        };
        ASSERT_VK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipelineData.obj));
        // vkDestroyPipeline (device, graphicsPipeline, nullptr);
        return pipeline;
    }

    Swapchain createSwapchain(Queue& queue, void* window, uint32_t w, uint32_t h, uint32_t frameCount) {
        return { -1 };

    }

    Buffer createBuffer(const char* name, BufferUsage usage, uint32_t size) {
        Buffer buffer = buffers.alloc();
        BufferData& bufferData = buffers[buffer];
        uint32_t memoryType = uploadMem;
        VkBufferUsageFlags usageFlags = 0;
        
        if (usage == BufferUsage::BufferConstant) {
            usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        } else {
            usageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            usageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        VkBufferCreateInfo info = {
            .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = 0,
            .size                  = size,
            .usage                 = usageFlags,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices   = nullptr,
        };
        ASSERT_VK(vkCreateBuffer(device, &info, nullptr, &bufferData.obj));
        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device, bufferData.obj, &memReq);
        ASSERT(memReq.memoryTypeBits & (1 << memoryType));
        VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = memReq.size,
            .memoryTypeIndex = memoryType,
        };
        ASSERT_VK(vkAllocateMemory(device, &allocInfo, nullptr, &bufferData.memory));
        ASSERT_VK(vkBindBufferMemory(device, bufferData.obj, bufferData.memory, 0));
        vkMapMemory(device, bufferData.memory, 0, size, 0, reinterpret_cast<void**>(&bufferData.data));
        // vkUnmapMemory(device, bufferData.uploadMemory);
        bufferData.size = size;
        return buffer;
    }


    Texture createTexture(const char* name, const uint8_t* data, uint32_t size, std::vector<ResourceData>& subresources) {
        return { -1 };

    }

    Texture createTexture1(TextureUsage usage, Format format, uvec3 dims, uint16_t mips = 1, ClearValue value = {}) {
        return { -1 };

    }

    Texture createTexture2(const char* name, TextureUsage usage, Format format, uvec3 dims, uint16_t mips = 1, uint8_t samples = 1, ClearValue value = {}) {
        return { -1 };

    }

    Texture createTexture3(TextureUsage usage, Format format, uvec3 dims, uint16_t mips = 1, ClearValue value = {}) {
        return { -1 };

    }

    TextureView createTextureView(Texture texture) {
        return { -1 };

    }

    TextureView createDepthView(Texture texture) {
        return { -1 };

    }

    TextureView createRenderView(Texture texture) {
        return { -1 };

    }

    int getTextureViewBind(TextureView textureView) {
        return 0;

    }


    void destroy(Buffer buffer) {
        BufferData& bufferData = buffers[buffer];
        vkDestroyBuffer(device, bufferData.obj, nullptr);
        vkFreeMemory(device, bufferData.memory, nullptr);
        buffers.free(buffer);
    }

    void destroy(Texture texture) {

    }

    void destroy(TextureView textureView) {

    }


    void execute(Queue queue, Command& command) {
        CommandData& commandData = commands[command];
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &commandData.waitSemaphore,
            .pWaitDstStageMask    = waitStages,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &commandData.cmdBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &swapchain.renderReady,
        };
        ASSERT_VK(vkQueueSubmit(gQ, 1, &submitInfo, commandData.fence));
    }

    void signal(Queue queue, Command& command) {

    }

    void wait(Queue queue, Command& command) {
        CommandData& commandData = commands[command];
        vkWaitForFences(device, 1, &commandData.fence, VK_TRUE, UINT64_MAX);
    }

    void signal(Queue queue, uint64_t& value) {

    }

    void wait(Queue queue, uint64_t value) {

    }

    void wait(Queue queue) {
        vkDeviceWaitIdle (device);
    }


    void begin(Command command, Pipeline pipeline = { -1 }) {
        CommandData& commandData = commands[command];
        ASSERT(swapchain.next(commandData.waitSemaphore));
        if (swapchain.recreated()) {
            // renderTarget.recreateFramebuffers(swapchain);
        }
        vkResetFences(device, 1, &commandData.fence);
        vkResetCommandBuffer(commandData.cmdBuffer, 0);
        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        ASSERT_VK(vkBeginCommandBuffer(commandData.cmdBuffer, &beginInfo));
        
        vec4 clearColor = { .1, .1, .1, 1 };
    
        VkClearValue vkClearColor[] = {
            { .color = { clearColor.x, clearColor.y, clearColor.z, clearColor.w }},
            { .depthStencil = { 1.0, 0 }}
        };
        VkRenderPassBeginInfo renderPassInfo = {
            .sType                = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass           = renderPass,
            .framebuffer          = framebuffers[swapchain.idx],
            .renderArea           = { .offset = { 0, 0 }, .extent = swapchain.extent },
            .clearValueCount      = 2,
            .pClearValues         = vkClearColor,
        };
        vkCmdBeginRenderPass(commandData.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        commandData.allocatedMemory = 0;
    }

    void bindHeap(Command command) {

    }

    void viewport(Command command, Viewport& viewport) {
        CommandData& commandData = commands[command];
        VkViewport vkViewport = {
            .x          = viewport.topLeftX,
            .y          = viewport.topLeftY,
            .width      = viewport.width,
            .height     = viewport.height,
            .minDepth   = viewport.minDepth,
            .maxDepth   = viewport.maxDepth,
        };
        vkCmdSetViewport(commandData.cmdBuffer, 0, 1, &vkViewport);
    }

    void scissor(Command command, Rect& rect) {
        CommandData& commandData = commands[command];
        VkRect2D scissor = {
            .offset = { static_cast<int>(rect.left), static_cast<int>(rect.top) },
            .extent = { rect.right - rect.left, rect.bottom - rect.top },
        };
        vkCmdSetScissor(commandData.cmdBuffer, 0, 1, &scissor);
    }

    void targets(Command command, TextureView renderTexture, TextureView depthTexture) {

    }

    void clear(Command command, TextureView renderTexture, vec4 color, TextureView depthTexture, float depth, uint8_t stencil) {

    }

    void pipeline(Command command, Pipeline pipeline) {
        CommandData& commandData = commands[command];
        PipelineData& pipelineData = pipelines[pipeline];
        vkCmdBindPipeline(commandData.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.obj);
    }

    void computeRoot(Command command, Root& root) {

    }

    void computeSrv(Command command, int idx, Buffer& buffer) {

    }

    void computeUav(Command command, int idx, Buffer& buffer) {

    }

    void dispatch(Command command, uint32_t groupsX, uint32_t  groupsY, uint32_t groupsZ) {

    }

    void graphicsRoot(Command command, Root& root) {
        CommandData& commandData = commands[command];
        commandData.currentRoot = root;
    }

    void graphicsCbv(Command command, int idx, const void* data, int dataSize) {
        CommandData& commandData = commands[command];
        RootData& rootData = roots[commandData.currentRoot];
        write(commandData.buffer, commandData.allocatedMemory, dataSize, reinterpret_cast<const uint8_t*>(data));
        commandData.allocatedMemory += dataSize;
        vkCmdBindDescriptorSets(commandData.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rootData.obj, 0, 1, &commandData.descriptorSet, 0, nullptr);
    }

    void graphicsTable(Command command, int idx, int descriptorIdx) {

    }

    void barrier(Command command, Buffer buffer, State state) {

    }

    void barrier(Command command, Texture texture, State state) {

    }

    void copy(Command command, Buffer& dst, Buffer& src) {

    }

    void subresources(Command command, Texture texture, Buffer uploadBuffer, int offset, int subresourceOffset, const std::vector<ResourceData>& subresources) {

    }

    void topology(Command command, PrimitiveTopology primitiveTopology) {

    }

    void vertexBuffer(Command command, uint32_t slot, VertexBufferView view) {
        CommandData& commandData = commands[command];
        BufferData& bufferData = buffers[view.buffer];
        VkBuffer buffers[] = { bufferData.obj };
        VkDeviceSize offsets[] = { view.offset };
        vkCmdBindVertexBuffers(commandData.cmdBuffer, slot, 1, buffers, offsets);
    }

    void indexBuffer(Command command, IndexBufferView view) {
        CommandData& commandData = commands[command];
        BufferData& bufferData = buffers[view.buffer];
        vkCmdBindIndexBuffer(commandData.cmdBuffer, bufferData.obj, view.offset, VK_INDEX_TYPE_UINT16);
    }

    void drawIndexed(Command command, uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, int baseVertexLocation, uint32_t startInstanceLocation) {
        CommandData& commandData = commands[command];
        vkCmdDrawIndexed(commandData.cmdBuffer, indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    }

    void draw(Command command, uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) {
        CommandData& commandData = commands[command];
        vkCmdDraw(commandData.cmdBuffer, vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
    }

    void end(Command command) {
        CommandData& commandData = commands[command];
        vkCmdEndRenderPass(commandData.cmdBuffer);
        ASSERT_VK(vkEndCommandBuffer(commandData.cmdBuffer));
    }



    void present(Swapchain sw, bool vsync) {
        swapchain.present();
    }

    SwapTarget next(Swapchain sw) {
        return {};
    }

    void write(Buffer buffer, uint32_t offset, uint32_t size, const uint8_t* data) {
        BufferData& bufferData = buffers[buffer];
        // ASSERT(bufferData.heap == D3D12_HEAP_TYPE_UPLOAD);
        ASSERT(bufferData.size >= offset + size);
        memcpy(bufferData.data + offset, data, size);
    }

    void read(Buffer buffer, const std::function<void(uint8_t*)>& f) {

    }
// bool copyBuffer(VkQueue q, VkCommandBuffer cmdBuffer, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
//     // GUARDV(vkResetCommandPool(device->device, cmdPool, 0));
//     VkCommandBufferBeginInfo beginInfo = {
//         .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//         .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
//     };
//     VkBufferCopy region = { 0, 0, size };
//     VkSubmitInfo submitInfo = {
//         .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
//         .commandBufferCount = 1,
//         .pCommandBuffers    = &cmdBuffer,
//     };
//     GUARDV(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
//     vkCmdCopyBuffer(cmdBuffer, src, dst, 1, &region);
//     GUARDV(vkEndCommandBuffer(cmdBuffer));
//     GUARDV(vkQueueSubmit(q, 1, &submitInfo, VK_NULL_HANDLE));
//     GUARDV(vkQueueWaitIdle(q));
//     return true;
// }


private:

    bool selectPhysicalDevice(const std::vector<const char*>& reqExtensions, bool graphics, bool compute) {
        std::vector<int> physicalDeviceScores;
        for (int i = 0; i < physicalDevices.size(); i++) {
            PhysicalDevice& physicalDevice = physicalDevices[i];

            std::set<const char*> requiredExtensions(reqExtensions.begin(), reqExtensions.end());
            for (const auto& devExtension : physicalDevice.devExtensions) {
                requiredExtensions.erase(devExtension.extensionName);
            }
            
            bool graphicsCond = true;
            if (graphics) {
                graphicsCond =
                    physicalDevice.graphicsFamilyIdx != -1 &&
                    physicalDevice.presentFamilyIdx != -1 &&
                    physicalDevice.surfaceFormats.size() > 0 &&
                    physicalDevice.surfacePresentModes.size() > 0;
            }

            bool computeCond = true;
            if (compute) {
                computeCond = physicalDevice.computeFamilyIdx != -1;
            }
            
            bool memoryCond = physicalDevice.uploadMemoryIdx != -1 && physicalDevice.deviceMemoryIdx != -1;

            if (selectedPhysicalDevice == nullptr &&
                // requiredExtensions.empty() &&
                graphicsCond &&
                computeCond &&
                memoryCond
            ) {
                selectedPhysicalDevice = &physicalDevices[i];
                return true;
            }
        }
        return false;
    }


    VkInstance                          instance;
    std::vector<const char*>            instanceLayers;
    std::vector<const char*>            instanceExtensions;
    VkDebugUtilsMessengerEXT            debugMessenger;
    PFN_vkCreateDebugUtilsMessengerEXT  createDebugMessengerFn;
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessengerFn;

    VkSurfaceKHR                        surface;
    std::vector<PhysicalDevice>         physicalDevices;
    PhysicalDevice*                     selectedPhysicalDevice = nullptr;

    VkDevice                            device;
    uint32_t                            gIdx;
    uint32_t                            pIdx;
    uint32_t                            cIdx;
    VkQueue                             gQ;
    VkQueue                             pQ;
    VkQueue                             cQ;
    uint32_t                            uploadMem;
    uint32_t                            deviceMem;
    
    SwapchainVk                         swapchain;

    VkRenderPass                        renderPass;
    std::vector<VkFramebuffer>          framebuffers;
    int                                 width;
    int                                 height;
    bool                                sizeChanged = false;
    VkDescriptorSetLayout               uboSetLayout;
    VkDescriptorPool                    descriptorPool;


    Vec<Queue,       QueueData>         queues;
    Vec<Command,     CommandData>       commands;
    Vec<Root,        RootData>          roots;
    Vec<Pipeline,    PipelineData>      pipelines;
    Vec<Swapchain,   SwapchainData>     swapchains;
    Vec<Buffer,      BufferData>        buffers;
    Vec<Texture,     TextureData>       textures;
    Vec<TextureView, TextureViewData>   textureViews;
    uint32_t                            colorViewCount;
    uint32_t                            depthViewCount;
    uint32_t                            renderViewCount;
    uint32_t                            maxColorViews;
    uint32_t                            maxDepthViews;
    uint32_t                            maxRenderViews;
    uint32_t                            maxTextureViews;
};



}

namespace gpu {
    std::unique_ptr<IGpu> createGpuVk(const GpuDesc& desc) {
        return std::make_unique<vk::GpuVk>(desc);
    }
}