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
        title = useVulkan ? "01-objects-vk" : "01-objects";
        
        renderer = createRenderer(useVulkan);
        renderer->init(window, width, height);
        renderer->setFillMode(FillWire);

        scene = createScene();

        assets = createAssets();
        assets->init(renderer.get(), scene.get());
        // assets->load("cube.gltf");
        assets->load("shapes.gltf");

        camera = createCamera();


        
        // float screenAR = static_cast<float>(width) / static_cast<float>(height);
        // float vertices[] = {
        //      0.0f,   0.25f * screenAR, 0.0f,
        //      0.25f, -0.25f * screenAR, 0.0f,
        //     -0.25f, -0.25f * screenAR, 0.0f,
        //     1.0f, 0.0f, 0.0f,
        //     0.0f, 1.0f, 0.0f,
        //     0.0f, 0.0f, 1.0f
        // };
        // BufferDesc bufferDesc = { 0, sizeof(vertices), reinterpret_cast<uint8_t*>(vertices) };
        // int bufferId = renderer->addBuffer(bufferDesc);
        // MeshDesc meshDesc = {
        //     .bufferId   = bufferId,
        //     .vCount     = 3,
        //     .indices    = { 0, 0 },
        //     .position   = { 0, sizeof(float) * 3 * 3 },
        //     .normal     = { 0, 0 },
        //     .uv         = { 0, 0 },
        //     .color      = { sizeof(float) * 3 * 3, sizeof(float) * 3 * 3 },
        // };
        // int meshId = renderer->addMesh(meshDesc);




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