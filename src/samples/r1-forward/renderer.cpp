#include "renderer.h"


namespace gpu {


std::unique_ptr<IRenderer> createRendererD3D();
std::unique_ptr<IRenderer> createRendererVk();
    
std::unique_ptr<IRenderer> createRendererPbr(bool vulkan) {
    if (vulkan) {
        return createRendererVk();
    } else {
        return createRendererD3D();
    }
}

};