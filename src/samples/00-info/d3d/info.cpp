#include <renderer/renderer.h>
#include <app/app.h>
#include <utils/utils.h>
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3dx12.h>
#include <d3d12.h>
#include <d3dcommon.h>
#include <directxmath.h>
#include <dxgi1_6.h>
#include <windows.h>
#include <wrl.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>
#define GUARD(x)    if (!x)         {  printf("Error: "#x); return 0; }
#define GUARDHR(hr) if (FAILED(hr)) {  printf("Error: "#hr); return 0; }
using namespace DirectX;
using namespace Microsoft::WRL;

namespace gpu {


bool info() {
    
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12InfoQueue> iq;

    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        printf("Debug layer enabled\n");
        debugController->EnableDebugLayer();
    }
    // ComPtr<ID3D12Debug1> debugController1;
    // if (SUCCEEDED(debugController.As(&debugController1))) {
    //     debugController1->SetEnableGPUBasedValidation(TRUE);
    // }
    ComPtr<IDXGIFactory4> factory;
    GUARDHR(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));
    GUARDHR(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
    if (SUCCEEDED(device.As(&iq))) {
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    }
    return 1;
}

}
