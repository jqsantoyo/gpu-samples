#include <utils/app.h>
#include <utils/renderer.h>
#include <utils/sceneLoader.h>
#include <utils/camera.h>
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
        title = useVulkan ? "01-objects-vk" : "01-objects";
        
        renderer        = createRenderer(useVulkan);
        scene           = std::make_unique<Scene>();
        cameraCtrl      = std::make_unique<CameraCtrl>();
        sceneLoader     = std::make_unique<SceneLoader>();
        sceneSelector   = std::make_unique<SceneSelector>();


        renderer->init(window, width, height);
        sceneLoader->init(scene.get(), renderer.get());
        sceneSelector->init(scene.get(), renderer.get(), {
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
                int meshId = renderer->addMesh(meshDesc);
                int objectIdx = scene->addObject("object.0", { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 }, meshId, 0);
                return true;
            },
            [&](){ return sceneLoader->load("cube.gltf"); },
            [&](){ return sceneLoader->load("shapes.gltf"); },
        });

        sceneSelector->load(0);
        renderer->wait();
        return true;
    }

    bool update() {
        FrameData frame = getFrameData();
        setWindowText("%s: fps: %f period: %.5f", title, 1 / frame.dtAvg, frame.dtAvg);
        renderer->trs2Transform(scene->objects.size(), scene->objects.getTrs(), scene->objects.getTransform());
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
        return renderer->render(view);
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