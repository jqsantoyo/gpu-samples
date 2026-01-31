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
#include <d3dcompiler.h>
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


constexpr int frameCount = 2;
class RendererD3D : public IRenderer {
public:
    int start(void* window, uint32_t screenWidth, uint32_t screenHeight) {
        auto hwnd = static_cast<HWND>(window);
        screenAR = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
        viewport = { 0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight) };
        scissorRect = { 0, 0, long(screenWidth), long(screenHeight) };


        ////////////////////////////////////////////////////////////////////////////////////////////
        // DEVICE
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

        D3D12_COMMAND_QUEUE_DESC queueDesc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        };
        GUARDHR(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue)));
        GUARDHR(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
        GUARDHR(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)));
        GUARDHR(cmdList.Get()->Close());

        GUARDHR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent == nullptr) {
            GUARDHR(HRESULT_FROM_WIN32(GetLastError()));
        }
        fenceValue = 1;
        rtvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


        ////////////////////////////////////////////////////////////////////////////////////////////
        // SWAPCHAIN
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width       = screenWidth,
            .Height      = screenHeight,
            .Format      = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc  = { .Count = 1, },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = frameCount,
            .SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        };
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFSDesc = {
            .Windowed = TRUE,
        };
        ComPtr<IDXGISwapChain1> swapChain1;
        GUARDHR(factory->CreateSwapChainForHwnd(cmdQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
        GUARDHR(swapChain1.As(&swapChain));
        GUARDHR(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
        frameIdx = swapChain->GetCurrentBackBufferIndex();


        ////////////////////////////////////////////////////////////////////////////////////////////
        // HEAPS
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = frameCount,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        };
        GUARDHR(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
        for (UINT n = 0; n < frameCount; n++) {
            GUARDHR(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
            device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvDescSize);
        }
        return 1;
    }

    void stop() {
        waitForPreviousFrame();
        CloseHandle(fenceEvent);
    }

    void setView(ViewDesc& desc) {
    }
    
    void setProjection(ProjectionDesc& desc) {

    }

    int render(const Color& clearColor, const std::vector<RenderItem>& items) {
        return 1;
    }
    
    int addBuffer(const BufferDesc& desc) {
        return -1;
    }
    
    int addMesh(const MeshDesc& desc) {
        return -1;
    }
    
    void setFillMode(FillMode mode) {
    }
    
    int waitForPreviousFrame() {
        const UINT64 f = fenceValue;
        GUARDHR(cmdQueue->Signal(fence.Get(), f));
        fenceValue++;
        if (fence->GetCompletedValue() < f) {
            GUARDHR(fence->SetEventOnCompletion(f, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
        }
        frameIdx = swapChain->GetCurrentBackBufferIndex();
        return 1;
    }

private:
    ComPtr<ID3D12Device>                device;
    ComPtr<ID3D12InfoQueue>             iq;
    ComPtr<ID3D12CommandQueue>          cmdQueue;
    ComPtr<ID3D12CommandAllocator>      cmdAllocator;
    ComPtr<ID3D12GraphicsCommandList>   cmdList;
    ComPtr<ID3D12Fence>                 fence;
    HANDLE                              fenceEvent;
    UINT64                              fenceValue;
    UINT                                rtvDescSize = 0;
    ComPtr<ID3D12DescriptorHeap>        rtvHeap;
    ComPtr<IDXGISwapChain3>             swapChain;
    ComPtr<ID3D12Resource>              renderTargets[frameCount];
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         pso;
    D3D12_VIEWPORT                      viewport;
    D3D12_RECT                          scissorRect;
    UINT                                frameIdx;
    float                               screenAR;
    ComPtr<ID3D12Resource>              vbUp;
    D3D12_VERTEX_BUFFER_VIEW            vbView;
};

std::unique_ptr<IRenderer> createRenderer() {
    return std::make_unique<RendererD3D>();
}
}
