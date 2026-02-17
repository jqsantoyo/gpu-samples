#include "../compute.h"
#include <utilsVk/utilsVk.h>
#include <vector>

namespace gpu {

class Compute : public ICompute {
public:
    int start(int argc, char** argv) {
        std::vector<const char*> layers;
        std::vector<const char*> extensions;
        GUARD(validateLayers(
            {
                "VK_LAYER_KHRONOS_validation"
            },
            {
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            },
            {
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            },
            layers,
            extensions
        ));

        GUARD(createInstance("02-Compute-Vk", VK_MAKE_VERSION(1, 0, 0), layers, extensions, instanceCtx));
        

        ComputePhysicalDeviceWrap computeWrap;
        std::vector<const char*> deviceExtensions = {};
        GUARD(selectComputePhysicalDevice(instanceCtx.instance, deviceExtensions, computeWrap));

        GUARD(createComputeDevice(computeWrap.physicalDevice, computeWrap.cIdx, device, computeQueue));

        const char* shaderDir = "02-compute-shaders-vk";
        Shader shader;
        GUARD(shader.load(device, shaderDir, "shader.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT));

        VkComputePipelineCreateInfo computePipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = shader.getInfo(),
            .layout = 0,
            // .baePipelineHandle = 0,
            .basePipelineIndex = 0,
        };
        vkCreateComputePipelines(device, 0, 1, &computePipelineCreateInfo, nullptr, &computePipeline);

        return 1;
    }
    int stop() {
        destroyInstance(instanceCtx);
        return 1;
    }

private:
    InstanceCtx instanceCtx;
    VkDevice device;
    VkQueue computeQueue;
    VkPipeline computePipeline;


};

std::unique_ptr<ICompute> createCompute() {
    return std::make_unique<Compute>();
}

}