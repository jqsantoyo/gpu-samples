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
#include <stdio.h>


#define GUARD(x) if (!x) { return 1; }

std::unique_ptr<gpu::IApp> app;

LRESULT CALLBACK windowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    int w = GET_WHEEL_DELTA_WPARAM(wParam); // only valid on WM_MOUSEWHEEL
    switch (msg) {
    case WM_CLOSE      : PostQuitMessage(0); break;;
    case WM_MOUSEMOVE  : app->mouseEvent({ gpu::MouseEventType::Move,   GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Left,   false }); break;
    case WM_MOUSEWHEEL : app->mouseEvent({ gpu::MouseEventType::Wheel,  GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), w, gpu::MouseButton::Left,   false }); break;
    case WM_LBUTTONDOWN: app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Left,   true  }); break;
    case WM_RBUTTONDOWN: app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Right,  true  }); break;
    case WM_MBUTTONDOWN: app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Middle, true  }); break;
    case WM_LBUTTONUP  : app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Left,   false }); break;
    case WM_RBUTTONUP  : app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Right,  false }); break;
    case WM_MBUTTONUP  : app->mouseEvent({ gpu::MouseEventType::Button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, gpu::MouseButton::Middle, false }); break;
    default: return DefWindowProc(window, msg, wParam, lParam);
    }
    return 0;
}

int main(int argc, char** argv) {
    app = gpu::createApp();
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
        nullptr
    );
    if (!windowHandle) {
        return 1;
    }
    
    if(app->start(argc, argv, windowHandle, screenWidth, screenHeight)) {
        bool run = true;
        ShowWindow(windowHandle, true ? SW_NORMAL : SW_MAXIMIZE);
        MSG msg = {};
        while (run && msg.message != WM_QUIT) {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            run = app->update();
        }
    }
    app->stop();
    DestroyWindow(windowHandle);
    return 0;
}