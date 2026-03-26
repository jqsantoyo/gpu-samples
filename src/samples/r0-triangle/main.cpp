#include "utils/app.h"
#include "utils/renderer.h"
#include "string.h"

namespace gpu {

class App: public IApp {
public:
    const char* title;
    std::unique_ptr<IRenderer> renderer;
    int meshId;

    bool init(void* window, uint32_t width, uint32_t height) {
        bool useVulkan = argBool("-vk");
        title = useVulkan ? "00-triangle-vk" : "00-triangle";
        
        renderer = createRenderer(useVulkan);
        renderer->init(window, width, height);
        
        float screenAR = static_cast<float>(width) / static_cast<float>(height);
        float vertices[] = {
             0.0f,   0.25f * screenAR, 0.0f,
             0.25f, -0.25f * screenAR, 0.0f,
            -0.25f, -0.25f * screenAR, 0.0f,
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f
        };
        BufferDesc bufferDesc = { 0, sizeof(vertices), reinterpret_cast<uint8_t*>(vertices) };
        int bufferId = renderer->addBuffer(bufferDesc);
        MeshDesc meshDesc = {
            .bufferId   = bufferId,
            .vCount     = 3,
            .indices    = { 0, 0 },
            .position   = { 0, sizeof(float) * 3 * 3 },
            .normal     = { 0, 0 },
            .uv         = { 0, 0 },
            .color      = { sizeof(float) * 3 * 3, sizeof(float) * 3 * 3 },
        };
        meshId = renderer->addMesh(meshDesc);
        renderer->wait();
        return true;
    }

    void terminate() {
        renderer->terminate();
    }

    bool resize(int width, int height) {
        renderer->resize(width, height);
        return 1;
    }

    bool update() {
        FrameData frame = getFrameData();
        setWindowText("%s: fps: %f period: %.5f", title, 1 / frame.dtAvg, frame.dtAvg);
        RenderItem ri = { {}, {}, {}, meshId, 0 };
        return renderer->render({.1, .1, .1, 1}, { ri });
    }
};

}

int main(int argc, char** argv) {
    gpu::App app;
    gpu::AppRunner runner(argc, argv, app);
    return runner.run();
}