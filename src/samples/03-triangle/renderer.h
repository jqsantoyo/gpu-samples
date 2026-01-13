#pragma once
#include <memory>

namespace gpu {

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual int start(void* window, int screenWidth, int screenHeight) = 0;
    virtual int update() = 0;
    virtual void stop() = 0;
};

std::unique_ptr<IRenderer> createRenderer();


}