#include "app/app.h"
#include "renderer/renderer.h"
#include "string.h"

namespace gpu {

class App: public IApp {
public:
    std::unique_ptr<IRenderer> renderer;

    int start(int argc, char** argv, void* window, int w, int h) {
        renderer = createRenderer();
        renderer->start(window, w, h);
        return true;
    }

    int update() {
        return renderer->render({.1, .1, .1, 1}, {});
    }

    int resize(int width, int height) {
        renderer->resize(width, height);
        return 1;
    }

    int mouseEvent(MouseEvent event) {
        return 1;
    }

    int keyboardEvent(KeyboardEvent event) {
        return 1;
    }

    void stop() {
        renderer->stop();
    }
};

std::unique_ptr<IApp> createApp() {
    return std::make_unique<App>();
}

}
