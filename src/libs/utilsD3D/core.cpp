#include "core.h"
#include <utils/utils.h>
#include <d3dcompiler.h>
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

bool Factory::select() {
    size_t maxMemory = 0;
    for (auto& a : adapters) {
        if (a.desc.DedicatedVideoMemory > maxMemory) {
            maxMemory = a.desc.DedicatedVideoMemory;
            selectedAdapter = &a;
        }
    }
    return selectedAdapter != nullptr;
}

Adapter* Factory::getSelected() {
    return selectedAdapter;
}















bool Heap::init(ID3D12Device* device, const char* name, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT capacity) {
    this->name = name;
    this->capacity = capacity;
    size = device->GetDescriptorHandleIncrementSize(type);
    D3D12_DESCRIPTOR_HEAP_DESC desc = {
        .Type           = type,
        .NumDescriptors = capacity,
        .Flags          = flags,
        .NodeMask       = 0,
    };
    GUARDHR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&obj)));
    return true;
}

void Heap::reset() {
    currentIdx = 0;
}

bool Heap::next(int& idx) {
    if (currentIdx < capacity) {
        idx = currentIdx;
        currentIdx++;
        return true;
    } else {
        idx = -1;
        printf("Reached heap[\"%s\"] limit of %d\n", name, capacity);
        return false;
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE Heap::getCpu(UINT idx) {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(obj->GetCPUDescriptorHandleForHeapStart(), idx, size);
}


D3D12_GPU_DESCRIPTOR_HANDLE Heap::getGpu(UINT idx) {
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(obj->GetGPUDescriptorHandleForHeapStart(), idx, size);
}
ID3D12DescriptorHeap* Heap::get() {
    return obj.Get();
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
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    }

    if (rtvCount > 0) {
        GUARD(rtvHeap.init(obj.Get(), "RTV", D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, rtvCount));
    }
    if (dsvCount > 0) {
        GUARD(dsvHeap.init(obj.Get(), "DSV", D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, dsvCount));
    }
    if (cbvCount > 0) {
        GUARD(cbvHeap.init(obj.Get(), "CBV-SRV-UAV", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, cbvCount));
    }
    return true;
}

void Device::reset() {
    // rtvHeap.reset(); // TODO: cleanup reset procedure
    dsvHeap.reset();
    cbvHeap.reset();
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
















bool Queue::init(
    Device& device,
    D3D12_COMMAND_LIST_TYPE type,
    INT priority,
    D3D12_COMMAND_QUEUE_FLAGS flags,
    UINT nodeMask
) {
    D3D12_COMMAND_QUEUE_DESC desc = {
        .Type = type,
        .Priority = priority,
        .Flags = flags,
        .NodeMask = nodeMask
    };
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
        .Flags       = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING,
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

    for (UINT i = 0; i < frameCount; i++) {
        RenderTarget& renderTarget = renderTargets[i];
        
        int rtvIdx;
        GUARD(device.rtvHeap.next(rtvIdx));
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = device.rtvHeap.getCpu(rtvIdx);
        GUARDHR(obj->GetBuffer(i, IID_PPV_ARGS(&renderTarget.resource)));
        // D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {
        //     .Format         = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        //     .ViewDimension  = D3D12_RTV_DIMENSION_TEXTURE2D,
        //     .Texture2D = {
        //         .MipSlice   = 0,
        //         .PlaneSlice = 0,
        //     }
        // };
        // device.obj->CreateRenderTargetView(renderTarget.resource.Get(), &rtvDesc, rtvHandle);
        device.obj->CreateRenderTargetView(renderTarget.resource.Get(), nullptr, rtvHandle);
        renderTarget.view = rtvHandle;
    }
    return true;
}


RenderTarget Swapchain::next() {
    frameIdx = obj->GetCurrentBackBufferIndex();
    return renderTargets[frameIdx];
}

bool Swapchain::present(bool vsync) {
    if (vsync) {
        GUARDHR(obj->Present(1, 0));
    } else {
        GUARDHR(obj->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    }
    return true;
}










bool Shader::compile(std::wstring dir, std::wstring name, const char* entry, const char* target, uint32_t flags) {
    std::wstring path = getAssetsPathW() + L"\\" + name;
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

bool Shader::load(std::string name) {
    bool result = false;
    std::string path = getAssetsPath() + name + ".dxil";
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







ConstantBufferBase::~ConstantBufferBase() {
    if (obj != nullptr) {
        obj->Unmap(0, nullptr);
    }
    data = nullptr;
}

bool ConstantBufferBase::init(ID3D12Device* device, uint32_t elementCount, uint32_t elementSize) {
    this->device = device;
    this->elementCount = elementCount;
    this->elementSize = elementSize;
    this->elementSizePadded = align256(elementSize);
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(elementSizePadded * elementCount);
    HRESULT res = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&obj)
    );
    obj->Map(0, nullptr, reinterpret_cast<void**>(&data));
    return true;
}

ID3D12Resource* ConstantBufferBase::getRes() {
    return obj.Get();
}

void ConstantBufferBase::set(int idx, void* element) {
    memcpy(data + elementSizePadded * idx, element, elementSize);
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantBufferBase::getAddress(int idx) {
    return obj.Get()->GetGPUVirtualAddress() + idx * elementSizePadded;
}

D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBufferBase::getView(int idx) {
    return {
        .BufferLocation = obj->GetGPUVirtualAddress() + idx * elementSizePadded,
        .SizeInBytes = elementSizePadded,
    };
}





}