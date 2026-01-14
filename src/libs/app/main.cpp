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
#include <wrl.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>


#define GUARD(x) if (!x) { return 1; }

LRESULT CALLBACK windowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(window, msg, wParam, lParam);
    }
}

int main(int argc, char** argv) {

    std::unique_ptr<gpu::IApp> app = gpu::createApp();
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