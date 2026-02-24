#pragma once
#include <memory>
#include <input/input.h>

namespace gpu {

class IApp {
public:
    virtual ~IApp() = default;
    virtual bool init(int argc, char** argv, void* window, uint32_t width, uint32_t height) = 0;
    virtual void terminate() = 0;
    virtual bool resize(int width, int height) = 0;
    virtual bool update() = 0;
    
    virtual bool mouseEvent(MouseEvent event) { return false; };
    virtual bool keyboardEvent(KeyboardEvent event) { return false; };
};

class AppRunner {
public:
    AppRunner(int argc, char** argv, IApp& app);
    int run();

private:
    int argc;
    char** argv;
    IApp& app;
};

}