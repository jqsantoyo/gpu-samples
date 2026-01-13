#pragma once
#include <memory>

namespace gpu {

struct MouseEvent {
    int x = 0;
};

struct KeyboardEvent {
    int x = 0;
};

class IApp {
public:
    virtual ~IApp() = default;
    virtual int start(int argc, char** argv, void* window, int w, int h) = 0;
    virtual int update() = 0;
    virtual int mouseEvent(MouseEvent event) = 0;
    virtual int keyboardEvent(KeyboardEvent event) = 0;
    virtual void stop() = 0;
};

std::unique_ptr<IApp> createApp(); ///< Defined in application-code

}