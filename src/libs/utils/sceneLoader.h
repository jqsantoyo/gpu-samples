#pragma once
#include <memory>
#include <string>
#include <utils/scene.h>
#include <utils/renderer.h>


namespace gpu {

class SceneLoader {
public:
    void init(Scene* scene, IRenderer* renderer);
    bool load(const std::string& filename);
private:
    Scene* scene;
    IRenderer* renderer;
};

}