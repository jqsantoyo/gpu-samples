#include "utilsVk.h"
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


namespace gpu::vk {






////////////////////////////////////////////////////////////////////////////////////////////////////
// UTLITIES

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




VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    printf("Validation layers: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

bool Instance::init(
    const char* name,
    uint32_t version,
    bool graphical,
    const std::vector<const char*>& ext
) {
#ifdef NDEBUG
    bool debug = false;
#else
    bool debug = true;
#endif

    uint32_t v = 0;
    vkEnumerateInstanceVersion(&v);
    printf("Supported Vulkan %u.%u.%u\n", VK_VERSION_MAJOR(v), VK_VERSION_MINOR(v), VK_VERSION_PATCH(v));

    if (debug) {
      layers.push_back("VK_LAYER_KHRONOS_validation");
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateFlags flags = 0;
    if (graphical) {
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
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
        .pApplicationName   = name,
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
        .enabledLayerCount        = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames      = layers.data(),
        .enabledExtensionCount    = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames  = extensions.data(),
    };
    GUARDV(vkCreateInstance(&createInfo, nullptr, &instance));

    if (debug) {
        PFN_vkCreateDebugUtilsMessengerEXT  createDebugMessengerFn;
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessengerFn;
        createDebugMessengerFn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        destroyDebugMessengerFn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        GUARD(createDebugMessengerFn != nullptr);
        GUARD(destroyDebugMessengerFn != nullptr);
        GUARDV(createDebugMessengerFn(instance, &debugCreateInfo, nullptr, &debugMessenger));
    }
    return true;
}

void Instance::terminate() {
    destroyDebugMessengerFn(instance, debugMessenger, nullptr);
    vkDestroyInstance(instance, nullptr);
}

















////////////////////////////////////////////////////////////////////////////////////////////////////
// SURFACE

bool Surface::init(VkInstance instance, void* window) {
    this->instance = instance;
    #ifdef WIN32
        HINSTANCE hInstance = GetModuleHandle(nullptr);
        auto hwnd = static_cast<HWND>(window);
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
            .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext     = nullptr,
            .hinstance = hInstance,
            .hwnd      = hwnd,
        };
        GUARDV(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface));
    #else
        
    #endif
    return true;
}

void Surface::terminate() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
}

bool Surface::info(VkPhysicalDevice pd, SurfaceInfo& surfaceInfo) {
    GUARDV(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &surfaceInfo.capabilities));
    GUARD(getPhysicalDeviceSurfaceFormatsKHR        (pd, surface, surfaceInfo.formats));
    GUARD(getPhysicalDeviceSurfacePresentModesKHR   (pd, surface, surfaceInfo.presentModes));
    return true;
}











////////////////////////////////////////////////////////////////////////////////////////////////////
// PHYSICAL DEVICE

bool getPhysicalDevices(VkInstance instance, std::vector<PhysicalDeviceData>& phyDevices) {
    std::vector<VkPhysicalDevice> physicalDevices;
    GUARD(enumeratePhysicalDevices(instance, physicalDevices));

    phyDevices.resize(physicalDevices.size());
    for (int i = 0; i < physicalDevices.size(); i++) {
        VkPhysicalDevice& pd = physicalDevices[i];
        PhysicalDeviceData& phyDevice = phyDevices[i];
        enumerateDeviceExtensionProperties       (pd, nullptr, phyDevice.devExtensions);
        vkGetPhysicalDeviceProperties            (pd, &phyDevice.props);
        vkGetPhysicalDeviceFeatures              (pd, &phyDevice.features);
        vkGetPhysicalDeviceMemoryProperties      (pd, &phyDevice.memProps);
        getPhysicalDeviceQueueFamilyProperties   (pd, phyDevice.queueFamilies);
    }
    return true;
}

