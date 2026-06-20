
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

    bool init(void* window, uint32_t width, uint32_t height) {
        bool useVulkan = argBool("-vk");
        title = useVulkan ? "r0-color-vk" : "r0-color";
        
        renderer        = createRendererBasic();
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
            [&]() {
                scene->addCamera("defaultCamera", 1, 3.1416 / 2, 0, 3.14159 / 4.0f, 1, 0.1f, 100.0f);
                float aspect = 1;
                float vertices[] = {
                    0.0f,   0.25f * aspect, 0.0f,
                    0.25f, -0.25f * aspect, 0.0f,
                    -0.25f, -0.25f * aspect, 0.0f,
                    1.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 1.0f
                };
                StaticBuffer buffer = renderer->create("Triangle", sizeof(vertices), reinterpret_cast<uint8_t*>(vertices));
                sceneLoader->record(buffer);
                MeshDesc meshDesc = {
                    .staticBuffer   = buffer,
                    .vCount         = 3,
                    .formatIndices  = Format::Unknown,
                    .offsetIndices  = 0,
                    .offsetPosition = 0,
                    .offsetNormal   = 0,
                    .offsetUv       = 0,
                    .offsetTangent  = 0,
                    .offsetColor    = sizeof(float) * 3 * 3,
                    .sizeIndices    = 0,
                    .sizePosition   = sizeof(float) * 3 * 3,
                    .sizeNormal     = 0,
                    .sizeUv         = 0,
                    .sizeTangent    = 0,
                    .sizeColor      = sizeof(float) * 3 * 3,
                };
                Mesh mesh = renderer->create(meshDesc);
                int objectIdx = scene->addObject("object.0", { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 }, mesh.idx, 0);
                return true;
            },
            [&](){ return sceneLoader->load("cube/cube.gltf"); },
            [&](){ return sceneLoader->load("shapes/shapes.gltf"); },
        });

        sceneSelector->load(2);
        renderer->wait();
        return true;
    }

    bool update() {
        FrameData frame = getFrameData();
        setWindowText("%s: fps: %f period: %.5f", title, 1 / frame.dtAvg, frame.dtAvg);
        trs2Transform(scene->objects.size(), scene->objects.getTrs(), scene->objects.getTransform());
        RenderView view = {
            .clearColor = { 0.1f, 0.1f, 0.1f, 1.0f },
            .fillMode   = Fill,
            .lightCount = 0,
            .modelCount = scene->objects.size(),
            .camera     = scene->cameras.getCamera(),
            .lights     = nullptr,
            .transforms = scene->objects.getTransform(),
            .models     = scene->objects.getModel(),
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