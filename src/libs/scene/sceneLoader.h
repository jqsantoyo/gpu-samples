#pragma once
#include <memory>
#include <string>
#include <app/app.h>
#include <scene/scene.h>
#include <rendererInterface/renderer.h>
#include <functional>


namespace gpu {

class SceneLoader {
public:
    void init(Scene* scene, IRenderer* renderer);
    bool load(const std::string& filename);
private:
    Scene* scene;
    IRenderer* renderer;
};


class SceneSelector {
public:
    void init(Scene* scene, IRenderer* renderer, std::initializer_list<std::function<bool()>> loaders);
    bool load(int idx);
    bool loadOnKeyboard(KeyboardEvent event);
private:
    Scene* scene;
    IRenderer* renderer;
    std::vector<std::function<bool()>> loaders;
    int sceneIdx = 0;
};


}