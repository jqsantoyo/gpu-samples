#include "../compute.h"
#include <utilsVk/utilsVk.h>
#include <vector>

using namespace gpu::vk;
namespace gpu {

class Compute : public ICompute {
public:
    bool init(int argc, char** argv) {

        GUARD(instance.init("02-Compute-Vk", VK_MAKE_VERSION(1, 0, 0), false, {}));
        
        std::vector<const char*> deviceExtensions = {};
        GUARD(physicalDevice.init(instance.instance, deviceExtensions, false, true, nullptr));

        // GUARD(device.init(computeWrap.physicalDevice, computeWrap.cIdx, device, computeQueue));

        const char* shaderDir = "02-compute-shaders-vk";
        Shader shader;
        // GUARD(shader.load(device, shaderDir, "shader.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT));

        VkComputePipelineCreateInfo computePipelineCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = shader.getInfo(),
            .layout = 0,
            // .baePipelineHandle = 0,
            .basePipelineIndex = 0,
        };
        // vkCreateComputePipelines(device, 0, 1, &computePipelineCreateInfo, nullptr, &computePipeline);

        return true;
    }

    void terminate() {
        instance.terminate();
    }

private:
    Instance        instance;
    PhysicalDevice  physicalDevice;
    Device          device;
    VkQueue         computeQueue;
    VkPipeline      computePipeline;


};

std::unique_ptr<ICompute> createComputeVk() {
    return std::make_unique<Compute>();
}

}