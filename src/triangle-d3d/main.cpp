///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Triangle Direct3D
//
// Sample that draws a triangle based on Microsoft's Hello Triangle, but simplified and flattened.
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#define UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3dx12/d3dx12.h>
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
const int frameCount = 2;

struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};

struct Renderer {
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
};

inline void WINRES(HRESULT hr) {
    if (FAILED(hr)) {
        throw std::exception();
    }
}

std::wstring GetAssetsPath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t lastSlashIndex = path.find_last_of(L"\\/");
    std::wstring assetPath = path.substr(0, lastSlashIndex + 1);
    return assetPath;
}

void waitForPreviousFrame(Renderer& r) {
    const UINT64 fence = r.fenceValue;
    WINRES(r.commandQueue->Signal(r.fence.Get(), fence));
    r.fenceValue++;
    if (r.fence->GetCompletedValue() < fence) {
        WINRES(r.fence->SetEventOnCompletion(fence, r.fenceEvent));
        WaitForSingleObject(r.fenceEvent, INFINITE);
    }
    r.frameIdx = r.swapChain->GetCurrentBackBufferIndex();
}

LRESULT CALLBACK windowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(window, msg, wParam, lParam);
    }
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

//int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int nCmdShow) {
int main() {
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
    WINRES(RegisterClassEx(&windowClass));

    RECT windowRect = { 0, 0, screenWidth, screenHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    HWND windowHandle = CreateWindow(
        windowClass.lpszClassName,
        L"Triangle",
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

    Renderer r;
    r.viewport = { 0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight) };
    r.scissorRect = { 0, 0, screenWidth, screenHeight };

    UINT dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
    }

    ComPtr<IDXGIFactory4> factory;
    ComPtr<IDXGIAdapter1> hwAdapter;
    WINRES(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
    GetHardwareAdapter(factory.Get(), &hwAdapter);
    WINRES(D3D12CreateDevice(hwAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&r.device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    WINRES(r.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&r.commandQueue)));

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
    ComPtr<IDXGISwapChain1> swapChain;
    WINRES(factory->CreateSwapChainForHwnd(r.commandQueue.Get(), windowHandle, &swapChainDesc, nullptr, nullptr, &swapChain));

    WINRES(swapChain.As(&r.swapChain));
    WINRES(factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));
    r.frameIdx = r.swapChain->GetCurrentBackBufferIndex();
 

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = frameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    WINRES(r.device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&r.rtvHeap)));
    r.rtvDescriptorSize = r.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(r.rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT n = 0; n < frameCount; n++) {
        WINRES(r.swapChain->GetBuffer(n, IID_PPV_ARGS(&r.renderTargets[n])));
        r.device->CreateRenderTargetView(r.renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, r.rtvDescriptorSize);
    }

    WINRES(r.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&r.commandAllocator)));

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    WINRES(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    WINRES(r.device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&r.rootSignature)));
    

    std::wstring assetsPath = GetAssetsPath();
    std::wstring shaderName = assetsPath + L"triangle-d3d/shaders.hlsl";
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    WINRES(D3DCompileFromFile(shaderName.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
    WINRES(D3DCompileFromFile(shaderName.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = r.rootSignature.Get();
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
    WINRES(r.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&r.pipelineState)));

    WINRES(r.device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, r.commandAllocator.Get(),
        r.pipelineState.Get(), IID_PPV_ARGS(&r.commandList)));
    WINRES(r.commandList.Get()->Close()); // By default in recording state, so close now.


    // Using upload heaps for now, but not recommended.
    Vertex triangleVertices[] = {
        { { 0.0f, 0.25f * screenAR, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.25f, -0.25f * screenAR, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.25f, -0.25f * screenAR, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
    };
    const UINT vertexBufferSize = sizeof(triangleVertices);
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    WINRES(r.device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&r.vb))
    );
    UINT8* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
    WINRES(r.vb->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
    memcpy(vertexDataBegin, triangleVertices, sizeof(triangleVertices));
    r.vb->Unmap(0, nullptr);
    r.vbView.BufferLocation = r.vb->GetGPUVirtualAddress();
    r.vbView.StrideInBytes = sizeof(Vertex);
    r.vbView.SizeInBytes = vertexBufferSize;


    // Wait for assets to upload to the GPU.
    WINRES(r.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&r.fence)));
    r.fenceValue = 1;
    r.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (r.fenceEvent == nullptr) {
        WINRES(HRESULT_FROM_WIN32(GetLastError()));
    }
    waitForPreviousFrame(r);

    ShowWindow(windowHandle, true ? SW_NORMAL : SW_MAXIMIZE);
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(r.renderTargets[r.frameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(r.rtvHeap->GetCPUDescriptorHandleForHeapStart(), r.frameIdx, r.rtvDescriptorSize);
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        WINRES(r.commandAllocator->Reset());
        WINRES(r.commandList->Reset(r.commandAllocator.Get(), r.pipelineState.Get()));
        r.commandList->SetGraphicsRootSignature(r.rootSignature.Get());
        r.commandList->RSSetViewports(1, &r.viewport);
        r.commandList->RSSetScissorRects(1, &r.scissorRect);
        r.commandList->ResourceBarrier(1, &barrier); // Use back buffer as a render target.
        r.commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
        r.commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        r.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        r.commandList->IASetVertexBuffers(0, 1, &r.vbView);
        r.commandList->DrawInstanced(3, 1, 0, 0);
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(r.renderTargets[r.frameIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        r.commandList->ResourceBarrier(1, &barrier); // Use back buffer to present.
        WINRES(r.commandList->Close());
        ID3D12CommandList* cmdLists[] = { r.commandList.Get() };
        r.commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
        WINRES(r.swapChain->Present(1, 0));
        waitForPreviousFrame(r);
    }
    waitForPreviousFrame(r);
    CloseHandle(r.fenceEvent);
    DestroyWindow(windowHandle);
    return 1;
}












