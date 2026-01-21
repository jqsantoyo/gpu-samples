#include <renderer/renderer.h>

namespace gpu {




class RendererVk : public IRenderer {
public:

    int start(void* window, uint32_t screenWidth, uint32_t screenHeight) {
        return 1;
    }

    void stop() {
    }
    
    int addMesh(const std::vector<Vertex>& vertices) {
        return -1;
    }

    int render(const Color& clearColor, const std::vector<RenderItem>& items) {
        return 1;
    }

    void setFillMode(FillMode mode) {
    }

    int addBuffer(const BufferDesc& desc) {
        return 1;
    }
    
    int addMesh(const MeshDesc& desc) {
        return 1;
    }
};

std::unique_ptr<IRenderer> createRenderer() {
    return std::make_unique<RendererVk>();
}
}








