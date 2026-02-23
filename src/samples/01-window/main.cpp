#include "app/app.h"
#include "renderer/renderer.h"
#include "string.h"

namespace gpu {

class App: public IApp {
public:
    std::unique_ptr<IRenderer> renderer;

    bool init(int argc, char** argv, const RendererInitInfo& info) {
        renderer = createRenderer();
        renderer->init(info);
        return true;
    }

    void terminate() {
        renderer->terminate();
    }

    bool update() {
        return renderer->render({.1, .1, .1, 1}, {});
        return true;
    }

    bool resize(int width, int height) {
        renderer->resize(width, height);
        return true;
    }
};


}

int main(int argc, char** argv) {
    gpu::App app;
    gpu::AppRunner runner(app);
    return runner.run() ? 0 : 1;
}