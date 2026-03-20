#pragma once
#include <memory>
#include <string>
#include <utils/scene.h>
#include <utils/renderer.h>


namespace gpu {

class IAssets {
public:
    virtual ~IAssets() = default;
    virtual void init(IRenderer* renderer, IScene* scene) = 0;
    virtual bool load(const std::string& filename) = 0;
private:

};

std::unique_ptr<IAssets> createAssets();

}