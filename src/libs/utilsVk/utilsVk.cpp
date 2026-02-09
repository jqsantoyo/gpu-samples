#include "utilsVk.h"
#include <utils/utils.h>
#include <cstdio>
#include <vector>
#include <inttypes.h>
#include <set>
#include <string>

#define GUARDV(x) if ((x != VK_SUCCESS)) {  printf("Error: "#x); return 0; }
#define GUARD(x)  if (!(x))              {  printf("Error: "#x); return 0; }

namespace gpu {

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
    printf("Validation layer: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

bool createInstance(
    const char* name,
    uint32_t version,
    const std::vector<const char*>& layers,
    const std::vector<const char*>& extensions,
    VkInstance& instance
) {
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
        .pApplicationInfo         = &appInfo,
        .enabledLayerCount        = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames      = layers.data(),
        .enabledExtensionCount    = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames  = extensions.data(),
    };
    GUARDV(vkCreateInstance(&createInfo, nullptr, &instance));
    return true;
}

bool createSurface(VkInstance instance, void* window, VkSurfaceKHR& surface) {
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



bool selectPhysicalDevice(
    VkInstance instance,
    VkSurfaceKHR surface,
    const std::vector<const char*>& extensions,
    PhysicalDeviceCtx& ctx
) {
    std::vector<int> physicalDeviceScores;
    std::vector<VkPhysicalDevice> physicalDevices;
    enumeratePhysicalDevices(instance, physicalDevices);

    int selectedPhysicalDeviceIdx = -1;
    for (int i = 0; i < physicalDevices.size(); i++) {
        VkPhysicalDevice& pd = physicalDevices[i];

        
        VkPhysicalDeviceProperties           props;
        VkPhysicalDeviceFeatures             features;
        VkPhysicalDeviceMemoryProperties     memProps;
        VkSurfaceCapabilitiesKHR             capabilities;
        std::vector<VkQueueFamilyProperties> queueFamilies;
        std::vector<VkSurfaceFormatKHR>      formats;
        std::vector<VkPresentModeKHR>        presentModes;
        std::vector<VkExtensionProperties>   devExtensions;

        enumerateDeviceExtensionProperties       (pd, nullptr, devExtensions);
        vkGetPhysicalDeviceProperties            (pd, &props);
        vkGetPhysicalDeviceFeatures              (pd, &features);
        vkGetPhysicalDeviceMemoryProperties      (pd, &memProps);
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &capabilities);
        getPhysicalDeviceQueueFamilyProperties   (pd, queueFamilies);
        getPhysicalDeviceSurfaceFormatsKHR       (pd, surface, formats);
        getPhysicalDeviceSurfacePresentModesKHR  (pd, surface, presentModes);

        int graphicsFamilyIdx = -1;
        int presentFamilyIdx = -1;
        std::set<const char*> requiredExtensions(extensions.begin(), extensions.end());
        for (const auto& devExtension : devExtensions) {
            requiredExtensions.erase(devExtension.extensionName);
        }
        for (int i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamilyIdx = i;
                break;
            }
        }
        for (int i = 0; i < queueFamilies.size(); i++) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &presentSupport);
            if (presentSupport) {
                presentFamilyIdx = i;
                break;
            }
        }
        if (selectedPhysicalDeviceIdx == -1 &&
            graphicsFamilyIdx != -1 &&
            presentFamilyIdx != -1
            // requiredExtensions.empty()
        ) {
            selectedPhysicalDeviceIdx = i;
            ctx.gIdx = graphicsFamilyIdx;
            ctx.pIdx = presentFamilyIdx;
            ctx.format = formats[0];
            ctx.presentMode = VK_PRESENT_MODE_FIFO_KHR;
            for (const auto& x : formats) {
                if (x.format == VK_FORMAT_B8G8R8A8_SRGB && x.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    ctx.format = x;
                }
            }
            for (const auto& x : presentModes) {
                if (x == VK_PRESENT_MODE_MAILBOX_KHR) {
                    ctx.presentMode = x;
                }
            }
            ctx.currTransform = capabilities.currentTransform;
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                ctx.extent =  capabilities.currentExtent;
            } else {
                int width, height;
                //glfwGetFramebufferSize(window, &width, &height);
                VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
                actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
                ctx.extent = actualExtent;
            }
            ctx.imageCount = capabilities.minImageCount + 1;
            if (capabilities.maxImageCount > 0 && ctx.imageCount > capabilities.maxImageCount) {
                ctx.imageCount = capabilities.maxImageCount;
            }
        }
    }
    if (selectedPhysicalDeviceIdx != -1) {
        ctx.physicalDevice = physicalDevices[selectedPhysicalDeviceIdx];
        return true;
    } else {
        ctx.physicalDevice = VK_NULL_HANDLE;
        return false;
    }
}

bool selectComputePhysicalDevice(VkInstance instance, const std::vector<const char*>& extensions, ComputePhysicalDeviceWrap& w) {
    std::vector<VkPhysicalDevice> physicalDevices;
    enumeratePhysicalDevices(instance, physicalDevices);

    for (int i = 0; i < physicalDevices.size(); i++) {
        VkPhysicalDevice& pd = physicalDevices[i];

        VkPhysicalDeviceProperties           props;
        VkPhysicalDeviceFeatures             features;
        VkPhysicalDeviceMemoryProperties     memProps;
        std::vector<VkQueueFamilyProperties> queueFamilies;
        std::vector<VkExtensionProperties>   availableExtensions;

        enumerateDeviceExtensionProperties       (pd, nullptr, availableExtensions);
        vkGetPhysicalDeviceProperties            (pd, &props);
        vkGetPhysicalDeviceFeatures              (pd, &features);
        vkGetPhysicalDeviceMemoryProperties      (pd, &memProps);
        getPhysicalDeviceQueueFamilyProperties   (pd, queueFamilies);

        int computeFamilyIdx = -1;
        std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        for (int i = 0; i < queueFamilies.size(); i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                computeFamilyIdx = i;
                break;
            }
        }
        if (requiredExtensions.empty() && computeFamilyIdx != -1) {
            w.physicalDevice = physicalDevices[i];
            w.cIdx = computeFamilyIdx;
            return true;
        }
    }
    return false;
}

