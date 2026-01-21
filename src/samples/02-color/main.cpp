#include <app/app.h>
#include <renderer/renderer.h>
#include <assets/assets.h>
#include <string>

namespace gpu {

class App: public IApp {
public:
    std::unique_ptr<IRenderer> renderer;
    std::unique_ptr<IScene> scene;
    std::unique_ptr<IAssets> assets;
    int shape0;
    int shape1;
    int shape2;
    int shape3;
    int shape4;

    int start(int argc, char** argv, void* window, int w, int h) {
        renderer = createRenderer();
        renderer->start(window, w, h);
        renderer->setFillMode(FillWire);

        scene = createScene();

        
        assets = createAssets();
        assets->setup(renderer.get(), scene.get());
        // assets->load("cube.gltf");
        assets->load("shapes.gltf");

        // shape0 = renderer->addMesh({ // triangle
        //     { {  0.00f,  0.25f, 0.0f }, { 1.000f, 0.278f, 0.278f, 1.0f } },
        //     { {  0.25f, -0.25f, 0.0f }, { 1.000f, 0.278f, 0.278f, 1.0f } },
        //     { { -0.25f, -0.25f, 0.0f }, { 1.000f, 0.278f, 0.278f, 1.0f } },
        // });
        // shape1 = renderer->addMesh({ // square
        //     { { -0.80f, -0.80f, 0.0f }, { 1.000f, 0.902f, 0.278f, 1.0f } },
        //     { { -0.40f, -0.40f, 0.0f }, { 1.000f, 0.902f, 0.278f, 1.0f } },
        //     { { -0.40f, -0.80f, 0.0f }, { 1.000f, 0.902f, 0.278f, 1.0f } },
        //     { { -0.80f, -0.80f, 0.0f }, { 1.000f, 0.902f, 0.278f, 1.0f } },
        //     { { -0.80f, -0.40f, 0.0f }, { 1.000f, 0.902f, 0.278f, 1.0f } },
        //     { { -0.40f, -0.40f, 0.0f }, { 1.000f, 0.902f, 0.278f, 1.0f } },
        // });
        // shape2 = renderer->addMesh({ // rhombus
        //     { { -0.90f,  0.65f, 0.0f }, { 0.396f, 0.922f, 0.349f, 1.0f } },
        //     { { -0.60f,  0.90f, 0.0f }, { 0.396f, 0.922f, 0.349f, 1.0f } },
        //     { { -0.60f,  0.40f, 0.0f }, { 0.396f, 0.922f, 0.349f, 1.0f } },
        //     { { -0.60f,  0.90f, 0.0f }, { 0.396f, 0.922f, 0.349f, 1.0f } },
        //     { { -0.30f,  0.65f, 0.0f }, { 0.396f, 0.922f, 0.349f, 1.0f } },
        //     { { -0.60f,  0.40f, 0.0f }, { 0.396f, 0.922f, 0.349f, 1.0f } },
        // });
        // shape3 = renderer->addMesh({ // rectangle
        //     { {  0.30f,  0.50f, 0.0f }, { 0.349f, 0.894f, 0.922f, 1.0f } },
        //     { {  0.30f,  0.80f, 0.0f }, { 0.349f, 0.894f, 0.922f, 1.0f } },
        //     { {  0.90f,  0.80f, 0.0f }, { 0.349f, 0.894f, 0.922f, 1.0f } },
        //     { {  0.30f,  0.50f, 0.0f }, { 0.349f, 0.894f, 0.922f, 1.0f } },
        //     { {  0.90f,  0.80f, 0.0f }, { 0.349f, 0.894f, 0.922f, 1.0f } },
        //     { {  0.90f,  0.50f, 0.0f }, { 0.349f, 0.894f, 0.922f, 1.0f } },
        // });
        // shape4 = renderer->addMesh({ // bow-tie
        //     { {  0.30f, -0.40f, 0.0f }, { 0.961f, 0.435f, 0.133f, 1.0f } },
        //     { {  0.60f, -0.65f, 0.0f }, { 0.961f, 0.435f, 0.133f, 1.0f } },
        //     { {  0.30f, -0.90f, 0.0f }, { 0.961f, 0.435f, 0.133f, 1.0f } },
        //     { {  0.60f, -0.65f, 0.0f }, { 0.961f, 0.435f, 0.133f, 1.0f } },
        //     { {  0.90f, -0.40f, 0.0f }, { 0.961f, 0.435f, 0.133f, 1.0f } },
        //     { {  0.90f, -0.90f, 0.0f }, { 0.961f, 0.435f, 0.133f, 1.0f } },
        // });
        printf("Completed start\n");
        return true;
    }

    int update() {
        return renderer->render({ 0.1f, 0.1f, 0.1f, 1.0f }, scene->get());
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
