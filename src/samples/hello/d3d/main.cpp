///////////////////////////////////////////////////////////////////////////////////////////////////
//
// First-D3D
//
// Draws a triangle based on Microsoft's Hello Triangle, but simplified and flattened.
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <directxmath.h>
#include <dxgi1_6.h>
#include <iostream>
#include <shellapi.h>
#include <stdio.h>
#include <windows.h>
#include <wrl.h>

#define GUARD(x)    if (!x)         {  std::cerr << "Error: "#x  << std::endl; exit(EXIT_FAILURE); }
#define GUARDHR(hr) if (FAILED(hr)) {  std::cerr << "Error: "#hr << std::endl; exit(EXIT_FAILURE); }

using namespace DirectX;
using namespace Microsoft::WRL;

constexpr int frameCount = 2;

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};

struct Renderer {
    ComPtr<ID3D12Device>              device;
    ComPtr<IDXGISwapChain3>           swapChain;
    ComPtr<ID3D12Resource>            renderTargets[frameCount];
    UINT                              rtvDescSize = 0;
    ComPtr<ID3D12DescriptorHeap>      rtvHeap;
    ComPtr<ID3D12CommandQueue>        cmdQueue;
    ComPtr<ID3D12CommandAllocator>    cmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> cmdList;
    HANDLE                            fenceEvent;
    ComPtr<ID3D12Fence>               fence;
    UINT64                            fenceValue;

    ComPtr<ID3D12RootSignature> rootSig;
    ComPtr<ID3D12PipelineState> pso;
    ComPtr<ID3D12Resource>      vb;
    D3D12_VERTEX_BUFFER_VIEW    vbView;
};

std::wstring GetAssetsPath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t       lastSlashIndex = path.find_last_of(L"\\/");
    std::wstring assetPath      = path.substr(0, lastSlashIndex + 1);
    return assetPath;
}

int waitForPreviousFrame(Renderer& r) {
    const UINT64 fence = r.fenceValue;
    HRESULT      res   = r.cmdQueue->Signal(r.fence.Get(), fence);
    GUARDHR(res);
    r.fenceValue++;
    if (r.fence->GetCompletedValue() < fence) {
        HRESULT res = r.fence->SetEventOnCompletion(fence, r.fenceEvent);
        GUARDHR(res);
        WaitForSingleObject(r.fenceEvent, INFINITE);
    }
    return 1;
}

LRESULT CALLBACK windowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE: PostQuitMessage(0); return 0;
    default      : return DefWindowProc(window, msg, wParam, lParam);
    }
}