void PhysicalDeviceData::print() {
    const char* deviceTypeStr = "";
    switch(props.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: deviceTypeStr = "integrated gpu"; break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU  : deviceTypeStr = "discrete gpu";   break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU   : deviceTypeStr = "virtual gpu";    break;
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


bool PhysicalDevice::init(VkInstance instance, const std::vector<const char*>& extensions, bool graphics, bool compute, Surface* surface) {
    std::vector<int> physicalDeviceScores;
    std::vector<VkPhysicalDevice> physicalDevices;
    enumeratePhysicalDevices(instance, physicalDevices);

    int selectedPhysicalDeviceIdx = -1;
    for (int i = 0; i < physicalDevices.size(); i++) {
        VkPhysicalDevice& pd = physicalDevices[i];
        VkPhysicalDeviceProperties           props;
        VkPhysicalDeviceFeatures             features;
        VkPhysicalDeviceMemoryProperties     memProps;
        std::vector<VkQueueFamilyProperties> queueFamilies;
        std::vector<VkExtensionProperties>   devExtensions;
        enumerateDeviceExtensionProperties       (pd, nullptr, devExtensions);
        vkGetPhysicalDeviceProperties            (pd, &props);
        vkGetPhysicalDeviceFeatures              (pd, &features);
        vkGetPhysicalDeviceMemoryProperties      (pd, &memProps);
        getPhysicalDeviceQueueFamilyProperties   (pd, queueFamilies);
        

        int graphicsFamilyIdx = -1;
        int presentFamilyIdx = -1;
        int computeFamilyIdx = -1;
        int uploadMemoryIdx = -1;
        int deviceMemoryIdx = -1;
        std::set<const char*> requiredExtensions(extensions.begin(), extensions.end());
        for (const auto& devExtension : devExtensions) {
            requiredExtensions.erase(devExtension.extensionName);
        }
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
                vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface->surface, &presentSupport);
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
        bool graphicsCond = true;
        if (graphics) {
            SurfaceInfo surfaceInfo;
            surface->info(pd, surfaceInfo);
            graphicsCond =
                graphicsFamilyIdx != -1 &&
                presentFamilyIdx != -1 &&
                surfaceInfo.formats.size() > 0 &&
                surfaceInfo.presentModes.size() > 0;
        }

        bool computeCond = true;
        if (compute) {
            computeCond = computeFamilyIdx != -1;
        }
        
        bool memoryCond = uploadMemoryIdx != -1 && deviceMemoryIdx != -1;

        if (selectedPhysicalDeviceIdx == -1 &&
            // requiredExtensions.empty() &&
            graphicsCond &&
            computeCond &&
            memoryCond
        ) {
            selectedPhysicalDeviceIdx = i;
            gIdx = graphicsFamilyIdx;
            pIdx = presentFamilyIdx;
            cIdx = computeFamilyIdx;
            uploadMem = uploadMemoryIdx;
            deviceMem = deviceMemoryIdx;
        }
    }
    if (selectedPhysicalDeviceIdx != -1) {
        physicalDevice = physicalDevices[selectedPhysicalDeviceIdx];
        return true;
    } else {
        physicalDevice = VK_NULL_HANDLE;
        return false;
    }
    return true;
}








////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVICE

bool Device::init(PhysicalDevice& physicalDevice, bool graphical, bool compute) {
    std::vector<const char*> layers;
    std::vector<const char*> extensions;
    float queuePriority = 1.0f;
    std::set<uint32_t> queueFamilyIdcs;
#ifndef NDEBUG
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
    if (graphical) {
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        queueFamilyIdcs.insert(physicalDevice.gIdx);
        queueFamilyIdcs.insert(physicalDevice.pIdx);
    }
    if (compute) {
        queueFamilyIdcs.insert(physicalDevice.cIdx);
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
    GUARDV(vkCreateDevice(physicalDevice.physicalDevice, &deviceCreateInfo, nullptr, &device));
    
    this->gIdx = physicalDevice.gIdx;
    this->pIdx = physicalDevice.pIdx;
    this->cIdx = physicalDevice.cIdx;
    if (graphical) {
        vkGetDeviceQueue(device, physicalDevice.gIdx, 0, &gQ);
        vkGetDeviceQueue(device, physicalDevice.pIdx, 0, &pQ);
    }
    if (compute) {
        vkGetDeviceQueue(device, physicalDevice.cIdx, 0, &cQ);
    }
    uploadMem = physicalDevice.uploadMem;
    deviceMem = physicalDevice.deviceMem;
    return true;
}

void Device::terminate() {
    vkDestroyDevice(device, nullptr);
}















////////////////////////////////////////////////////////////////////////////////////////////////////
// SWAPCHAIN

bool Swapchain::init(VkDevice device, Surface* surface, VkPhysicalDevice physicalDevice, uint32_t gIdx, uint32_t pIdx, VkQueue pQueue) {
    this->device = device;
    this->surface = surface;
    this->physicalDevice = physicalDevice;
    this->gIdx = gIdx;
    this->pIdx = pIdx;
    this->pQueue = pQueue;
    recreate();
    return true;
}

void Swapchain::terminate() {
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

bool Swapchain::recreate() {
    terminate();

    SurfaceInfo surfaceInfo;
    surface->info(physicalDevice, surfaceInfo);
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    format = surfaceInfo.formats[0].format;
    presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& x : surfaceInfo.formats) {
        if (x.format == VK_FORMAT_B8G8R8A8_UNORM && x.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            format = x.format;
        }
    }
    for (const auto& x : surfaceInfo.presentModes) {
        if (x == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = x;
        }
    }
    VkSurfaceTransformFlagBitsKHR currTransform = surfaceInfo.capabilities.currentTransform;
    if (surfaceInfo.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent =  surfaceInfo.capabilities.currentExtent;
    } else {
        // VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        // actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        // actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        // extent = actualExtent;
    }
    uint32_t imageCount = surfaceInfo.capabilities.minImageCount + 1;
    if (surfaceInfo.capabilities.maxImageCount > 0 && imageCount > surfaceInfo.capabilities.maxImageCount) {
        imageCount = surfaceInfo.capabilities.maxImageCount;
    }
    
    bool uniqueQueue = gIdx == pIdx;
    uint32_t queueIndices[] = { gIdx, pIdx };
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface                = surface->surface,
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
    return true;
}

void Swapchain::resize() {
    resizedFlag = true;
}

bool Swapchain::next(VkSemaphore signal) {
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

bool Swapchain::recreated() {
    bool temp = recreatedFlag;
    recreatedFlag = false;
    return temp;
}

bool Swapchain::present() {
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
// FRAME CONTROL

bool FrameControl::init(VkDevice device, uint32_t queueFamilyIdx, VkQueue queue, int frameCount) {
    this->device = device;
    this->queue = queue;
    this->frameCount = frameCount;
    frameIdx = -1;
    cmdPool.resize(frameCount);
    cmdBuffer.resize(frameCount);
    imageReady.resize(frameCount);
    execution.resize(frameCount);
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIdx,
    };
    for (int i = 0; i < frameCount; i++) {
        GUARDV(vkCreateCommandPool(device, &poolInfo, nullptr, &cmdPool[i]));

        VkCommandBufferAllocateInfo allocInfo = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = cmdPool[i],
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        GUARDV(vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer[i]));
        GUARDV(createSemaphore(device, imageReady[i]));
        GUARDV(createFence(device, execution[i], true));


    }
    return true;
}

void FrameControl::terminate() {
    for (int i = 0; i < frameCount; i++) {
        vkDestroySemaphore       (device, imageReady[i], nullptr);
        vkDestroyFence           (device, execution[i], nullptr);
        vkDestroyCommandPool     (device, cmdPool[i], nullptr);
    }
}

Frame FrameControl::next() {
    frameIdx = (frameIdx + 1) % frameCount;
    vkWaitForFences(device, 1, &execution[frameIdx], VK_TRUE, UINT64_MAX);
    // printf("Active frame: %d\n", frameIdx);
    return { cmdBuffer[frameIdx], imageReady[frameIdx] };
}

bool FrameControl::begin() {
    vkResetFences(device, 1, &execution[frameIdx]);
    vkResetCommandBuffer(cmdBuffer[frameIdx], 0);
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    GUARDV(vkBeginCommandBuffer(cmdBuffer[frameIdx], &beginInfo));
    return true;
}

bool FrameControl::end(VkSemaphore renderReady) {
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &imageReady[frameIdx],
        .pWaitDstStageMask    = waitStages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &cmdBuffer[frameIdx],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &renderReady,
    };
    GUARDV(vkQueueSubmit(queue, 1, &submitInfo, execution[frameIdx]));
    return true;
}














////////////////////////////////////////////////////////////////////////////////////////////////////
// SHADER

Shader::~Shader() {
    vkDestroyShaderModule(device, module, nullptr);
}

bool Shader::load(VkDevice device, const char* name, VkShaderStageFlagBits stage) {
    this->device = device;
    std::string path = getAssetsPath() + name + ".spv";
    FILE* file = fopen(path.c_str(), "rb");
    if (file == nullptr) {
        return false;
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    data.resize(size);
    fread(data.data(), size, 1, file);

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

VkPipelineShaderStageCreateInfo Shader::getInfo() {
    return stageInfo;
}

















////////////////////////////////////////////////////////////////////////////////////////////////////
// MESH CONTROL

bool createBuffer(VkDevice device, uint32_t memoryType, VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo info = {
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .size                  = size,
        .usage                 = usage,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
    };
    GUARDV(vkCreateBuffer(device, &info, nullptr, &buffer));
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, buffer, &memReq);
    GUARD(memReq.memoryTypeBits & (1 << memoryType));
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memReq.size,
        .memoryTypeIndex = memoryType,
    };
    GUARDV(vkAllocateMemory(device, &allocInfo, nullptr, &memory));
    GUARDV(vkBindBufferMemory(device, buffer, memory, 0));
    return true;
}

bool copyBuffer(VkQueue q, VkCommandBuffer cmdBuffer, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkBufferCopy region = { 0, 0, size };
    VkSubmitInfo submitInfo = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmdBuffer,
    };
    GUARDV(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
    vkCmdCopyBuffer(cmdBuffer, src, dst, 1, &region);
    GUARDV(vkEndCommandBuffer(cmdBuffer));
    GUARDV(vkQueueSubmit(q, 1, &submitInfo, VK_NULL_HANDLE));
    GUARDV(vkQueueWaitIdle(q));
    return true;
}

bool MeshControl::init(Device* device) {
    this->device =  device;
    VkCommandPoolCreateInfo poolInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = device->gIdx,
    };
    GUARDV(vkCreateCommandPool(device->device, &poolInfo, nullptr, &cmdPool));
    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = cmdPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    GUARDV(vkAllocateCommandBuffers(device->device, &allocInfo, &cmdBuffer));
    return true;
}

