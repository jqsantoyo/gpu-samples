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

bool createInstance(
    const char* name,
    uint32_t version,
    const std::vector<const char*>& layers,
    const std::vector<const char*>& extensions,
    InstanceCtx& ctx
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
    GUARDV(vkCreateInstance(&createInfo, nullptr, &ctx.instance));
    
    PFN_vkCreateDebugUtilsMessengerEXT  createDebugMessengerFn;
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessengerFn;
    createDebugMessengerFn  = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(ctx.instance, "vkCreateDebugUtilsMessengerEXT");
    destroyDebugMessengerFn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx.instance, "vkDestroyDebugUtilsMessengerEXT");
    GUARD(createDebugMessengerFn != nullptr);
    GUARD(destroyDebugMessengerFn != nullptr);
    GUARDV(createDebugMessengerFn(ctx.instance, &debugCreateInfo, nullptr, &ctx.debugMessenger));
    return true;
}

void destroyInstance(InstanceCtx& ctx) {
    ctx.destroyDebugMessengerFn(ctx.instance, ctx.debugMessenger, nullptr);
    vkDestroyInstance(ctx.instance, nullptr);
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

bool getSurfaceInfo(VkPhysicalDevice pd, VkSurfaceKHR surface, SurfaceInfo& surfaceInfo) {
    GUARDV(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &surfaceInfo.capabilities));
    GUARD(getPhysicalDeviceSurfaceFormatsKHR        (pd, surface, surfaceInfo.formats));
    GUARD(getPhysicalDeviceSurfacePresentModesKHR   (pd, surface, surfaceInfo.presentModes));
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
        std::vector<VkExtensionProperties>   devExtensions;
        SurfaceInfo surfaceInfo;
        enumerateDeviceExtensionProperties       (pd, nullptr, devExtensions);
        vkGetPhysicalDeviceProperties            (pd, &props);
        vkGetPhysicalDeviceFeatures              (pd, &features);
        vkGetPhysicalDeviceMemoryProperties      (pd, &memProps);
        getPhysicalDeviceQueueFamilyProperties   (pd, queueFamilies);
        getSurfaceInfo                           (pd, surface, surfaceInfo);

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
            presentFamilyIdx != -1 && 
            // requiredExtensions.empty() &&
            surfaceInfo.formats.size() > 0 &&
            surfaceInfo.presentModes.size() > 0
        ) {
            selectedPhysicalDeviceIdx = i;
            ctx.gIdx = graphicsFamilyIdx;
            ctx.pIdx = presentFamilyIdx;
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

    float queuePriority = 1.0f;    
    std::vector<VkDeviceQueueCreateInfo> queueInfos = {
        { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, pdCtx.gIdx, 1, &queuePriority },
        { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, pdCtx.pIdx, 1, &queuePriority },
    };
    if (pdCtx.gIdx == pdCtx.pIdx) {
        queueInfos.resize(1);
    }
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount       = static_cast<uint32_t>(queueInfos.size()),
        .pQueueCreateInfos          = queueInfos.data(),
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
    destroySwapchain(device, ctx);

    SurfaceInfo surfaceInfo;
    getSurfaceInfo(pdCtx.physicalDevice, surface, surfaceInfo);
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkFormat format = surfaceInfo.formats[0].format;
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& x : surfaceInfo.formats) {
        if (x.format == VK_FORMAT_B8G8R8A8_SRGB && x.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            ctx.format = x.format;
        }
    }
    for (const auto& x : surfaceInfo.presentModes) {
        if (x == VK_PRESENT_MODE_MAILBOX_KHR) {
            ctx.presentMode = x;
        }
    }
    VkSurfaceTransformFlagBitsKHR currTransform = surfaceInfo.capabilities.currentTransform;
    if (surfaceInfo.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        ctx.extent =  surfaceInfo.capabilities.currentExtent;
    } else {
        // VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        // actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        // actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        // ctx.extent = actualExtent;
    }
    uint32_t imageCount = surfaceInfo.capabilities.minImageCount + 1;
    if (surfaceInfo.capabilities.maxImageCount > 0 && imageCount > surfaceInfo.capabilities.maxImageCount) {
        imageCount = surfaceInfo.capabilities.maxImageCount;
    }
    
    bool uniqueQueue = pdCtx.gIdx == pdCtx.pIdx;
    uint32_t queueIndices[] = { pdCtx.gIdx, pdCtx.pIdx };
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface                = surface,
        .minImageCount          = imageCount,
        .imageFormat            = format,
        .imageColorSpace        = colorSpace,
        .imageExtent            = ctx.extent,
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
    GUARDV(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &ctx.swapchain));
    GUARD(getSwapchainImagesKHR(device, ctx.swapchain, ctx.images));
    ctx.imageViews.resize(ctx.images.size());
    for (size_t i = 0; i < ctx.images.size(); i++) {
        VkImageViewCreateInfo createInfo = {
            .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext              = nullptr,
            .image              = ctx.images[i],
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
        GUARDV(vkCreateImageView(device, &createInfo, nullptr, &ctx.imageViews[i]));
    }
    return true;
}

bool createSwapchainFramebuffers(VkDevice device, SwapchainCtx& ctx, VkRenderPass renderPass) {
    ctx.framebuffers.resize(ctx.imageViews.size());
    for (size_t i = 0; i < ctx.imageViews.size(); i++) {
        VkImageView attachments[] = {
            ctx.imageViews[i],
        };
        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = ctx.extent.width,
            .height = ctx.extent.height,
            .layers = 1,
        };
        GUARDV(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &ctx.framebuffers[i]));
    }
    return true;
}

bool destroySwapchain(VkDevice device, SwapchainCtx& ctx) {
    for (auto framebuffer : ctx.framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for (auto imageView : ctx.imageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    if (ctx.swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, ctx.swapchain, nullptr);
    }
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
    renderReady.resize(frameCount);
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
        GUARDV(createSemaphore(device, renderReady[i]));
        GUARDV(createFence(device, execution[i], true));


    }
    return true;
}

void FrameControl::deinit() {
    for (int i = 0; i < frameCount; i++) {
        vkDestroySemaphore       (device, imageReady[i], nullptr);
        vkDestroySemaphore       (device, renderReady[i], nullptr);
        vkDestroyFence           (device, execution[i], nullptr);
        vkDestroyCommandPool     (device, cmdPool[i], nullptr);
    }
}

Frame FrameControl::next() {
    frameIdx = (frameIdx + 1) % frameCount;
    vkWaitForFences(device, 1, &execution[frameIdx], VK_TRUE, UINT64_MAX);
    printf("Active frame: %d\n", frameIdx);
    return { cmdBuffer[frameIdx], imageReady[frameIdx], renderReady[frameIdx] };
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

bool FrameControl::end() {
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &imageReady[frameIdx],
        .pWaitDstStageMask    = waitStages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &cmdBuffer[frameIdx],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &renderReady[frameIdx],
    };
    GUARDV(vkQueueSubmit(queue, 1, &submitInfo, execution[frameIdx]));
    return true;
}














////////////////////////////////////////////////////////////////////////////////////////////////////
// SHADER

Shader::~Shader() {
    vkDestroyShaderModule(device, module, nullptr);
}

bool Shader::load(VkDevice device, const char* dir, const char* name, VkShaderStageFlagBits stage) {
    this->device = device;
    std::string path = getAssetsPath() + dir + "\\" + name;
    FILE* file;
    errno_t err = fopen_s(&file, path.c_str(), "rb");
    if (err != 0 || file == nullptr) {
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

}
