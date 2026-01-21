#pragma once
#include <renderer/renderer.h>
#include <memory>
#include <string>

namespace gpu {



struct ObjectDesc {
    std::string name;
    float position[3];
    float scale[3];
    float rotation[4];
    int meshId;
    int materialId;
};


class IScene {
public:
    virtual ~IScene() = default;
    virtual int addObject(ObjectDesc& desc) = 0;
    virtual std::vector<RenderItem>& get() = 0;
};

std::unique_ptr<IScene> createScene();

}