void MeshControl::terminate() {
    for (int i = 0; i < meshes.size(); i++) {
        vkDestroyBuffer(device->device, meshes[i].buffer, nullptr);
        vkFreeMemory(device->device, meshes[i].memory, nullptr);
    }
    vkDestroyCommandPool(device->device, cmdPool, nullptr);
}

bool MeshControl::addMesh(float* data, uint32_t size, uint16_t* idxData, uint32_t idxSize, int& meshIdx) {
    meshIdx = meshes.size();
    meshes.push_back({});
    Mesh& mesh = meshes.back();

    createBuffer(device->device, device->uploadMem, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, uploadBuffer, uploadMemory);
    createBuffer(device->device, device->deviceMem, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, mesh.buffer, mesh.memory);

    void* v;
    vkMapMemory(device->device, uploadMemory, 0, size, 0, &v);
    memcpy(v, data, size);
    vkUnmapMemory(device->device, uploadMemory);

    GUARDV(vkResetCommandPool(device->device, cmdPool, 0));
    GUARD(copyBuffer(device->gQ, cmdBuffer, uploadBuffer, mesh.buffer, size));

    vkDestroyBuffer(device->device, uploadBuffer, nullptr);
    vkFreeMemory(device->device, uploadMemory, nullptr);

    
    if (idxData != nullptr) {
        createBuffer(device->device, device->uploadMem, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, uploadBuffer, uploadMemory);
        createBuffer(device->device, device->deviceMem, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, mesh.idxBuffer, mesh.idxMemory);

        void* v;
        vkMapMemory(device->device, uploadMemory, 0, idxSize, 0, &v);
        memcpy(v, idxData, idxSize);
        vkUnmapMemory(device->device, uploadMemory);

        GUARDV(vkResetCommandPool(device->device, cmdPool, 0));
        GUARD(copyBuffer(device->gQ, cmdBuffer, uploadBuffer, mesh.idxBuffer, idxSize));
        
        vkDestroyBuffer(device->device, uploadBuffer, nullptr);
        vkFreeMemory(device->device, uploadMemory, nullptr);
    }
    return true;
}



Mesh& MeshControl::getMesh(int i) {
    return meshes[i];
}

}
