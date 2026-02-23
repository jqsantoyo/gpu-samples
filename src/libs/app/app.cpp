
#include "app.h"



#ifdef WIN32
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
#else
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Foundation/NSObject.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <AppKit/AppKit.hpp>
#endif
#define GUARD(x) if (!x) { return 1; }

namespace gpu {

AppRunner::AppRunner(IApp& app) : app(app) {}



#ifdef WIN32
LRESULT CALLBACK windowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
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
    default: return DefWindowProc(window, msg, wParam, lParam);
    }
    return 0;
}

bool AppRunner::run() {
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
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            run = app->update();
        }
    }
    app->stop();
    DestroyWindow(windowHandle);
    return true;
}




#else




// class MTKViewDelegate : public MTK::ViewDelegate
// {
// public:
//     MTKViewDelegate(IApp& app) : app(app) {}

//     virtual ~MTKViewDelegate() override {}

//     virtual void drawInMTKView(MTK::View* view) override {
//         app.update();
//     }

// private:
//     IApp& app;
// };



class Delegate : public NS::ApplicationDelegate {
public:
    Delegate(IApp& app) : app(app) {}

    virtual void applicationWillFinishLaunching( NS::Notification* pNotification ) override {
        NS::Application* nsApp = reinterpret_cast<NS::Application*>(pNotification->object());
        nsApp->setActivationPolicy( NS::ActivationPolicy::ActivationPolicyRegular );
    }

    virtual void applicationDidFinishLaunching( NS::Notification* pNotification ) override {
        uint32_t width = 512;
        uint32_t height = 512;
        CGRect frame = (CGRect){ {100.0, 100.0}, { static_cast<float>(width), static_cast<float>(height) } };

        window = NS::Window::alloc()->init(
            frame,
            NS::WindowStyleMaskClosable|NS::WindowStyleMaskTitled,
            NS::BackingStoreBuffered,
            false
        );

        CA::MetalLayer* metalLayer = CA::MetalLayer::layer();
        metalLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
        metalLayer->setFramebufferOnly(false);
        metalLayer->setDrawableSize(CGSizeMake(width, height));

        auto view = NS::View::alloc()->init(frame);
        view->sendMessage<void>("setWantsLayer:", true);
        view->sendMessage<void>("setLayer:", metalLayer);


        window->setContentView(view);
        window->setTitle(NS::String::string("App", NS::StringEncoding::UTF8StringEncoding));
        window->makeKeyAndOrderFront(nullptr);

        NS::Application* nsApp = reinterpret_cast<NS::Application*>(pNotification->object());
        nsApp->activateIgnoringOtherApps(true);

        // device = MTL::CreateSystemDefaultDevice();
        // viewDelegate = new MTKViewDelegate(app);
        // mtkView = MTK::View::alloc()->init(frame, device);
        // mtkView->setColorPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
        // mtkView->setClearColor(MTL::ClearColor::Make(1.0, 0.0, 0.0, 1.0));

        


        // RendererInitInfo rendererInitInfo = {
        //     .width = width,
        //     .height = height,
        //     .view = mtkView,
        //     .device = device,
        // };
        // app.init(0, nullptr, rendererInitInfo);
        // mtkView->setDelegate(viewDelegate);
    }

    virtual bool applicationShouldTerminateAfterLastWindowClosed(NS::Application* pSender) override {
        return true;
    }

private:
    IApp& app;
    NS::Window* window = nullptr;
    // MTK::View* mtkView = nullptr;
    // MTKViewDelegate* viewDelegate = nullptr;
    MTL::Device* device = nullptr;
};

bool AppRunner::run() {
    NS::AutoreleasePool* autoRel = NS::AutoreleasePool::alloc()->init();
    Delegate delegate(app);
    NS::Application* appNS = NS::Application::sharedApplication();
    appNS->setDelegate(&delegate);
    appNS->run();
    autoRel->release();
    app.terminate();
    return true;
}

#endif


}