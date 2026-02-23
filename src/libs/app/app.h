#pragma once
#include <memory>
#include <input/input.h>
#include <renderer/renderer.h>

namespace gpu {

class IApp {
public:
    virtual ~IApp() = default;
    virtual bool init(int argc, char** argv, const RendererInitInfo& info) = 0;
    virtual void terminate() = 0;
    virtual bool resize(int width, int height) = 0;
    virtual bool update() = 0;
    
    virtual bool mouseEvent(MouseEvent event) { return false; };
    virtual bool keyboardEvent(KeyboardEvent event) { return false; };
};

class AppRunner {
public:
    AppRunner(IApp& app);
    bool run();

private:
    IApp& app;
};

}