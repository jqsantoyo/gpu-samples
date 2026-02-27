#pragma once
#include <memory>

namespace gpu {

enum MouseEventType {
    Move,
    Wheel,
    Button
};

enum MouseButton {
    Right,
    Left,
    Middle,
};

struct MouseEvent {
    MouseEventType type;
    int x;
    int y;
    int wheel;
    MouseButton button;
    bool down;
};

struct KeyboardEvent {
    int x = 0;
};

class IApp {
public:
    virtual ~IApp() = default;
    virtual bool init(int argc, char** argv, void* window, uint32_t width, uint32_t height) = 0;
    virtual void terminate() = 0;

    virtual bool resize(int width, int height) { return false; }
    virtual bool update() { return false; }
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