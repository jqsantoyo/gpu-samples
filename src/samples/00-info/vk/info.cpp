#include <renderer/renderer.h>
#include <app/app.h>
#include <utils/utils.h>
#include <utilsVk/utilsVk.h>
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

namespace gpu {

int info() {
    VkInstance          instance        = VK_NULL_HANDLE;
    VkPhysicalDevice    physicalDevice  = VK_NULL_HANDLE;
    VkDevice            device          = VK_NULL_HANDLE;

    uint32_t version = 0;
    vkEnumerateInstanceVersion(&version);
    printf("Loader supports Vulkan %u.%u.%u\n", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));

    VkApplicationInfo appInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "01-Info-Vk",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_0,
    };

#ifdef WIN32
    VkInstanceCreateFlags createFlags = = 0;
    std::vector<const char*> extensions = {  };
#else
    VkInstanceCreateFlags createFlags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    std::vector<const char*> extensions = { VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME };
#endif

    VkInstanceCreateInfo createInfo = {
        .sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                    = nullptr,
        .flags                    = createFlags,
        .pApplicationInfo         = &appInfo,
        .enabledLayerCount        = 0,
        .ppEnabledLayerNames      = nullptr,
        .enabledExtensionCount    = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames  = extensions.data(),
    };
    
    GUARDV(vkCreateInstance(&createInfo, nullptr, &instance));

    std::vector<VkPhysicalDevice> physicalDevices;
    enumeratePhysicalDevices(instance, physicalDevices);
    printf("\nPhysical devices:\n");
    for (int i = 0; i < physicalDevices.size(); i++) {
        VkPhysicalDevice& pd = physicalDevices[i];
        
        VkPhysicalDeviceProperties           props;
        VkPhysicalDeviceFeatures             features;
        VkPhysicalDeviceMemoryProperties     memProps;
        std::vector<VkQueueFamilyProperties> queueFamilies;
        std::vector<VkExtensionProperties>   extensions;

        vkGetPhysicalDeviceProperties            (pd, &props);
        vkGetPhysicalDeviceFeatures              (pd, &features);
        vkGetPhysicalDeviceMemoryProperties      (pd, &memProps);
        getPhysicalDeviceQueueFamilyProperties   (pd, queueFamilies);
        enumerateDeviceExtensionProperties       (pd, nullptr, extensions);

        const char* deviceTypeStr = "";
        switch(props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: deviceTypeStr = "integrated gpu"; break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU  : deviceTypeStr = "discrete gpu";   break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU   : deviceTypeStr = "virtual gpu";    break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU           : deviceTypeStr = "cpu";            break;
            default                                    : deviceTypeStr = "unknown";        break;
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

        printf("  \nExtensions: %zu\n", extensions.size());
        for(int i = 0; i < extensions.size(); i++) {
            VkExtensionProperties& ext = extensions[i];
            printf("    %6d :: %s\n", ext.specVersion, ext.extensionName);
        }
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount       = 0,
        .pQueueCreateInfos          = nullptr,
        .enabledLayerCount          = 0,
        .ppEnabledLayerNames        = nullptr,
        .enabledExtensionCount      = 0,
        .ppEnabledExtensionNames    = nullptr,
        .pEnabledFeatures           = &deviceFeatures,
    };
    GUARDV(vkCreateDevice(physicalDevices[0], &deviceCreateInfo, nullptr, &device));
    printf("Device created\n");
    
    vkDeviceWaitIdle         (device);
    vkDestroyDevice          (device, nullptr);
    vkDestroyInstance        (instance, nullptr);
    return 1;
}

}
