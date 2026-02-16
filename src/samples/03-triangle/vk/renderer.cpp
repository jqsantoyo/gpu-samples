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



const int frameCount = 2;

class RendererVk : public IRenderer {
public:
    int start(void* window, uint32_t screenWidth, uint32_t screenHeight) {
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

        GUARD(createInstance("03-triangle-vk", VK_MAKE_VERSION(1, 0, 0), layers, extensions, instanceCtx));
        GUARD(createSurface(instanceCtx.instance, window, surface));
        GUARD(selectPhysicalDevice(instanceCtx.instance, surface, { VK_KHR_SWAPCHAIN_EXTENSION_NAME }, pdCtx));
        GUARD(createDevice(pdCtx, device, graphicsQueue, presentQueue));
        GUARD(createSwapchain(device, surface, pdCtx, swapchainCtx));
        GUARD(createPipeline());
        GUARD(createSwapchainFramebuffers(device, swapchainCtx, renderPass));
        GUARD(createFrameControl(device, pdCtx.gIdx, 2, frameControl));
        return 1;
    }

    void stop() {
        vkDeviceWaitIdle         (device);
        destroyFrameControl      (device, frameControl);
        destroySwapchain         (device, swapchainCtx);
        vkDestroyPipeline        (device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout  (device, pipelineLayout, nullptr);
        vkDestroyRenderPass      (device, renderPass, nullptr);
        vkDestroyDevice          (device, nullptr);
        vkDestroySurfaceKHR      (instanceCtx.instance, surface, nullptr);
        destroyInstance          (instanceCtx);
    }

    int resize(int width, int height) {
        this->width = width;
        this->height = height;
        sizeChanged = true;
        return 1;
    }

    void setView(ViewDesc& desc) {
    }

    void setProjection(ProjectionDesc& desc) {
    }

    int render(const Color& clearColor, const std::vector<RenderItem>& items) {
        Frame frame = nextFrame(frameControl, device);
        
        uint32_t imageIndex;
        VkResult res = vkAcquireNextImageKHR(device, swapchainCtx.swapchain, UINT64_MAX, frame.imageReady, VK_NULL_HANDLE, &imageIndex);
        if (res == VK_ERROR_OUT_OF_DATE_KHR || sizeChanged) {
            sizeChanged = false;
            createSwapchain(device, surface, pdCtx, swapchainCtx);
            createSwapchainFramebuffers(device, swapchainCtx, renderPass);
        } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
            printf("Acquire image error\n");
            return 0;
        }
        VkFramebuffer fb = swapchainCtx.framebuffers[imageIndex];
        beginFrame(frame, device);
        
    
        VkClearValue vkClearColor = { {{clearColor.v[0], clearColor.v[1], clearColor.v[2], clearColor.v[3]}} };
        VkRenderPassBeginInfo renderPassInfo = {
            .sType                = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass           = renderPass,
            .framebuffer          = fb,
            .renderArea           = { .offset    = { 0, 0 }, .extent = swapchainCtx.extent },
            .clearValueCount      = 1,
            .pClearValues         = &vkClearColor,
        };
        vkCmdBeginRenderPass(frame.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        VkViewport viewport = {
            .x          = 0.0f,
            .y          = 0.0f,
            .width      = static_cast<float>(swapchainCtx.extent.width),
            .height     = static_cast<float>(swapchainCtx.extent.height),
            .minDepth   = 0.0f,
            .maxDepth   = 1.0f,
        };
        vkCmdSetViewport(frame.cmdBuffer, 0, 1, &viewport);
        VkRect2D scissor = {
            .offset = { 0, 0 },
            .extent = swapchainCtx.extent,
        };
        vkCmdSetScissor(frame.cmdBuffer, 0, 1, &scissor);
        vkCmdDraw(frame.cmdBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(frame.cmdBuffer);
        GUARDV(vkEndCommandBuffer(frame.cmdBuffer));
        
        endFrame(frame, graphicsQueue);

        VkSwapchainKHR swapchains[] = { swapchainCtx.swapchain };
        VkPresentInfoKHR presentInfo = {
            .sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount  = 1,
            .pWaitSemaphores     = &frame.renderReady,
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
    InstanceCtx                         instanceCtx;
    VkSurfaceKHR                        surface;
    PhysicalDeviceCtx                   pdCtx;
    SwapchainCtx                        swapchainCtx;
    VkDevice                            device;
    VkQueue                             graphicsQueue;
    VkQueue                             presentQueue;
    VkRenderPass                        renderPass;
    VkPipelineLayout                    pipelineLayout;
    VkPipeline                          graphicsPipeline;
    FrameControl                        frameControl;
    VkFence                             inFlightFence;
    int                                 width;
    int                                 height;
    bool                                sizeChanged = false;

    bool createPipeline() {
        VkAttachmentDescription colorAttachment = {
            .format                 = swapchainCtx.format,
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

        const char* shaderDir = "03-triangle-shaders-vk";
        Shader vertShader;
        Shader fragShader;
        GUARD(loadShader(device, shaderDir, "shader.vert.spv", vertShader, VK_SHADER_STAGE_VERTEX_BIT));
        GUARD(loadShader(device, shaderDir, "shader.frag.spv", fragShader, VK_SHADER_STAGE_FRAGMENT_BIT));
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShader.info, fragShader.info };
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
        vkDestroyShaderModule(device, fragShader.module, nullptr);
        vkDestroyShaderModule(device, vertShader.module, nullptr);
        return true;
    }
};

std::unique_ptr<IRenderer> createRenderer() {
    return std::make_unique<RendererVk>();
}
}
