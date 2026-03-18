#include <app/app.h>
#include <renderer/renderer.h>
#include <assets/assets.h>
#include <camera/camera.h>
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
        if (useVulkan) {
            title = "03-texture-vk";
            renderer = createRendererVk();
        } else {
            title = "03-texture";
            renderer = createRenderer();
        }
        renderer->init(window, width, height);
        renderer->setFillMode(FillWire);

        scene = createScene();

        assets = createAssets();
        assets->setup(renderer.get(), scene.get());
        assets->load("crate.gltf");

        camera = createCamera();

        printf("Completed start\n");
        return true;
    }

    bool update() {
        FrameData frame = getFrameData();
        setWindowText("%s: fps: %f period: %.5f", title, 1 / frame.dtAvg, frame.dtAvg);
        float posX;
        float posY;
        float posZ;
        camera->getCartesian(posX, posY, posZ);
        ViewDesc viewDesc = {
            { posX, posY, posZ },
            { 0, 0, 0 },
            { 0, 1, 0 },
        };
        ProjectionDesc projDesc = {
            .fovAngleY = 3.14159 / 4.0f,
            .aspectRatio = (float)512 / (float)512,
            .nearZ = 0.1f,
            .farZ = 100.0f,
        };
        renderer->setView(viewDesc);
        renderer->setProjection(projDesc);
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