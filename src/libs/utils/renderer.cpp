#include "renderer.h"



namespace gpu {


    
std::unique_ptr<IRenderer> createRenderer(bool vulkan) {
    if (vulkan) {
        return createRendererVk();
    } else {
        return createRendererD3D();
    }
}

};