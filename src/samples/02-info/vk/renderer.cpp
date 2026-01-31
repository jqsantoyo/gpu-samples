#include <renderer/renderer.h>
#include <app/app.h>
#include <utils/utils.h>
#define UNICODE
#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define VK_USE_PLATFORM_WIN32_KHR
#include <windows.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <inttypes.h>

#define GUARDV(x) if ((x != VK_SUCCESS)) {  printf("Error: "#x); return 0; }
#define GUARD(x)  if (!(x))              {  printf("Error: "#x); return 0; }
namespace gpu {


const int frameCount = 2;


VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}


class RendererVk : public IRenderer {
public:
    int start(void* window, uint32_t screenWidth, uint32_t screenHeight) {
        HINSTANCE hInstance = GetModuleHandle(nullptr);
        auto hwnd = static_cast<HWND>(window);
        float screenAR = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);


        ////////////////////////////////////////////////////////////////////////////////////////////
        // LAYERS & EXTENSIONS
        const std::vector<const char*> requiredLayers = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        const std::vector<const char*> extensions = {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };

        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
        printf("Available layers:\n");
        for (const auto& layerProperties : layers) {
            printf("  %s\n", layerProperties.layerName);
        }
        printf("Required layers:\n");
        for (const char* layerName : requiredLayers) {
            bool layerFound = false;
            for (const auto& layerProperties : layers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    printf("  %s : found\n", layerName);
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) {
                    printf("  %s : not found\n", layerName);
                return 0;
            }
        }

