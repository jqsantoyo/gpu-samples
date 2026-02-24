#include "app/app.h"
#include "renderer/renderer.h"
#include "string.h"

namespace gpu {

class App: public IApp {
public:
    std::unique_ptr<IRenderer> renderer;

    bool init(int argc, char** argv, void* window, uint32_t width, uint32_t height) {
        renderer = createRenderer();
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
        return renderer->render({.1, .1, .1, 1}, {});
    }
};

}

int main(int argc, char** argv) {
    gpu::App app;
    gpu::AppRunner runner(argc, argv, app);
    return runner.run();
}