// int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int nCmdShow) {
int main() {
    printf("Sample: hello-d3d\n");
    Renderer r;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Create window
    int        screenWidth  = 512;
    int        screenHeight = 512;
    float      screenAR     = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    RECT       windowRect   = {0, 0, screenWidth, screenHeight};
    HINSTANCE  instance     = GetModuleHandle(NULL);
    WNDCLASSEX windowClass  = {
         .cbSize        = sizeof(WNDCLASSEX),
         .style         = CS_HREDRAW | CS_VREDRAW, // CS_OWNDC;
         .lpfnWndProc   = windowProc,
         .hInstance     = instance,
         .hCursor       = LoadCursor(NULL, IDC_ARROW),
         .lpszClassName = L"hello-d3d",
    };
    GUARDHR(RegisterClassEx(&windowClass));
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    HWND window = CreateWindow(
        windowClass.lpszClassName,
        L"hello-d3d",
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
    GUARD(window);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Create device (and a fence, a queue, swapchain, & heaps)
    ComPtr<IDXGIFactory4> factory;
    ComPtr<ID3D12Debug>   debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }
    GUARDHR(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));
    GUARDHR(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&r.device)));

    // Create fence
    GUARDHR(r.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&r.fence)));
    r.fenceValue = 1;
    r.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (r.fenceEvent == nullptr) {
        GUARDHR(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Create queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {
        .Type  = D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
    };
    GUARDHR(r.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&r.cmdQueue)));

    // Create swapchain
    ComPtr<IDXGISwapChain1> swapChain;
    DXGI_SWAP_CHAIN_DESC1   swapChainDesc = {
          .Width       = static_cast<uint32_t>(screenWidth),
          .Height      = static_cast<uint32_t>(screenHeight),
          .Format      = DXGI_FORMAT_R8G8B8A8_UNORM,
          .SampleDesc  = {.Count = 1},
          .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
          .BufferCount = frameCount,
          .SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    };
    GUARDHR(factory->CreateSwapChainForHwnd(r.cmdQueue.Get(), window, &swapChainDesc, nullptr, nullptr, &swapChain));
    GUARDHR(swapChain.As(&r.swapChain));
    GUARDHR(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));

    // Create heaps
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = frameCount,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };
    GUARDHR(r.device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&r.rtvHeap)));
    r.rtvDescSize = r.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(r.rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT n = 0; n < frameCount; n++) {
        GUARDHR(r.swapChain->GetBuffer(n, IID_PPV_ARGS(&r.renderTargets[n])));
        r.device->CreateRenderTargetView(r.renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, r.rtvDescSize);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Create pipeline state object (root signature, shaders, input layout)
    ComPtr<ID3DBlob>            sig;
    ComPtr<ID3DBlob>            error;
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    GUARDHR(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
    GUARDHR(r.device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&r.rootSig)));

    std::wstring     assetsPath   = GetAssetsPath();
    std::wstring     shaderName   = assetsPath + L"shaders.hlsl";
    UINT             compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    GUARDHR(D3DCompileFromFile(
        shaderName.c_str(),
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        compileFlags,
        0,
        &vertexShader,
        nullptr
    ));
    GUARDHR(D3DCompileFromFile(
        shaderName.c_str(),
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        compileFlags,
        0,
        &pixelShader,
        nullptr
    ));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature  = r.rootSig.Get(),
        .VS              = {reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize()},
        .PS              = {reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize()},
        .BlendState      = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask      = UINT_MAX,
        .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState     = {.DepthEnable = FALSE, .StencilEnable = FALSE},
        .InputLayout           = {inputElementDescs, _countof(inputElementDescs)},
        .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets      = 1,
        .RTVFormats            = {DXGI_FORMAT_R8G8B8A8_UNORM},
        .SampleDesc            = {.Count = 1},
    };
    GUARDHR(r.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&r.pso)));

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Create command allocator & list
    GUARDHR(r.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&r.cmdAllocator)));
    GUARDHR(r.device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        r.cmdAllocator.Get(),
        r.pso.Get(),
        IID_PPV_ARGS(&r.cmdList)
    ));
    GUARDHR(r.cmdList.Get()->Close()); // By default in recording state, so close now.

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Upload some data using upload heaps for now, but not recommended.
    Vertex triangleVertices[] = {
        {{0.0f, 0.25f * screenAR, 0.0f},    {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.25f, -0.25f * screenAR, 0.0f},  {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-0.25f, -0.25f * screenAR, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}
    };
    const UINT              vertexBufferSize = sizeof(triangleVertices);
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    auto                    desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    GUARDHR(r.device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&r.vb)
    ));
    UINT8*        vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
    GUARDHR(r.vb->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
    memcpy(vertexDataBegin, triangleVertices, sizeof(triangleVertices));
    r.vb->Unmap(0, nullptr);
    r.vbView.BufferLocation = r.vb->GetGPUVirtualAddress();
    r.vbView.StrideInBytes  = sizeof(Vertex);
    r.vbView.SizeInBytes    = vertexBufferSize;

    // Wait for assets to upload to the GPU.
    GUARD(waitForPreviousFrame(r));

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Enter main loop
    ShowWindow(window, true ? SW_NORMAL : SW_MAXIMIZE);
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Build and execute command list
        const float    clearColor[]   = {0.1f, 0.1f, 0.1f, 1.0f};
        int            frameIdx       = r.swapChain->GetCurrentBackBufferIndex();
        D3D12_VIEWPORT viewport       = {0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight)};
        D3D12_RECT     scissorRect    = {0, 0, screenWidth, screenHeight};
        ComPtr<ID3D12Resource> target = r.renderTargets[frameIdx];

        auto barrierTarget = CD3DX12_RESOURCE_BARRIER::Transition(
            target.Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        auto barrierPresent = CD3DX12_RESOURCE_BARRIER::Transition(
            target.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
            r.rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            frameIdx,
            r.rtvDescSize
        );
        GUARDHR(r.cmdAllocator->Reset());
        GUARDHR(r.cmdList->Reset(r.cmdAllocator.Get(), r.pso.Get()));
        r.cmdList->SetGraphicsRootSignature(r.rootSig.Get());
        r.cmdList->RSSetViewports(1, &viewport);
        r.cmdList->RSSetScissorRects(1, &scissorRect);
        r.cmdList->ResourceBarrier(1, &barrierTarget); // Use back buffer as a render target.
        r.cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
        r.cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        r.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        r.cmdList->IASetVertexBuffers(0, 1, &r.vbView);
        r.cmdList->DrawInstanced(3, 1, 0, 0);
        r.cmdList->ResourceBarrier(1, &barrierPresent); // Use back buffer to present.
        GUARDHR(r.cmdList->Close());

        ID3D12CommandList* cmdLists[] = {r.cmdList.Get()};
        r.cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
        GUARDHR(r.swapChain->Present(1, 0));
        GUARD(waitForPreviousFrame(r));
    }
    GUARD(waitForPreviousFrame(r));
    CloseHandle(r.fenceEvent);
    DestroyWindow(window);
    return 1;
}
