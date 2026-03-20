#include <utils/app.h>
#include <utils/renderer.h>
#include <utils/assets.h>
#include <utils/camera.h>
#include <string>

namespace gpu {

class App: public IApp {
public:
    const char* title;
    std::unique_ptr<IRenderer> renderer;
    std::unique_ptr<IScene> scene;
    std::unique_ptr<IAssets> assets;
    std::unique_ptr<ICamera> camera;

    bool init(void* window, uint32_t width, uint32_t height) {
        bool useVulkan = argBool("-vk");
        title = useVulkan ? "03-forward-vk" : "03-forward";

        renderer = createRenderer(useVulkan);
        renderer->init(window, width, height);
        renderer->setFillMode(FillWire);

        scene = createScene();

        assets = createAssets();
        assets->init(renderer.get(), scene.get());
        assets->load("crate.gltf");

        camera = createCamera();

        printf("Completed start\n");
        return true;
    }

    bool update() {
        FrameData frame = getFrameData();
        float aspect = (float)512 / (float)512;
        vec3 pos = camera->getCartesian();
        setWindowText("%s: fps: %f period: %.5f", title, 1 / frame.dtAvg, frame.dtAvg);
        renderer->setView(pos, { 0, 0, 0 }, { 0, 1, 0 });
        renderer->setProjection(3.14159 / 4.0f, aspect, 0.1f, 100.0f);
        return renderer->render({ 0.1f, 0.1f, 0.1f, 1.0f }, scene->get());
    }

    bool resize(int width, int height) {
        renderer->resize(width, height);
        return true;
    }

    bool mouseEvent(MouseEvent event) {
        camera->mouseEvent(event);
        return true;
    }

    void terminate() {
        renderer->terminate();
    }
};

}


int main(int argc, char** argv) {
    gpu::App app;
    gpu::AppRunner runner(argc, argv, app);
    return runner.run();
}