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

namespace gpu {

class RendererMt : public IRenderer {
public:
    int start(void* window, uint32_t screenWidth, uint32_t screenHeight) {
        printf("Device created");
        return true;
    }

    void stop() {
    }

    int resize(int width, int height) {
        return 1;
    }

    void setView(ViewDesc& desc) {
    }

    void setProjection(ProjectionDesc& desc) {
    }

    int render(const Color& clearColor, const std::vector<RenderItem>& items) {
        return 1;
    }
    
    void setFillMode(FillMode mode) {
    }
    
    int addBuffer(const BufferDesc& desc) {
        return -1;
    }
    
    int addMesh(const MeshDesc& desc) {
        return -1;
    }

private:
};

std::unique_ptr<IRenderer> createRenderer() {
    return std::make_unique<RendererMt>();
}
}
