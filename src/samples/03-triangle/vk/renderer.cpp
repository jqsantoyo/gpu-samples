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

float vertices[] = {
     0.0f, -0.5f,  0.0f,    1.0f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.0f,    0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.0f,    0.0f, 0.0f, 1.0f,
};


class RendererVk : public IRenderer {
public:
    int start(void* window, uint32_t screenWidth, uint32_t screenHeight) {

        GUARD(instance.init("03-triangle-vk", VK_MAKE_VERSION(1, 0, 0), true, {}, {}));
        GUARD(surface.init(instance.instance, window));
        GUARD(physicalDevice.init(instance.instance, { VK_KHR_SWAPCHAIN_EXTENSION_NAME }, true, false, &surface));
        GUARD(device.init(physicalDevice, true, false));
        GUARD(swapchain.init(device.device, &surface, physicalDevice.physicalDevice, physicalDevice.gIdx, physicalDevice.pIdx, device.pQ));
        GUARD(frameControl.init(device.device, physicalDevice.gIdx, device.gQ, 2));
        GUARD(createPipeline());
        
        framebuffers.resize(swapchain.imageViews.size());
        for (size_t i = 0; i < swapchain.imageViews.size(); i++) {
            VkImageView attachments[] = {
                swapchain.imageViews[i],
            };
            VkFramebufferCreateInfo framebufferInfo = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = renderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = swapchain.extent.width,
                .height = swapchain.extent.height,
                .layers = 1,
            };
            GUARDV(vkCreateFramebuffer(device.device, &framebufferInfo, nullptr, &framebuffers[i]));
        }

        GUARD(meshControl.init(&device));
        GUARD(meshControl.addMesh(vertices, sizeof(vertices), mesh));

        return 1;
    }

    void stop() {
        vkDeviceWaitIdle (device.device);
        meshControl.deinit();
        vkDestroyPipeline (device.device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout (device.device, pipelineLayout, nullptr);
        vkDestroyRenderPass (device.device, renderPass, nullptr);
        frameControl.deinit();
        swapchain.deinit();
        device.deinit();
        surface.deinit();
        instance.deinit();
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
        Frame frame = frameControl.next();
        GUARD(swapchain.next(frame.imageReady));
        if (swapchain.recreated()) {
            // renderTarget.recreateFramebuffers(swapchain);
        }
        frameControl.begin();
        
    
        VkClearValue vkClearColor = { {{clearColor.v[0], clearColor.v[1], clearColor.v[2], clearColor.v[3]}} };
        VkRenderPassBeginInfo renderPassInfo = {
            .sType                = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass           = renderPass,
            .framebuffer          = framebuffers[swapchain.idx],
            .renderArea           = { .offset = { 0, 0 }, .extent = swapchain.extent },
            .clearValueCount      = 1,
            .pClearValues         = &vkClearColor,
        };
        vkCmdBeginRenderPass(frame.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(frame.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        VkViewport viewport = {
            .x          = 0.0f,
            .y          = 0.0f,
            .width      = static_cast<float>(swapchain.extent.width),
            .height     = static_cast<float>(swapchain.extent.height),
            .minDepth   = 0.0f,
            .maxDepth   = 1.0f,
        };
        vkCmdSetViewport(frame.cmdBuffer, 0, 1, &viewport);
        VkRect2D scissor = {
            .offset = { 0, 0 },
            .extent = swapchain.extent,
        };
        vkCmdSetScissor(frame.cmdBuffer, 0, 1, &scissor);

        VkBuffer buffer = meshControl.getMesh(mesh);
        VkBuffer buffers[] = { buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(frame.cmdBuffer, 0, 1, buffers, offsets);

        vkCmdDraw(frame.cmdBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(frame.cmdBuffer);
        GUARDV(vkEndCommandBuffer(frame.cmdBuffer));
        
        frameControl.end(swapchain.renderReady);
        swapchain.present();
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
    Instance                            instance;
    Surface                             surface;
    PhysicalDevice                      physicalDevice;
    Swapchain                           swapchain;
    Device                              device;
    VkRenderPass                        renderPass;
    std::vector<VkFramebuffer>          framebuffers;
    VkPipelineLayout                    pipelineLayout;
    VkPipeline                          graphicsPipeline;
    FrameControl                        frameControl;
    MeshControl                         meshControl;
    int                                 mesh;
    int                                 width;
    int                                 height;
    bool                                sizeChanged = false;

    bool createPipeline() {
        VkAttachmentDescription colorAttachment = {
            .format                 = swapchain.format,
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
        GUARDV(vkCreateRenderPass(device.device, &renderPassInfo, nullptr, &renderPass));

        const char* shaderDir = "03-triangle-shaders-vk";
        Shader vertShader;
        Shader fragShader;
        GUARD(vertShader.load(device.device, shaderDir, "shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
        GUARD(fragShader.load(device.device, shaderDir, "shader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShader.getInfo(), fragShader.getInfo() };


        VkVertexInputBindingDescription bindingDesc = {
            .binding = 0,
            .stride = sizeof(float) * (3 + 3),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
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
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = sizeof(float) * 3,
            }
        };
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = &bindingDesc,
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
        if (vkCreatePipelineLayout(device.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
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
        if (vkCreateGraphicsPipelines(device.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            printf("failed to create graphics pipeline");
            return 0;
        }
        return true;
    }
};

std::unique_ptr<IRenderer> createRenderer() {
    return std::make_unique<RendererVk>();
}
}
