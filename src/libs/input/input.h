#pragma once


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

}