#include "gpu.h"


namespace gpu {


std::unique_ptr<IGpu> createGpuD3D(const GpuDesc& desc);
std::unique_ptr<IGpu> createGpuMtl(const GpuDesc& desc);
std::unique_ptr<IGpu> createGpuVk(const GpuDesc& desc);


std::unique_ptr<IGpu> createGpu(const GpuDesc& desc, Backend backend) {
    switch(backend) {
        case DirectX: return createGpuD3D(desc);
        case Metal:   return createGpuMtl(desc);
        case Vulkan:  return createGpuVk(desc);
    }
}

}