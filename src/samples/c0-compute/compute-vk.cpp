#include "compute.h"
#include <gpu/gpu.h>
#include <vector>

namespace gpu {

class Compute : public ICompute {
public:
    bool init() {
        // GUARD(instance.init("c0-compute-vk", VK_MAKE_VERSION(1, 0, 0), false, {}));
        
        // std::vector<const char*> deviceExtensions = {};
        // GUARD(physicalDevice.init(instance.instance, deviceExtensions, false, true, nullptr));

        // // GUARD(gpu.init(computeWrap.physicalDevice, computeWrap.cIdx, gpu, computeQueue));

        // const char* shaderDir = "02-compute-shaders-vk";
        // Shader shader;
        // // GUARD(shader.load(gpu, shaderDir, "shader.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT));

        // VkComputePipelineCreateInfo computePipelineCreateInfo = {
        //     .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        //     .pNext = nullptr,
        //     .flags = 0,
        //     .stage = shader.getInfo(),
        //     .layout = 0,
        //     // .baePipelineHandle = 0,
        //     .basePipelineIndex = 0,
        // };
        // // vkCreateComputePipelines(gpu, 0, 1, &computePipelineCreateInfo, nullptr, &computePipeline);

        return true;
    }

    bool compute(int count, vec3* a, vec3* b, vec3* c) {
        return true;
    }

    void terminate() {
        // instance.terminate();
    }

private:
    // Instance        instance;
    // PhysicalDevice  physicalDevice;
    // Gpu          gpu;
    // VkQueue         computeQueue;
    // VkPipeline      computePipeline;


};

std::unique_ptr<ICompute> createComputeVk() {
    return std::make_unique<Compute>();
}

}