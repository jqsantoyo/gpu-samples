#include "renderer.h"
#include <app/app.h>
#include <scene/sceneLoader.h>
#include <scene/camera.h>
#include <string>

namespace gpu {

class App: public IApp {
public:
    const char* title;
    std::unique_ptr<IRenderer>      renderer;
    std::unique_ptr<Scene>          scene;
    std::unique_ptr<SceneLoader>    sceneLoader;
    std::unique_ptr<SceneSelector>  sceneSelector;
    std::unique_ptr<CameraCtrl>     cameraCtrl;
    bool                            enableShadows = true;

    bool init(void* window, uint32_t width, uint32_t height) {
        bool useVulkan = argBool("-vk");
        title = useVulkan ? "01-forward-vk" : "01-forward";

        renderer        = createRendererPbr();
        scene           = std::make_unique<Scene>();
        cameraCtrl      = std::make_unique<CameraCtrl>();
        sceneLoader     = std::make_unique<SceneLoader>();
        sceneSelector   = std::make_unique<SceneSelector>();

        RendererBaseDesc renderDesc = {
            .vulkan     = useVulkan,
            .window     = window,
            .windowSize = { width, height },
        };
        renderer->init(renderDesc);
        sceneLoader->init(scene.get(), renderer.get());
        sceneSelector->init(sceneLoader.get(), {
            // [&]() {
            //     bool result = sceneLoader->load("crate/crate.gltf");
            //     return result;
            // },
            [&]() {
                GUARD(sceneLoader->load("damagedHelmet/DamagedHelmet.gltf"));
                enableShadows = false;
                return true;
            },
            [&]() {
                GUARD(sceneLoader->load("ponyCar/car.gltf"));
                GUARD(sceneLoader->load("ground/ground.gltf"));
                enableShadows = true;
                return true;
            },
        });
        sceneSelector->load(1);
        renderer->wait();
        return true;
    }

    bool update() {
        FrameData frame = getFrameData();
        setWindowText("%s: fps: %f period: %.5f", title, 1 / frame.dtAvg, frame.dtAvg);
        trs2Transform(scene->objects.size(), scene->objects.getTrs(), scene->objects.getTransform());
        RenderView view = {
            .clearColor     = { 0.1f, 0.1f, 0.1f, 1.0f },
            .fillMode       = Fill,
            .enableShadows  = enableShadows,
            .lightCount     = 0,
            .modelCount     = scene->objects.size(),
            .camera         = scene->cameras.getCamera(),
            .lights         = nullptr,
            .transforms     = scene->objects.getTransform(),
            .models         = scene->objects.getModel(),
        };
        renderer->render(view);
        return true;
    }

    bool resize(int width, int height) {
        renderer->resize(width, height);
        return true;
    }

    bool mouseEvent(MouseEvent event) {
        cameraCtrl->mouseEvent(event, scene->cameras.size(), scene->cameras.getCameraPos(), scene->cameras.getCamera());
        return true;
    }
    
    bool keyboardEvent(KeyboardEvent event) {
        return sceneSelector->loadOnKeyboard(event);
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