        uint32_t instanceExtensionsCount;
        std::vector<VkExtensionProperties> instanceExtensionProperties;
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, nullptr);
        instanceExtensionProperties.resize(instanceExtensionsCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, instanceExtensionProperties.data());
        printf("\nInstance extensions: %d\n", instanceExtensionsCount);
        for (int i = 0; i < instanceExtensionsCount; i++) {
            VkExtensionProperties& props = instanceExtensionProperties[i];
            printf("     [%2d]: %3d: %s\n", i, props.specVersion, props.extensionName);
        }


        ////////////////////////////////////////////////////////////////////////////////////////////
        // INSTANCE
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
            .pApplicationName   = "01-Hello-Vk",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = "No Engine",
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_0,
        };
        VkInstanceCreateInfo createInfo = {
            .sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext                    = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo,
            .pApplicationInfo         = &appInfo,
            .enabledLayerCount        = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames      = requiredLayers.data(),
            .enabledExtensionCount    = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames  = extensions.data(),
        };
        GUARDV(vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS);


        ////////////////////////////////////////////////////////////////////////////////////////////
        // SURFACE
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
            .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext     = nullptr,
            .hinstance = hInstance,
            .hwnd      = hwnd,
        };
        GUARDV(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface));


        ////////////////////////////////////////////////////////////////////////////////////////////
        // PHYSICAL DEVICES
        uint32_t physicalDeviceCount = 0;
        std::vector<VkPhysicalDevice> physicalDevices;
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        GUARD(physicalDeviceCount > 0);
        physicalDevices.resize(physicalDeviceCount);
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

        printf("\nPhysical devices:\n");
        for (int i = 0; i < physicalDevices.size(); i++) {
            VkPhysicalDevice& pd = physicalDevices[i];
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(pd, &props);
            
            const char* deviceTypeStr = "";
            switch(props.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: deviceTypeStr = "integrated gpu"; break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU  : deviceTypeStr = "discrete gpu";   break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU   : deviceTypeStr = "virtual gpu";    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU           : deviceTypeStr = "cpu";            break;
                default: break;
            }

            printf("[%d]:\n", i);
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

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(pd, &features);
            printf("  Features:\n");
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


            VkPhysicalDeviceMemoryProperties memProps;
            vkGetPhysicalDeviceMemoryProperties(pd, &memProps);
            printf("  Memory types:\n");
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
            printf("  Memory heaps:\n");
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

            uint32_t queueCount = 0;
            std::vector<VkQueueFamilyProperties> queueProps;
            vkGetPhysicalDeviceQueueFamilyProperties(pd, &queueCount, nullptr);
            GUARD(queueCount > 0);
            queueProps.resize(queueCount);
            vkGetPhysicalDeviceQueueFamilyProperties(pd, &queueCount, queueProps.data());
            printf("  Queues:\n");
            printf("                |  Flags                                                                                                      |\n");
            printf("    idx | Count |  Graphics |   Compute |  Transfer |    Sparse | Protected | VidDecKhr | VidEncKhr |   OptFlow | DataGraph | timestamp | min image trans |\n");
            printf("    -------------------------------------------------------------------------------------------------------------------------------------------------------\n");
            for (int i = 0; i < memProps.memoryHeapCount; i++) {
                VkQueueFamilyProperties& props = queueProps[i];
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

            // uint32_t extensionCount = 0;
            // uint32_t formatCount = 0;
            // uint32_t presentModeCount = 0;
            // std::vector<VkExtensionProperties>   availableExtensions;
            // VkSurfaceCapabilitiesKHR             capabilities;
            // std::vector<VkSurfaceFormatKHR>      formats;
            // std::vector<VkPresentModeKHR>        presentModes;

            // vkGetPhysicalDeviceSurfaceFormatsKHR     (pd, surface, &formatCount,      nullptr);
            // vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &presentModeCount, nullptr);
            // vkEnumerateDeviceExtensionProperties     (pd, nullptr,   &extensionCount,   nullptr);
            // availableExtensions.resize(extensionCount);
            // formats            .resize(formatCount);
            // presentModes       .resize(presentModeCount);
            // vkGetPhysicalDeviceSurfaceFormatsKHR     (pd, surface, &formatCount,      formats.data());
            // vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &presentModeCount, presentModes.data());
            // vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &capabilities);
            // vkEnumerateDeviceExtensionProperties     (pd, nullptr, &extensionCount, availableExtensions.data());


            // graphicsFamilyIdx = -1;
            // presentFamilyIdx = -1;
            // std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
            // for (const auto& extension : availableExtensions) {
            //     requiredExtensions.erase(extension.extensionName);
            // }
            // for (int i = 0; i < queueFamilies.size(); i++) {
            //     if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            //         graphicsFamilyIdx = i;
            //         break;
            //     }
            // }
            // for (int i = 0; i < queueFamilies.size(); i++) {
            //     VkBool32 presentSupport = false;
            //     vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            //     if (presentSupport) {
            //         presentFamilyIdx = i;
            //         break;
            //     }
            // }
            // if (graphicsFamilyIdx == -1 || presentFamilyIdx == -1 || !requiredExtensions.empty() || formats.empty() || presentModes.empty()) {
            //     continue;
            // }

        }





        // // createDebugMessengerFn  = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        // // destroyDebugMessengerFn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        // // if (createDebugMessengerFn == nullptr || destroyDebugMessengerFn == nullptr) {
        // //     printf("Debug messenger extensions not found");
        // //     return 0;
        // // }
        // // if (createDebugMessengerFn(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        // //     printf("failed to set up debug messenger");
        // //     return 0;
        // // }


        // Create logical device
        // std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        // std::set<uint32_t> uniqueQueueFamilies = { graphicsFamilyIdx, presentFamilyIdx };
        // float queuePriority = 1.0f;
        // for (uint32_t queueFamily : uniqueQueueFamilies) {
        //     VkDeviceQueueCreateInfo queueCreateInfo = {
        //         .sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        //         .queueFamilyIndex    = queueFamily,
        //         .queueCount          = 1,
        //         .pQueuePriorities    = &queuePriority,
        //     };
        //     queueCreateInfos.push_back(queueCreateInfo);
        // }
        // VkPhysicalDeviceFeatures deviceFeatures{};
        // VkDeviceCreateInfo deviceCreateInfo = {
        //     .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        //     .queueCreateInfoCount       = static_cast<uint32_t>(queueCreateInfos.size()),
        //     .pQueueCreateInfos          = queueCreateInfos.data(),
        //     .enabledLayerCount          = static_cast<uint32_t>(requiredLayers.size()),
        //     .ppEnabledLayerNames        = requiredLayers.data(),
        //     .enabledExtensionCount      = static_cast<uint32_t>(deviceExtensions.size()),
        //     .ppEnabledExtensionNames    = deviceExtensions.data(),
        //     .pEnabledFeatures           = &deviceFeatures,
        // };
        // GUARDV(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
        // vkGetDeviceQueue(device, graphicsFamilyIdx, 0, &graphicsQueue);
        // vkGetDeviceQueue(device, presentFamilyIdx, 0, &presentQueue);

        // // Create swapchain
        // VkSurfaceFormatKHR surfaceFormat = formats[0];
        // for (const auto& x : formats) {
        //     if (x.format == VK_FORMAT_B8G8R8A8_SRGB && x.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        //         surfaceFormat = x;
        //     }
        // }
        // VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        // for (const auto& x : presentModes) {
        //     if (x == VK_PRESENT_MODE_MAILBOX_KHR) {
        //         presentMode = x;
        //     }
        // }
        // VkExtent2D extent;
        // if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        //     extent =  capabilities.currentExtent;
        // } else {
        //     int width, height;
        //     //glfwGetFramebufferSize(window, &width, &height);
        //     VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        //     actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        //     actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        //     extent = actualExtent;
        // }
        // uint32_t imageCount = capabilities.minImageCount + 1;
        // if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        //     imageCount = capabilities.maxImageCount;
        // }
        // VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        //     .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        //     .surface          = surface,
        //     .minImageCount    = imageCount,
        //     .imageFormat      = surfaceFormat.format,
        //     .imageColorSpace  = surfaceFormat.colorSpace,
        //     .imageExtent      = extent,
        //     .imageArrayLayers = 1,
        //     .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        // };
        // uint32_t queueFamilyIndicesValues[] = { graphicsFamilyIdx, presentFamilyIdx };
        // if (graphicsFamilyIdx != presentFamilyIdx) {
        //     swapchainCreateInfo.imageSharingMode        = VK_SHARING_MODE_CONCURRENT;
        //     swapchainCreateInfo.queueFamilyIndexCount   = 2;
        //     swapchainCreateInfo.pQueueFamilyIndices     = queueFamilyIndicesValues;
        // } else {
        //     swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        // }
        // swapchainCreateInfo.preTransform    = capabilities.currentTransform;
        // swapchainCreateInfo.compositeAlpha  = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        // swapchainCreateInfo.presentMode     = presentMode;
        // swapchainCreateInfo.clipped         = VK_TRUE;
        // swapchainCreateInfo.oldSwapchain    = VK_NULL_HANDLE;
        // GUARDV(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapChain));
        // vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        // swapChainImages.resize(imageCount);
        // vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
        // swapChainImageFormat  = surfaceFormat.format;
        // swapChainExtent       = extent;

        // // Create image views
        // swapChainImageViews.resize(swapChainImages.size());
        // for (size_t i = 0; i < swapChainImages.size(); i++) {
        //      VkComponentMapping components = {
        //         .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        //         .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        //         .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        //         .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        //     };
        //      VkImageSubresourceRange subresourceRange = {
        //         .aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT,
        //         .baseMipLevel    = 0,
        //         .levelCount      = 1,
        //         .baseArrayLayer  = 0,
        //         .layerCount      = 1,
        //     };
        //     VkImageViewCreateInfo createInfo = {
        //         .sType                            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        //         .pNext                            = nullptr,
        //         .image                            = swapChainImages[i],
        //         .viewType                         = VK_IMAGE_VIEW_TYPE_2D,
        //         .format                           = swapChainImageFormat,
        //         .components                       = components,
        //         .subresourceRange                 = subresourceRange,
        //     };
        //     if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
        //         printf("failed to create image views");
        //         return 0;
        //     }
        // }

        // // Create command pool
        // VkCommandPoolCreateInfo poolInfo = {
        //     .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        //     .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        //     .queueFamilyIndex = graphicsFamilyIdx,
        // };
        // if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        //     printf("failed to create command pool");
        //     return 0;
        // }

        // // Create command buffer
        // VkCommandBufferAllocateInfo allocInfo = {
        //     .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        //     .commandPool        = commandPool,
        //     .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        //     .commandBufferCount = 1,
        // };
        // GUARDV(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

        // // Create sync objects
        // VkSemaphoreCreateInfo semaphoreInfo = {
        //     .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        // };
        // VkFenceCreateInfo fenceInfo = {
        //     .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        //     .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        // };
        // if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        //     vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
        //     vkCreateFence    (device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
        //     printf("failed to create synchronization objects for a frame");
        //     return 0;
        // }
        return 1;
    }

    void stop() {
        vkDeviceWaitIdle         (device);
        vkDestroySemaphore       (device, renderFinishedSemaphore, nullptr);
        vkDestroySemaphore       (device, imageAvailableSemaphore, nullptr);
        vkDestroyFence           (device, inFlightFence, nullptr);
        vkDestroyCommandPool     (device, commandPool, nullptr);
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer (device, framebuffer, nullptr);
        }
        vkDestroyPipeline        (device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout  (device, pipelineLayout, nullptr);
        vkDestroyRenderPass      (device, renderPass, nullptr);
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView   (device, imageView, nullptr);
        }
        vkDestroySwapchainKHR    (device, swapChain, nullptr);
        vkDestroyDevice          (device, nullptr);
        destroyDebugMessengerFn(instance, debugMessenger, nullptr);
        vkDestroySurfaceKHR      (instance, surface, nullptr);
        vkDestroyInstance        (instance, nullptr);
    }

    void setView(ViewDesc& desc) {
    }

    void setProjection(ProjectionDesc& desc) {
    }

    int render(const Color& clearColor, const std::vector<RenderItem>& items) {
        return 1;
    }
    
    void setFillMode(FillMode mode) {
    }
    
    int addBuffer(const BufferDesc& desc) {
        return -1;
    }
    
    int addMesh(const MeshDesc& desc) {
        return -1;
    }

