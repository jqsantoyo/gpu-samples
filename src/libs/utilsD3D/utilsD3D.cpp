#include "utilsD3D.h"
#include <utils/utils.h>
using namespace DirectX;
using namespace Microsoft::WRL;


namespace gpu {


bool Factory::init() {
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    GUARDHR(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&obj)));

    int i = 0;
    IDXGIAdapter* adapter = nullptr;
    while(obj->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
        adapters.push_back({});
        Adapter& a = adapters.back();

        a.obj = adapter;
        adapter->GetDesc(&a.desc);
        
        int j = 0;
        IDXGIOutput* output = nullptr;
        while(adapter->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND) {
            a.outputs.push_back({});
            Output& o = a.outputs.back();

            o.obj = output;
            output->GetDesc(&o.desc);
            UINT modeCount = 0;
            output->GetDisplayModeList(format, 0, &modeCount, nullptr);
            o.modes.resize(modeCount);
            output->GetDisplayModeList(format, 0, &modeCount, &o.modes[0]);
            j++;
        }
        i++;
    }
    return true;
}

void Factory::print() {
    printf("\nAdapters: %zu\n", adapters.size());
    for (auto& a : adapters) {
        printf("Adapter\n");
        printf("  Description: %ls\n",              a.desc.Description);
        printf("  VendorId: %d\n",                  a.desc.VendorId);
        printf("  DeviceId: %d\n",                  a.desc.DeviceId);
        printf("  SubSysId: %d\n",                  a.desc.SubSysId);
        printf("  Revision: %d\n",                  a.desc.Revision);
        printf("  DedicatedVideoMemory: %zu\n",     a.desc.DedicatedVideoMemory);
        printf("  DedicatedSystemMemory: %zu\n",    a.desc.DedicatedSystemMemory);
        printf("  SharedSystemMemory: %zu\n",       a.desc.SharedSystemMemory);
        for (auto& o : a.outputs) {
            printf("    Output: %ls\n", o.desc.DeviceName);
            for (auto m : o.modes) {
                printf("      resolution: %d x %d, refresh: %d / %d\n", m.Width, m.Height, m.RefreshRate.Numerator, m.RefreshRate.Denominator);
            }
        }
    }
}

Adapter* Factory::select() {
    Adapter* selectedAdapter = nullptr;
    size_t maxMemory = 0;
    for (auto& a : adapters) {
        if (a.desc.DedicatedVideoMemory > maxMemory) {
            maxMemory = a.desc.DedicatedVideoMemory;
            selectedAdapter = &a;
        }
    }
    return selectedAdapter;
}



























bool Device::init(Adapter* adapter, UINT rtvCount, UINT dsvCount, UINT cbvCount) {
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        printf("Debug layer enabled\n");
        debugController->EnableDebugLayer();
    }
    // ComPtr<ID3D12Debug1> debugController1;
    // if (SUCCEEDED(debugController.As(&debugController1))) {
    //     debugController1->SetEnableGPUBasedValidation(TRUE);
    // }
    GUARDHR(D3D12CreateDevice(adapter->obj, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&obj)));
    if (SUCCEEDED(obj.As(&iq))) {
        // iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        // iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        // iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    }
    
    rtvDescSize = obj->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescSize = obj->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    cbvDescSize = obj->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = rtvCount,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask       = 0,
    };
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        .NumDescriptors = dsvCount,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask       = 0,
    };
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {
        .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = cbvCount,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask       = 0,
    };
    if (rtvCount > 0) {
        GUARDHR(obj->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
    }
    if (dsvCount > 0) {
        GUARDHR(obj->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));
    }
    if (cbvCount > 0) {
        GUARDHR(obj->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap)));
    }
    return true;
}


void Device::printErrors() {
    UINT64 count = iq->GetNumStoredMessages();
    printf("D3D12: messages in queue = %llu\n", count);
    for (UINT64 i = 0; i < count; ++i) {
        SIZE_T size = 0;
        iq->GetMessage(i, nullptr, &size);
        auto* msg = (D3D12_MESSAGE*)malloc(size);
        iq->GetMessage(i, msg, &size);
        printf("D3D12 MSG: %s\n", msg->pDescription);
        free(msg);
    }
    __debugbreak();
}












bool Queue::init(Device& device, D3D12_COMMAND_QUEUE_DESC desc) {
    GUARDHR(device.obj->CreateCommandQueue(&desc, IID_PPV_ARGS(&obj)));
    GUARDHR(device.obj->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr) {
        GUARDHR(HRESULT_FROM_WIN32(GetLastError()));
    }
    return true;
}

void Queue::terminate() {
    CloseHandle(fenceEvent);
}

void Queue::execute(std::initializer_list<ID3D12CommandList*> cmdLists) {
    obj->ExecuteCommandLists(cmdLists.size(), cmdLists.begin());
}

bool Queue::signal(UINT64& value) {
    nextFenceValue++;
    value = nextFenceValue;
    GUARDHR(obj->Signal(fence.Get(), nextFenceValue));
    return true;
}

