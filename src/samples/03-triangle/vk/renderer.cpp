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

namespace gpu {




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



class RendererVk : public IRenderer {
public:
    int start(void* window, uint32_t screenWidth, uint32_t screenHeight) {
        float screenAR = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);


        std::vector<const char*> layers;
        std::vector<const char*> extensions;
        GUARD(validateLayers(
            {
                "VK_LAYER_KHRONOS_validation"
            },
            {
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME
            },
            {
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                VK_KHR_SURFACE_EXTENSION_NAME,
            },
            layers,
            extensions
        ));

        GUARD(createInstance("01-Hello-Vk", VK_MAKE_VERSION(1, 0, 0), layers, extensions, instance));
        GUARD(createSurface(instance, window, surface));
        GUARD(selectPhysicalDevice(instance, surface, { VK_KHR_SWAPCHAIN_EXTENSION_NAME }, pdCtx));
        GUARD(createDevice(pdCtx, device, graphicsQueue, presentQueue));
        GUARD(createSwapchain(device, surface, pdCtx, swapchainCtx));
        // GUARD(createCommandObjects());





        // Create render pass
        VkAttachmentDescription colorAttachment = {
            .format                 = swapchainCtx.imageFormat,
            .samples                = VK_SAMPLE_COUNT_1_BIT,
            .loadOp                 = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp                = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp          = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp         = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout          = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout            = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };
        VkAttachmentReference colorAttachmentRef = {
            .attachment             = 0,
            .layout                 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        VkSubpassDescription subpass = {
            .pipelineBindPoint      = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount   = 1,
            .pColorAttachments      = &colorAttachmentRef,
        };
        VkSubpassDependency dependency = {
            .srcSubpass             = VK_SUBPASS_EXTERNAL,
            .dstSubpass             = 0,
            .srcStageMask           = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask           = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask          = 0,
            .dstAccessMask          = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        };
        VkRenderPassCreateInfo renderPassInfo = {
            .sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount        = 1,
            .pAttachments           = &colorAttachment,
            .subpassCount           = 1,
            .pSubpasses             = &subpass,
            .dependencyCount        = 1,
            .pDependencies          = &dependency,
        };
        GUARDV(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

        // Create graphics pipeline
        std::wstring assetsPath = getAssetsPathW();
        auto vertShaderCode = readFile(assetsPath + L"03-triangle-shaders-vk/shader.vert.spv");
        auto fragShaderCode = readFile(assetsPath + L"03-triangle-shaders-vk/shader.frag.spv");
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
        VkShaderModuleCreateInfo vertCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = vertShaderCode.size(),
            .pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data()),
        };
        VkShaderModuleCreateInfo fragCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = fragShaderCode.size(),
            .pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data()),
        };
        if (vkCreateShaderModule(device, &vertCreateInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
            printf("failed to create vertex shader module");
            return 0;
        }
        if (vkCreateShaderModule(device, &fragCreateInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
            printf("failed to create fragment shader module");
            return 0;
        }
        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
            .sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage   = VK_SHADER_STAGE_VERTEX_BIT,
            .module  = vertShaderModule,
            .pName   = "main",
        };
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
            .sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage   = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module  = fragShaderModule,
            .pName   = "main",
        };
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 0,
            .vertexAttributeDescriptionCount = 0,
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
            .polygonMode              = VK_POLYGON_MODE_FILL,
            .cullMode                 = VK_CULL_MODE_BACK_BIT,
            .frontFace                = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable          = VK_FALSE,
            .lineWidth                = 1.0f,
        };
        VkPipelineMultisampleStateCreateInfo multisampling = {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable   = VK_FALSE,
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
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount         = 0,
            .pushConstantRangeCount = 0,
        };
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            printf("failed to create pipeline layout");
            return 0;
        }
        VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount          = 2,
            .pStages             = shaderStages,
            .pVertexInputState   = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState      = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pColorBlendState    = &colorBlending,
            .pDynamicState       = &dynamicState,
            .layout              = pipelineLayout,
            .renderPass          = renderPass,
            .subpass             = 0,
            .basePipelineHandle  = VK_NULL_HANDLE,
        };
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            printf("failed to create graphics pipeline");
            return 0;
        }
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);

        // Create frame buffers
        swapchainCtx.framebuffers.resize(swapchainCtx.imageViews.size());
        for (size_t i = 0; i < swapchainCtx.imageViews.size(); i++) {
            VkImageView attachments[] = {
                swapchainCtx.imageViews[i],
            };
            VkFramebufferCreateInfo framebufferInfo = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = renderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = swapchainCtx.extent.width,
                .height = swapchainCtx.extent.height,
                .layers = 1,
            };
            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainCtx.framebuffers[i]) != VK_SUCCESS) {
                printf("failed to create framebuffer");
                return 0;
            }
        }

        // Create command pool
        VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = pdCtx.gIdx,
        };
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            printf("failed to create command pool");
            return 0;
        }

        // Create command buffer
        VkCommandBufferAllocateInfo allocInfo = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = commandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
            printf("failed to allocate command buffers");
            return 0;
        }

        // Create sync objects
        VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
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
        for (auto framebuffer : swapchainCtx.framebuffers) {
            vkDestroyFramebuffer (device, framebuffer, nullptr);
        }
        vkDestroyPipeline        (device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout  (device, pipelineLayout, nullptr);
        vkDestroyRenderPass      (device, renderPass, nullptr);
        for (auto imageView : swapchainCtx.imageViews) {
            vkDestroyImageView   (device, imageView, nullptr);
        }
        vkDestroySwapchainKHR    (device, swapchainCtx.swapchain, nullptr);
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
        vkAcquireNextImageKHR(device, swapchainCtx.swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);

        // Record command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            printf("failed to begin recording command buffer");
            return 0;
        }
        VkClearValue vkClearColor = { {{clearColor.v[0], clearColor.v[1], clearColor.v[2], clearColor.v[3]}} };
        VkRenderPassBeginInfo renderPassInfo = {
            .sType                = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass           = renderPass,
            .framebuffer          = swapchainCtx.framebuffers[imageIndex],
            .renderArea           = { .offset    = { 0, 0 }, .extent = swapchainCtx.extent },
            .clearValueCount      = 1,
            .pClearValues         = &vkClearColor,
        };
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        VkViewport viewport = {
            .x          = 0.0f,
            .y          = 0.0f,
            .width      = static_cast<float>(swapchainCtx.extent.width),
            .height     = static_cast<float>(swapchainCtx.extent.height),
            .minDepth   = 0.0f,
            .maxDepth   = 1.0f,
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        VkRect2D scissor = {
            .offset = { 0, 0 },
            .extent = swapchainCtx.extent,
        };
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
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = waitSemaphores,
            .pWaitDstStageMask    = waitStages,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = signalSemaphores,
        };
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
            printf("failed to submit draw command buffer");
            return 0;
        }
        VkSwapchainKHR swapchains[] = { swapchainCtx.swapchain };
        VkPresentInfoKHR presentInfo = {
            .sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount  = 1,
            .pWaitSemaphores     = signalSemaphores,
            .swapchainCount      = 1,
            .pSwapchains         = swapchains,
            .pImageIndices       = &imageIndex,
        };
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
    VkInstance                          instance;
    PFN_vkCreateDebugUtilsMessengerEXT  createDebugMessengerFn;
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessengerFn;
    VkDebugUtilsMessengerEXT            debugMessenger;
    VkSurfaceKHR                        surface;
    PhysicalDeviceCtx                   pdCtx;
    SwapchainCtx                        swapchainCtx;
    VkDevice                            device;
    VkQueue                             graphicsQueue;
    VkQueue                             presentQueue;
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
