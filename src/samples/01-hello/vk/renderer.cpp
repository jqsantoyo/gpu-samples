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

#define GUARD(x) if (!x) {  printf("Error: "#x); return 0; }
namespace gpu {



template <typename T>
const T& clamp(const T& value, const T& low, const T& high) {
    return value < low ? low : (value > high ? high : value);
}


std::vector<char> readFile(const std::wstring& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        printf("Failed to open file");
        return {};
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

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
        // DEVICE
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

        VkApplicationInfo appInfo = {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName   = "01-Hello-Vk",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = "No Engine",
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_0,
        };
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
        VkInstanceCreateInfo createInfo{
            .sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext                    = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo,
            .pApplicationInfo         = &appInfo,
            .enabledLayerCount        = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames      = requiredLayers.data(),
            .enabledExtensionCount    = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames  = extensions.data(),
        };
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            printf("failed to create instance");
            return 0;
        }

        createDebugMessengerFn  = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        destroyDebugMessengerFn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (createDebugMessengerFn == nullptr || destroyDebugMessengerFn == nullptr) {
            printf("Debug messenger extensions not found");
            return 0;
        }
        if (createDebugMessengerFn(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            printf("failed to set up debug messenger");
            return 0;
        }

        // Create surface
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
            .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = hInstance,
            .hwnd      = hwnd,
        };
        VkResult res = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
        if (res != VK_SUCCESS) {
            printf("Error creating surface");
            return 0;
        }