bool createDevice(PhysicalDeviceCtx pdCtx, VkDevice& device, VkQueue& gQ, VkQueue& pQ) {
    const std::vector<const char*> requiredLayers = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // PFN_vkCreateDebugUtilsMessengerEXT  createDebugMessengerFn;
    // PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessengerFn;
    // createDebugMessengerFn  = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // destroyDebugMessengerFn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    // if (createDebugMessengerFn == nullptr || destroyDebugMessengerFn == nullptr) {
    //     printf("Debug messenger extensions not found");
    //     return 0;
    // }
    // if (createDebugMessengerFn(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
    //     printf("failed to set up debug messenger");
    //     return 0;
    // }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { pdCtx.gIdx, pdCtx.pIdx };
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex    = queueFamily,
            .queueCount          = 1,
            .pQueuePriorities    = &queuePriority,
        };
        queueCreateInfos.push_back(queueCreateInfo);
    }
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount       = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos          = queueCreateInfos.data(),
        .enabledLayerCount          = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames        = requiredLayers.data(),
        .enabledExtensionCount      = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames    = deviceExtensions.data(),
        .pEnabledFeatures           = &deviceFeatures,
    };
    GUARDV(vkCreateDevice(pdCtx.physicalDevice, &deviceCreateInfo, nullptr, &device));
    vkGetDeviceQueue(device, pdCtx.gIdx, 0, &gQ);
    vkGetDeviceQueue(device, pdCtx.pIdx, 0, &pQ);
    return true;
}

bool createComputeDevice(VkPhysicalDevice pd, uint32_t cIdx, VkDevice& device, VkQueue& cQ) {
    const std::vector<const char*> requiredLayers = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // PFN_vkCreateDebugUtilsMessengerEXT  createDebugMessengerFn;
    // PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessengerFn;
    // createDebugMessengerFn  = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // destroyDebugMessengerFn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    // if (createDebugMessengerFn == nullptr || destroyDebugMessengerFn == nullptr) {
    //     printf("Debug messenger extensions not found");
    //     return 0;
    // }
    // if (createDebugMessengerFn(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
    //     printf("failed to set up debug messenger");
    //     return 0;
    // }

    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = {
        {
        .sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex    = cIdx,
        .queueCount          = 1,
        .pQueuePriorities    = &queuePriority,
        },
    };
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount       = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos          = queueCreateInfos.data(),
        .enabledLayerCount          = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames        = requiredLayers.data(),
        .enabledExtensionCount      = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames    = deviceExtensions.data(),
        .pEnabledFeatures           = &deviceFeatures,
    };
    GUARDV(vkCreateDevice(pd, &deviceCreateInfo, nullptr, &device));
    vkGetDeviceQueue(device, cIdx, 0, &cQ);
    return true;
}

bool createSwapchain(VkDevice device, VkSurfaceKHR surface, const PhysicalDeviceCtx& pdCtx, SwapchainCtx& ctx) {
    bool uniqueQueue = pdCtx.gIdx == pdCtx.pIdx;
    uint32_t queueIndices[] = { pdCtx.gIdx, pdCtx.pIdx };
    ctx.imageFormat = pdCtx.format.format;
    ctx.extent = pdCtx.extent;
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface                = surface,
        .minImageCount          = pdCtx.imageCount,
        .imageFormat            = pdCtx.format.format,
        .imageColorSpace        = pdCtx.format.colorSpace,
        .imageExtent            = pdCtx.extent,
        .imageArrayLayers       = 1,
        .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode       = uniqueQueue ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount  = uniqueQueue ? 1u : 2u,
        .pQueueFamilyIndices    = queueIndices,
        .preTransform           = pdCtx.currTransform,
        .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode            = pdCtx.presentMode,
        .clipped                = VK_TRUE,
        .oldSwapchain           = VK_NULL_HANDLE,
    };
    GUARDV(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &ctx.swapchain));
    GUARD(getSwapchainImagesKHR(device, ctx.swapchain, ctx.images));
    ctx.imageViews.resize(ctx.images.size());
    for (size_t i = 0; i < ctx.images.size(); i++) {
        VkImageViewCreateInfo createInfo = {
            .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext              = nullptr,
            .image              = ctx.images[i],
            .viewType           = VK_IMAGE_VIEW_TYPE_2D,
            .format             = pdCtx.format.format,
            .components         = {},
            .subresourceRange   = {
                .aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel    = 0,
                .levelCount      = 1,
                .baseArrayLayer  = 0,
                .layerCount      = 1,
            },
        };
        GUARDV(vkCreateImageView(device, &createInfo, nullptr, &ctx.imageViews[i]));
    }
    return true;
}

bool createCommandObjects() {
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
    return true;
}


bool loadShader(VkDevice device, const char* dir, const char* name, Shader& shader) {
    std::string path = getAssetsPath() + dir + "\\" + name;
    FILE* file;
    errno_t err = fopen_s(&file, path.c_str(), "rb");
    if (err != 0 || file == nullptr) {
        return false;
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    shader.data.resize(size);
    fread(shader.data.data(), size, 1, file);

    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = shader.data.size(),
        .pCode = reinterpret_cast<uint32_t*>(shader.data.data()),
    };
    GUARDV(vkCreateShaderModule(device, &info, nullptr, &shader.module));
    return true;
}

}
