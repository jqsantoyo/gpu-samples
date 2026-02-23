#include <renderer/renderer.h>
#include <app/app.h>
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
#include <inttypes.h>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <MetalKit/MetalKit.hpp>

namespace gpu {

class RendererMt : public IRenderer {
public:
    bool init(const RendererInitInfo& info) {
        printf("Device created");
        this->device = reinterpret_cast<MTL::Device*>(info.device);
        this->view = reinterpret_cast<MTK::View*>(info.view);
        return true;
    }

    void terminate() {

    }

    bool resize(int width, int height) {
        return true;
    }

    bool render(const Color& clearColor, const std::vector<RenderItem>& items) {
        printf("Render\n");
        return true;
    }

private:
    MTK::View* view = nullptr;
    MTL::Device* device = nullptr;
};

std::unique_ptr<IRenderer> createRenderer() {
    return std::make_unique<RendererMt>();
}
}
