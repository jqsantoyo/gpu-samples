#pragma once
#include <memory>
#include <input/input.h>

namespace gpu {



class IApp {
public:
    virtual ~IApp() = default;
    virtual int start(int argc, char** argv, void* window, int w, int h) = 0;
    virtual int update() = 0;
    virtual int resize(int width, int height) = 0;
    virtual int mouseEvent(MouseEvent event) = 0;
    virtual int keyboardEvent(KeyboardEvent event) = 0;
    virtual void stop() = 0;
};

std::unique_ptr<IApp> createApp(); ///< Defined in application-code

}