        // Pick physical device
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            printf("failed to find GPUs with Vulkan support");
            return 0;
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        std::vector<VkQueueFamilyProperties> queueFamilies;
        std::vector<VkExtensionProperties>   availableExtensions;
        VkSurfaceCapabilitiesKHR             capabilities;
        std::vector<VkSurfaceFormatKHR>      formats;
        std::vector<VkPresentModeKHR>        presentModes;
        for (const auto& device : devices) {
            uint32_t extensionCount = 0;
            uint32_t formatCount = 0;
            uint32_t presentModeCount = 0;
            uint32_t queueFamilyCount = 0;
            graphicsFamilyIdx = -1;
            presentFamilyIdx = -1;

            vkGetPhysicalDeviceQueueFamilyProperties (device, &queueFamilyCount, nullptr);
            vkGetPhysicalDeviceSurfaceFormatsKHR     (device, surface, &formatCount,      nullptr);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
            vkEnumerateDeviceExtensionProperties     (device, nullptr,   &extensionCount,   nullptr);
            if (extensionCount == 0 || formatCount == 0 || presentModeCount == 0 || queueFamilyCount == 0) {
                continue;
            }
            queueFamilies       .resize(queueFamilyCount);
            availableExtensions .resize(extensionCount);
            formats             .resize(formatCount);
            presentModes        .resize(presentModeCount);
            vkGetPhysicalDeviceQueueFamilyProperties (device, &queueFamilyCount, queueFamilies.data());
            vkGetPhysicalDeviceSurfaceFormatsKHR     (device, surface, &formatCount,      formats.data());
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes.data());
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
            vkEnumerateDeviceExtensionProperties     (device, nullptr, &extensionCount, availableExtensions.data());
            std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
            for (const auto& extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
            }
            for (int i = 0; i < queueFamilies.size(); i++) {
                if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    graphicsFamilyIdx = i;
                    break;
                }
            }
            for (int i = 0; i < queueFamilies.size(); i++) {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (presentSupport) {
                    presentFamilyIdx = i;
                    break;
                }
            }
            if (graphicsFamilyIdx == -1 || presentFamilyIdx == -1 || !requiredExtensions.empty() || formats.empty() || presentModes.empty()) {
                continue;
            }
            physicalDevice = device;
            break;
        }
        if (physicalDevice == VK_NULL_HANDLE) {
            printf("failed to find a suitable GPU");
            return 0;
        }

        // Create logical device
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { graphicsFamilyIdx, presentFamilyIdx };
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex    = queueFamily;
            queueCreateInfo.queueCount          = 1;
            queueCreateInfo.pQueuePriorities    = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        VkPhysicalDeviceFeatures deviceFeatures{};
        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount       = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos          = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures           = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount      = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames    = deviceExtensions.data();
        deviceCreateInfo.enabledLayerCount          = static_cast<uint32_t>(requiredLayers.size());
        deviceCreateInfo.ppEnabledLayerNames        = requiredLayers.data();
        if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
            printf("failed to create logical device");
            return 0;
        }
        vkGetDeviceQueue(device, graphicsFamilyIdx, 0, &graphicsQueue);
        vkGetDeviceQueue(device, presentFamilyIdx, 0, &presentQueue);

        // Create swapchain
        VkSurfaceFormatKHR surfaceFormat = formats[0];
        for (const auto& x : formats) {
            if (x.format == VK_FORMAT_B8G8R8A8_SRGB && x.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surfaceFormat = x;
            }
        }
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& x : presentModes) {
            if (x == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentMode = x;
            }
        }
        VkExtent2D extent;
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            extent =  capabilities.currentExtent;
        } else {
            int width, height;
            //glfwGetFramebufferSize(window, &width, &height);
            VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            extent = actualExtent;
        }
        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }
        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface          = surface;
        swapchainCreateInfo.minImageCount    = imageCount;
        swapchainCreateInfo.imageFormat      = surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent      = extent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        uint32_t queueFamilyIndicesValues[] = { graphicsFamilyIdx, presentFamilyIdx };
        if (graphicsFamilyIdx != presentFamilyIdx) {
            swapchainCreateInfo.imageSharingMode        = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount   = 2;
            swapchainCreateInfo.pQueueFamilyIndices     = queueFamilyIndicesValues;
        } else {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        swapchainCreateInfo.preTransform    = capabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha  = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode     = presentMode;
        swapchainCreateInfo.clipped         = VK_TRUE;
        swapchainCreateInfo.oldSwapchain    = VK_NULL_HANDLE;
        if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapChain) != VK_SUCCESS) {
            printf("failed to create swap chain");
            return 0;
        }
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
        swapChainImageFormat  = surfaceFormat.format;
        swapChainExtent       = extent;

        // Create image views
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType                            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image                            = swapChainImages[i];
            createInfo.viewType                         = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format                           = swapChainImageFormat;
            createInfo.components.r                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel    = 0;
            createInfo.subresourceRange.levelCount      = 1;
            createInfo.subresourceRange.baseArrayLayer  = 0;
            createInfo.subresourceRange.layerCount      = 1;
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                printf("failed to create image views");
                return 0;
            }
        }

        // Create render pass
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format          = swapChainImageFormat;
        colorAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment   = 0;
        colorAttachmentRef.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &colorAttachmentRef;
        VkSubpassDependency dependency{};
        dependency.srcSubpass           = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass           = 0;
        dependency.srcStageMask         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask        = 0;
        dependency.dstStageMask         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount  = 1;
        renderPassInfo.pAttachments     = &colorAttachment;
        renderPassInfo.subpassCount     = 1;
        renderPassInfo.pSubpasses       = &subpass;
        renderPassInfo.dependencyCount  = 1;
        renderPassInfo.pDependencies    = &dependency;
        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            printf("failed to create render pass");
            return 0;
        }

        // Create graphics pipeline
        std::wstring assetsPath = getAssetsPathW();
        auto vertShaderCode = readFile(assetsPath + L"01-hello-shaders-vk/shader.vert.spv");
        auto fragShaderCode = readFile(assetsPath + L"01-hello-shaders-vk/shader.frag.spv");
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
        VkShaderModuleCreateInfo vertCreateInfo{};
        vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertCreateInfo.codeSize = vertShaderCode.size();
        vertCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());
        VkShaderModuleCreateInfo fragCreateInfo{};
        fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragCreateInfo.codeSize = fragShaderCode.size();
        fragCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());
        if (vkCreateShaderModule(device, &vertCreateInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
            printf("failed to create vertex shader module");
            return 0;
        }
        if (vkCreateShaderModule(device, &fragCreateInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
            printf("failed to create fragment shader module");
            return 0;
        }
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage   = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module  = vertShaderModule;
        vertShaderStageInfo.pName   = "main";
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage   = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module  = fragShaderModule;
        fragShaderStageInfo.pName   = "main";
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount   = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount  = 1;
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType                    = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable         = VK_FALSE;
        rasterizer.rasterizerDiscardEnable  = VK_FALSE;
        rasterizer.polygonMode              = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth                = 1.0f;
        rasterizer.cullMode                 = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace                = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable          = VK_FALSE;
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable   = VK_FALSE;
        multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable    = VK_FALSE;
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable     = VK_FALSE;
        colorBlending.logicOp           = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount   = 1;
        colorBlending.pAttachments      = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount  = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates     = dynamicStates.data();
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount         = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            printf("failed to create pipeline layout");
            return 0;
        }
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount          = 2;
        pipelineInfo.pStages             = shaderStages;
        pipelineInfo.pVertexInputState   = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pColorBlendState    = &colorBlending;
        pipelineInfo.pDynamicState       = &dynamicState;
        pipelineInfo.layout              = pipelineLayout;
        pipelineInfo.renderPass          = renderPass;
        pipelineInfo.subpass             = 0;
        pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            printf("failed to create graphics pipeline");
            return 0;
        }
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);

        // Create frame buffers
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;
            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                printf("failed to create framebuffer");
                return 0;
            }
        }

        // Create command pool
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = graphicsFamilyIdx;
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            printf("failed to create command pool");
            return 0;
        }

        // Create command buffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = commandPool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
            printf("failed to allocate command buffers");
            return 0;
        }

        // Create sync objects
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence    (device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
            printf("failed to create synchronization objects for a frame");
            return 0;
        }
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
        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence);
        uint32_t imageIndex;
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);

        // Record command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            printf("failed to begin recording command buffer");
            return 0;
        }
        VkClearValue vkClearColor = { {{clearColor.v[0], clearColor.v[1], clearColor.v[2], clearColor.v[3]}} };
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType                = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass           = renderPass;
        renderPassInfo.framebuffer          = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset    = { 0, 0 };
        renderPassInfo.renderArea.extent    = swapChainExtent;
        renderPassInfo.clearValueCount      = 1;
        renderPassInfo.pClearValues         = &vkClearColor;
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        VkViewport viewport{};
        viewport.x          = 0.0f;
        viewport.y          = 0.0f;
        viewport.width      = static_cast<float>(swapChainExtent.width);
        viewport.height     = static_cast<float>(swapChainExtent.height);
        viewport.minDepth   = 0.0f;
        viewport.maxDepth   = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            printf("failed to record command buffer");
            return 0;
        }
        VkSemaphore signalSemaphores[]    = { renderFinishedSemaphore };
        VkSemaphore waitSemaphores[]      = { imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = waitSemaphores;
        submitInfo.pWaitDstStageMask    = waitStages;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
            printf("failed to submit draw command buffer");
            return 0;
        }
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount  = 1;
        presentInfo.pWaitSemaphores     = signalSemaphores;
        VkSwapchainKHR swapChains[]     = { swapChain };
        presentInfo.swapchainCount      = 1;
        presentInfo.pSwapchains         = swapChains;
        presentInfo.pImageIndices       = &imageIndex;
        vkQueuePresentKHR(presentQueue, &presentInfo);
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
