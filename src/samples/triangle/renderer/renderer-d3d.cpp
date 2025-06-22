#include "renderer.h"
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

using namespace DirectX;
using namespace Microsoft::WRL;

#define GUARD(x) if (!x) { return 0; }
#define WINGUARD(hr) if (FAILED(hr)) { return 0; }

namespace gpu {

const int frameCount = 2;

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};

std::wstring GetAssetsPath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSlashIndex = path.find_last_of(L"\\/");
    std::wstring assetPath = path.substr(0, lastSlashIndex + 1);
    return assetPath;
}

void GetHardwareAdapter(IDXGIFactory1* factory, IDXGIAdapter1** ppAdapter) {
    *ppAdapter = nullptr;
    DXGI_ADAPTER_DESC1 desc;
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; SUCCEEDED(factory->EnumAdapters1(i, &adapter)); i++) {
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
            break;
        }
    }
    *ppAdapter = adapter.Detach();
}

class RendererD3D : public IRenderer {
public:
    ComPtr<IDXGISwapChain3>             swapChain;
    ComPtr<ID3D12Device>                device;
    ComPtr<ID3D12Resource>              renderTargets[frameCount];
    ComPtr<ID3D12CommandAllocator>      commandAllocator;
    ComPtr<ID3D12CommandQueue>          commandQueue;
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12DescriptorHeap>        rtvHeap;
    ComPtr<ID3D12PipelineState>         pipelineState;
    ComPtr<ID3D12GraphicsCommandList>   commandList;
    D3D12_VIEWPORT                      viewport;
    D3D12_RECT                          scissorRect;
    UINT                                rtvDescriptorSize = 0;
    UINT                                frameIdx;
    HANDLE                              fenceEvent;
    ComPtr<ID3D12Fence>                 fence;
    UINT64                              fenceValue;
    ComPtr<ID3D12Resource>              vb;
    D3D12_VERTEX_BUFFER_VIEW            vbView;


    int start(void* window, int screenWidth, int screenHeight) {
        auto windowHandle = static_cast<HWND>(window);
        float screenAR = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);

        viewport = { 0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight) };
        scissorRect = { 0, 0, screenWidth, screenHeight };

        UINT dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
        }

        ComPtr<IDXGIFactory4> factory;
        ComPtr<IDXGIAdapter1> hwAdapter;
        WINGUARD(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
        GetHardwareAdapter(factory.Get(), &hwAdapter);
        WINGUARD(D3D12CreateDevice(hwAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        WINGUARD(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = frameCount;
        swapChainDesc.Width = screenWidth;
        swapChainDesc.Height = screenHeight;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFSDesc = {};
        swapChainFSDesc.Windowed = TRUE;
        ComPtr<IDXGISwapChain1> swapChain1;
        WINGUARD(factory->CreateSwapChainForHwnd(commandQueue.Get(), windowHandle, &swapChainDesc, nullptr, nullptr, &swapChain1));

        WINGUARD(swapChain1.As(&swapChain));
        WINGUARD(factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));
        frameIdx = swapChain->GetCurrentBackBufferIndex();
    

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = frameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        WINGUARD(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
        for (UINT n = 0; n < frameCount; n++) {
            WINGUARD(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
            device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvDescriptorSize);
        }

        WINGUARD(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        WINGUARD(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        WINGUARD(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
        

        std::wstring assetsPath = GetAssetsPath();
        std::wstring shaderName = assetsPath + L"shaders.hlsl";
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;
        WINGUARD(D3DCompileFromFile(shaderName.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        WINGUARD(D3DCompileFromFile(shaderName.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
        psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        WINGUARD(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));

        WINGUARD(device->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(),
            pipelineState.Get(), IID_PPV_ARGS(&commandList)));
        WINGUARD(commandList.Get()->Close()); // By default in recording state, so close now.


        // Using upload heaps for now, but not recommended.
        Vertex triangleVertices[] = {
            { { 0.0f, 0.25f * screenAR, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * screenAR, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * screenAR, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };
        const UINT vertexBufferSize = sizeof(triangleVertices);
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        WINGUARD(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vb))
        );
        UINT8* vertexDataBegin;
        CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
        WINGUARD(vb->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
        memcpy(vertexDataBegin, triangleVertices, sizeof(triangleVertices));
        vb->Unmap(0, nullptr);
        vbView.BufferLocation = vb->GetGPUVirtualAddress();
        vbView.StrideInBytes = sizeof(Vertex);
        vbView.SizeInBytes = vertexBufferSize;


        // Wait for assets to upload to the GPU.
        WINGUARD(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fenceValue = 1;
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent == nullptr) {
            WINGUARD(HRESULT_FROM_WIN32(GetLastError()));
        }
        waitForPreviousFrame();
        return 1;
    }

    int update() {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIdx, rtvDescriptorSize);
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        WINGUARD(commandAllocator->Reset());
        WINGUARD(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));
        commandList->SetGraphicsRootSignature(rootSignature.Get());
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);
        commandList->ResourceBarrier(1, &barrier); // Use back buffer as a render target.
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vbView);
        commandList->DrawInstanced(3, 1, 0, 0);
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrier); // Use back buffer to present.
        WINGUARD(commandList->Close());
        ID3D12CommandList* cmdLists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
        WINGUARD(swapChain->Present(1, 0));
        GUARD(waitForPreviousFrame());
        return 1;
    }

    void stop() {
        waitForPreviousFrame();
        CloseHandle(fenceEvent);
    }
    
    int waitForPreviousFrame() {
        const UINT64 f = fenceValue;
        WINGUARD(commandQueue->Signal(fence.Get(), f));
        fenceValue++;
        if (fence->GetCompletedValue() < f) {
            WINGUARD(fence->SetEventOnCompletion(f, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
        }
        frameIdx = swapChain->GetCurrentBackBufferIndex();
        return 1;
    }

};

RendererPtr createRenderer() {
    return std::make_unique<RendererD3D>();
}
}








