#pragma once
#include <memory>
#include <chrono>

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

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

enum KeyCode { // do not overlap with alphanumeric range 0x30-0x5A as per win32
    KeyLeft,
    KeyRight,
    KeyDown,
    KeyUp,
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
    bool press;
    uint32_t key;
};

struct FrameData {
    float dt;
    float dtAvg;
    uint64_t frameIdx;
};


class IApp {
public:
    virtual ~IApp() = default;
    virtual bool init(void* window, uint32_t width, uint32_t height) = 0;
    virtual void terminate() = 0;

    virtual bool resize(int width, int height) { return false; }
    virtual bool update() { return false; }
    virtual bool mouseEvent(MouseEvent event) { return false; };
    virtual bool keyboardEvent(KeyboardEvent event) { return false; };

    bool argBool(const char* arg);
    void setWindowText(const char* fmt, ...);
    const FrameData& getFrameData();

private:
    friend class AppRunner;
    void initInternal(int argc, char** argv, void* window);
    bool updateInternal();
    int argc;
    char** argv;
    void* window;
    TimePoint t0;
    FrameData frameData;
    char windowTitle[100];
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