private:
    PFN_vkCreateDebugUtilsMessengerEXT  createDebugMessengerFn;
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessengerFn;
    VkInstance                          instance;
    VkDebugUtilsMessengerEXT            debugMessenger;
    VkSurfaceKHR                        surface;
    VkPhysicalDevice                    physicalDevice = VK_NULL_HANDLE;
    VkDevice                            device;
    uint32_t                            graphicsFamilyIdx;
    uint32_t                            presentFamilyIdx;
    VkQueue                             graphicsQueue;
    VkQueue                             presentQueue;
    VkSwapchainKHR                      swapChain;
    std::vector<VkImage>                swapChainImages;
    VkFormat                            swapChainImageFormat;
    VkExtent2D                          swapChainExtent;
    std::vector<VkImageView>            swapChainImageViews;
    std::vector<VkFramebuffer>          swapChainFramebuffers;
    VkRenderPass                        renderPass;
    VkPipelineLayout                    pipelineLayout;
    VkPipeline                          graphicsPipeline;
    VkCommandPool                       commandPool;
    VkCommandBuffer                     commandBuffer;
    VkSemaphore                         imageAvailableSemaphore;
    VkSemaphore                         renderFinishedSemaphore;
    VkFence                             inFlightFence;
};

std::unique_ptr<IRenderer> createRenderer() {
    return std::make_unique<RendererVk>();
}
}
