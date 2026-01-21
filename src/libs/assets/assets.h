#pragma once
#include <memory>
#include <string>
#include <scene/scene.h>
#include <renderer/renderer.h>


namespace gpu {

class IAssets {
public:
    virtual ~IAssets() = default;
    virtual void setup(IRenderer* renderer, IScene* scene) = 0;
    virtual bool load(const std::string& filename) = 0;
private:

};

std::unique_ptr<IAssets> createAssets();

}