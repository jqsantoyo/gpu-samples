#include "renderer.h"

namespace gpu {




class RendererVk : public IRenderer {
public:

    int start(void* window, int screenWidth, int screenHeight) {
        return 1;
    }

    int update() {
        return 1;
    }

    void stop() {
    }

};

RendererPtr createRendererVk() {
    return std::make_unique<RendererVk>();
}
}








