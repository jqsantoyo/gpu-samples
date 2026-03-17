#include "app/app.h"
#include "renderer/renderer.h"
#include "string.h"

namespace gpu {

class App: public IApp {
public:
    const char* title;
    std::unique_ptr<IRenderer> renderer;

    bool init(void* window, uint32_t width, uint32_t height) {
        bool useVulkan = argBool("-vk");
        if (useVulkan) {
            title = "00-triangle-vk";
            renderer = createRendererVk();
        } else {
            title = "00-triangle";
            renderer = createRenderer();
        }
        renderer->init(window, width, height);
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
        return renderer->render({.1, .1, .1, 1}, {});
    }
};

}

int main(int argc, char** argv) {
    gpu::App app;
    gpu::AppRunner runner(argc, argv, app);
    return runner.run();
}