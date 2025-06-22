#include "app/app.h"
#include "renderer/renderer.h"
#include "string.h"

namespace gpu {

class App: public IApp {
public:
    RendererPtr renderer;

    int start(int argc, char** argv, void* window, int w, int h) {
        renderer = (argc == 2 && strcmp(argv[1], "-vk") == 0) ? createRendererVk() : createRenderer();
        return renderer->start(window, w, h);
    }

    int update() {
        return renderer->update();
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

AppPtr createApp() {
    return std::make_unique<App>();
}

}
