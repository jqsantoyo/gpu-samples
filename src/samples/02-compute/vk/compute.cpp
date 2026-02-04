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

        GUARD(createInstance("02-Compute-Vk", VK_MAKE_VERSION(1, 0, 0), layers, extensions, instance));
        

        ComputePhysicalDeviceWrap computeWrap;
        std::vector<const char*> deviceExtensions = {};
        GUARD(selectComputePhysicalDevice(instance, deviceExtensions, computeWrap));

        GUARD(createComputeDevice(computeWrap.physicalDevice, computeWrap.cIdx, device, computeQueue));

        const char* shaderDir = "02-compute-shaders-vk";
        Shader computeShader;
        GUARD(loadShader(device, shaderDir, "shader.comp.spv", computeShader));

        VkComputePipelineCreateInfo computePipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = computeShader.module,
                .pName = computeShader.name,
                .pSpecializationInfo = nullptr,
            },
            .layout = 0,
            // .baePipelineHandle = 0,
            .basePipelineIndex = 0,
        };
        vkCreateComputePipelines(device, 0, 1, &computePipelineCreateInfo, nullptr, &computePipeline);

        return 1;
    }
    int stop() {
        return 1;
    }

private:
    VkInstance instance;
    VkDevice device;
    VkQueue computeQueue;
    VkPipeline computePipeline;


};

std::unique_ptr<ICompute> createCompute() {
    return std::make_unique<Compute>();
}

}