bool Queue::wait(UINT64 value) {
    if (fence->GetCompletedValue() < value) {
        GUARDHR(fence->SetEventOnCompletion(value, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }
    return true;
}

bool Queue::wait() {
    UINT64 value;
    GUARD(signal(value));
    GUARD(wait(nextFenceValue));
    return true;
}


















bool Swapchain::init(Factory& factory, Device& device, Queue& queue, HWND hwnd, uint32_t w, uint32_t h, UINT frameCount) {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
        .Width       = w,
        .Height      = h,
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
    GUARDHR(factory.obj->CreateSwapChainForHwnd(queue.obj.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
    GUARDHR(swapChain1.As(&obj));
    GUARDHR(factory.obj->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    frameIdx = obj->GetCurrentBackBufferIndex();

    renderTargets.resize(frameCount);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(device.rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < frameCount; i++) {
        RenderTarget& renderTarget = renderTargets[i];
        GUARDHR(obj->GetBuffer(i, IID_PPV_ARGS(&renderTarget.resource)));
        device.obj->CreateRenderTargetView(renderTarget.resource.Get(), nullptr, rtvHandle);
        renderTarget.view = rtvHandle;
        rtvHandle.Offset(1, device.rtvDescSize);
    }
    return true;
}


RenderTarget Swapchain::next() {
    frameIdx = obj->GetCurrentBackBufferIndex();
    return renderTargets[frameIdx];
}

bool Swapchain::present() {
    GUARDHR(obj->Present(1, 0));
    return true;
}



























bool FrameControl::init(Device& device, Queue* queue, Swapchain* swapchain, int frameCount) {
    this->queue = queue;
    this->swapchain = swapchain;
    frames.resize(frameCount);
    for (auto& frame : frames) {
        GUARDHR(device.obj->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.cmdAllocator)));
        frame.fenceValue = 0;
    }
    GUARDHR(device.obj->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames[0].cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)));
    GUARDHR(cmdList.Get()->Close());
    return true;
}

bool FrameControl::begin(ID3D12PipelineState* initialState) {
    frameIdx = frameCounter % frames.size();
    Frame& frame = frames[frameIdx];
    queue->wait(frame.fenceValue);
    GUARDHR(frame.cmdAllocator->Reset());
    GUARDHR(cmdList->Reset(frame.cmdAllocator.Get(), initialState));
    return true;
}

bool FrameControl::end() {
    Frame& frame = frames[frameIdx];
    GUARDHR(cmdList->Close());
    queue->execute({ cmdList.Get() });
    GUARD(swapchain->present());
    GUARD(queue->signal(frame.fenceValue));
    frameCounter++;
    return true;
}

























bool Shader::compile(std::wstring dir, std::wstring name, const char* entry, const char* target, uint32_t flags) {
    std::wstring path = getAssetsPathW() + dir + L"\\" + name;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT res = D3DCompileFromFile(path.c_str(), nullptr, nullptr, entry, target, flags, 0, &blob, &errorBlob);
    if (FAILED(res)) {
        if (errorBlob) {
            const char* msg = static_cast<const char*>(errorBlob->GetBufferPointer());
            printf("%s\n", msg);
        } else {
            printf("D3DCompileFromFile[%ls]: %ld\n", path.c_str(), res);
        }
        return false;
    }
    bytecode = { blob->GetBufferPointer(), blob->GetBufferSize() };
    return true;
}

bool Shader::load(std::string dir, std::string name) {
    bool result = false;
    std::string path = getAssetsPath() + dir + "\\" + name;
    FILE* file;
    errno_t err = fopen_s(&file, path.c_str(), "rb");
    if (err != 0 || file == nullptr) {
        return result;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    HRESULT res = D3DCreateBlob(size, blob.GetAddressOf());
    if (SUCCEEDED(res)) {
        fread(blob->GetBufferPointer(), size, 1, file);
        bytecode = { blob->GetBufferPointer(), blob->GetBufferSize() };
        result = true;
    }
    fclose(file);
    return result;
}

















bool MeshControl::init() {
    return true;
}

int MeshControl::addBuffer(Device& device, const BufferDesc& desc) {
    buffers.push_back(Buffer());
    Buffer& b = buffers.back();

    HRESULT res;
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.size);
    res = device.obj->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&b.vbUp)
    );
    if (FAILED(res)) {
        return false;
    }

    UINT8* vertexDataBegin;
    CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
    GUARDHR(b.vbUp->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
    memcpy(vertexDataBegin, desc.data + desc.offset, desc.size);
    b.vbUp->Unmap(0, nullptr);
    return buffers.size() - 1;
}

int MeshControl::addMesh(const MeshDesc& desc) {
    Buffer& buffer = buffers[desc.bufferId];
    meshes.push_back(Mesh());
    Mesh& m = meshes.back();
    m = {
        .bufferId = desc.bufferId,
        .vCount = desc.vCount,
        .indicesView = {
            .BufferLocation = buffer.vbUp->GetGPUVirtualAddress() + desc.indices.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.indices.size),
            .Format = DXGI_FORMAT_R16_UINT,
        },
        .positionView = {
            .BufferLocation = buffer.vbUp->GetGPUVirtualAddress() + desc.position.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.position.size),
            .StrideInBytes = sizeof(float) * 3,
        },
        .colorView = {
            .BufferLocation = buffer.vbUp->GetGPUVirtualAddress() + desc.color.offset,
            .SizeInBytes = static_cast<uint32_t>(desc.color.size),
            .StrideInBytes = sizeof(float) * 3,
        },
    };
    return meshes.size() - 1;
}


}