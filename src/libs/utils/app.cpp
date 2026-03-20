
#include "app.h"
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d12.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <dxgi1_6.h>
#include <windows.h>
#include <windowsX.h>
#include <wrl.h>
#include <shellapi.h>
#include <string>
#include <cstdio>
#include <cstdarg>
#define GUARD(x) if (!x) { return 1; }

namespace gpu {
    

bool IApp::argBool(const char* arg) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], arg) == 0) {
            return true;
        }
    }
    return false;
}

void IApp::setWindowText(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    auto hwnd = static_cast<HWND>(window);
    vsnprintf(windowTitle, sizeof(windowTitle), fmt, args);
    SetWindowTextA(hwnd, windowTitle);
    va_end(args);
}

const FrameData& IApp::getFrameData() {
    return frameData;
}

void IApp::initInternal(int argc, char** argv, void* window) {
    this->argc = argc;
    this->argv = argv;
    this->window = window;
    t0 = Clock::now();
    frameData.dtAvg = 0;
}

bool IApp::updateInternal() {
    TimePoint t1 = Clock::now();
    float alpha = .05;
    frameData.dt = std::chrono::duration<float>(t1 - t0).count();
    frameData.dtAvg = alpha * frameData.dt + (1 - alpha) * frameData.dtAvg;
    t0 = t1;
    bool result = update();
    return result;
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    IApp* app;

    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = reinterpret_cast<IApp*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        app = reinterpret_cast<IApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (app) {
        int w = GET_WHEEL_DELTA_WPARAM(wParam); // only valid on WM_MOUSEWHEEL
        switch (msg) {
            case WM_CLOSE      : PostQuitMessage(0); break;;
            case WM_SIZE       : app->resize(LOWORD(lParam), HIWORD(lParam)); break;
            case WM_MOUSEMOVE  : app->mouseEvent({ gpu::MouseEventType::Move,   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Left,   false }); break;
            case WM_MOUSEWHEEL : app->mouseEvent({ gpu::MouseEventType::Wheel,  GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), w, gpu::MouseButton::Left,   false }); break;
            case WM_LBUTTONDOWN: app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Left,   true  }); break;
            case WM_RBUTTONDOWN: app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Right,  true  }); break;
            case WM_MBUTTONDOWN: app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Middle, true  }); break;
            case WM_LBUTTONUP  : app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Left,   false }); break;
            case WM_RBUTTONUP  : app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Right,  false }); break;
            case WM_MBUTTONUP  : app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Middle, false }); break;
            default:  return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    } else {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


AppRunner::AppRunner(int argc, char** argv, IApp& app) : argc(argc), argv(argv), app(app) {}

int AppRunner::run() {
    int screenWidth = 512;
    int screenHeight = 512;
    float screenAR = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);

    HINSTANCE instance = GetModuleHandle(NULL);
    WNDCLASSEX windowClass      = {};
    windowClass.cbSize          = sizeof(WNDCLASSEX);
    windowClass.style           = CS_HREDRAW | CS_VREDRAW; //CS_OWNDC;
    windowClass.lpfnWndProc     = windowProc;
    windowClass.hInstance       = instance;
    windowClass.hCursor         = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName   = L"DXSampleClass";;
    GUARD(RegisterClassEx(&windowClass));

    RECT windowRect = { 0, 0, screenWidth, screenHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    HWND windowHandle = CreateWindow(
        windowClass.lpszClassName,
        L"Sample",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        instance,
        &app
    );
    if (!windowHandle) {
        return -1;
    }
    
    SetThreadDescription(GetCurrentThread(), L"MainThread");
    app.initInternal(argc, argv, windowHandle);
    if(app.init(windowHandle, screenWidth, screenHeight)) {
        bool run = true;
        ShowWindow(windowHandle, true ? SW_NORMAL : SW_MAXIMIZE);
        MSG msg = {};
        while (run && msg.message != WM_QUIT) {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            run = app.updateInternal();
        }
    }
    app.terminate();
    DestroyWindow(windowHandle);
    return 0;
}


}