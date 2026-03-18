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

bool Swapchain::present(bool vsync) {
    if (vsync) {
        GUARDHR(obj->Present(1, 0));
    } else {
        GUARDHR(obj->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    }
    return true;
}



























bool FrameControl::init(Device& device, Queue* queue, int frameCount) {
    this->queue = queue;
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

bool FrameControl::execute() {
    Frame& frame = frames[frameIdx];
    HRESULT res = cmdList->Close();
    queue->execute({ cmdList.Get() });
    return true;
}

bool FrameControl::end() {
    Frame& frame = frames[frameIdx];
    GUARD(queue->signal(frame.fenceValue));
    frameCounter++;
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

Mesh& MeshControl::getMesh(int idx) {
    return meshes[idx];
}

















bool RootSig::initVoid(Device& device) {
    ComPtr<ID3DBlob>            sig;
    ComPtr<ID3DBlob>            error;
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    GUARDHR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
    GUARDHR(device.obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&obj)));
    return true;
}

bool RootSig::init1Cbv(Device& device) {
    ComPtr<ID3DBlob>            sig;
    ComPtr<ID3DBlob>            error;

    CD3DX12_ROOT_PARAMETER rootParam[1];
    rootParam[0].InitAsConstantBufferView(0);
    
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(1, rootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    GUARDHR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
    GUARDHR(device.obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&obj)));
    return true;
}

bool RootSig::initStd(Device& device) {
    ComPtr<ID3DBlob>            sig;
    ComPtr<ID3DBlob>            error;

    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    CD3DX12_ROOT_PARAMETER rootParam[1];
    rootParam[0].InitAsDescriptorTable(1, &cbvTable);
    
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(1, rootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    GUARDHR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
    GUARDHR(device.obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&obj)));
    return true;
}




bool PipelineBasic::init(Device& device, RootSig& sig) {
    Shader vShader;
    Shader pShader;
    GUARD(vShader.load("shaders_v"));
    GUARD(pShader.load("shaders_p"));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        .pRootSignature         = sig.obj.Get(),
        .VS                     = vShader.bytecode,
        .PS                     = pShader.bytecode,
        .BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask             = UINT_MAX,
        .RasterizerState        = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        .DepthStencilState      = { .DepthEnable = FALSE, .StencilEnable = FALSE, },
        .InputLayout            = { inputElementDescs, _countof(inputElementDescs) },
        .PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets       = 1,
        .RTVFormats             = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .SampleDesc             = { .Count = 1},
    };
    GUARDHR(device.obj->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&obj)));
    return true;
}



bool PipelineFill::init(Device& device, RootSig& sig) {
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    Shader vShader;
    Shader pShader;
    GUARD(vShader.load("shaders_v"));
    GUARD(pShader.load("shaders_p")); 

    // D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels = {
    //     .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
    //     .SampleCount = 4,
    //     .Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
    //     .NumQualityLevels = 0,
    // };
    // GUARDHR(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels)));
    // GUARD((qualityLevels.NumQualityLevels > 0));
    // DXGI_SAMPLE_DESC sampleDesc = { .Count = 4, .Quality = qualityLevels.NumQualityLevels - 1 }; 
    DXGI_SAMPLE_DESC sampleDesc = { .Count = 1, .Quality = 0 };

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 1,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    D3D12_RASTERIZER_DESC fillRasterDesc = {
        .FillMode               = D3D12_FILL_MODE_SOLID,
        .CullMode               = D3D12_CULL_MODE_NONE,
        // .CullMode               = D3D12_CULL_MODE_BACK,
        .FrontCounterClockwise  = FALSE,
        .DepthBias              = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp         = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias   = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable        = TRUE,
        .MultisampleEnable      = FALSE,
        .AntialiasedLineEnable  = FALSE,
        .ForcedSampleCount      = 0,
        .ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };
    D3D12_DEPTH_STENCIL_DESC depthStateDesc = {
        .DepthEnable        = TRUE,
        .DepthWriteMask     = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc          = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .StencilEnable      = FALSE,
        // .StencilReadMask    = 0,
        // .StencilWriteMask   = 0,
        // .FrontFace = {
        //     .StencilFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilPassOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilFunc = D3D12_COMPARISON_FUNC_GREATER,
        // },
        // .BackFace = {
        //     .StencilFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilPassOp = D3D12_STENCIL_OP_REPLACE,
        //     .StencilFunc = D3D12_COMPARISON_FUNC_GREATER,
        // },

    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoFillDesc = {
        .pRootSignature         = sig.obj.Get(),
        .VS                     = vShader.bytecode,
        .PS                     = pShader.bytecode,
        .BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask             = UINT_MAX,
        .RasterizerState        = fillRasterDesc,
        .DepthStencilState      = depthStateDesc,
        .InputLayout            = { inputElementDescs, _countof(inputElementDescs) },
        .PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets       = 1,
        .RTVFormats             = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .DSVFormat              = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc             = sampleDesc,
    };
    GUARDHR(device.obj->CreateGraphicsPipelineState(&psoFillDesc, IID_PPV_ARGS(&obj)));
    return true;
}



bool PipelineWire::init(Device& device, RootSig& sig) {
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    Shader vShaderWire;
    Shader pShaderWire;
    GUARD(vShaderWire.load("shadersWire_v"));
    GUARD(pShaderWire.load("shadersWire_p"));

    DXGI_SAMPLE_DESC sampleDesc = { .Count = 1, .Quality = 0 };

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 1,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    D3D12_RASTERIZER_DESC wireRasterDesc = {
        .FillMode               = D3D12_FILL_MODE_WIREFRAME,
        .CullMode               = D3D12_CULL_MODE_NONE,
        .FrontCounterClockwise  = FALSE,
        .DepthBias              = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp         = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias   = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable        = TRUE,
        .MultisampleEnable      = FALSE,
        .AntialiasedLineEnable  = FALSE,
        .ForcedSampleCount      = 0,
        .ConservativeRaster     = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };
    D3D12_DEPTH_STENCIL_DESC depthStateDesc = {
        .DepthEnable        = TRUE,
        .DepthWriteMask     = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc          = D3D12_COMPARISON_FUNC_LESS_EQUAL,
        .StencilEnable      = FALSE,
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoWireDesc = {
        .pRootSignature         = sig.obj.Get(),
        .VS                     = vShaderWire.bytecode,
        .PS                     = pShaderWire.bytecode,
        .BlendState             = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        .SampleMask             = UINT_MAX,
        .RasterizerState        = wireRasterDesc,
        .DepthStencilState      = depthStateDesc,
        .InputLayout            = { inputElementDescs, _countof(inputElementDescs) },
        .PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets       = 1,
        .RTVFormats             = { DXGI_FORMAT_R8G8B8A8_UNORM },
        .DSVFormat              = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc             = sampleDesc,
    };
    GUARDHR(device.obj->CreateGraphicsPipelineState(&psoWireDesc, IID_PPV_ARGS(&obj)));
    return true;
};







bool DepthBuffer::init(Device& device, uint64_t width, uint32_t height) {
    DXGI_SAMPLE_DESC sampleDesc = { .Count = 1, .Quality = 0 };
    D3D12_RESOURCE_DESC depthDesc = {
        .Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment          = 0,
        .Width              = width,
        .Height             = height,
        .DepthOrArraySize   = 1,
        .MipLevels          = 1,
        .Format             = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc         = sampleDesc,
        .Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
    };
    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_CLEAR_VALUE optClear = {
        .Format = DXGI_FORMAT_D32_FLOAT,
        .DepthStencil = { .Depth = 1, .Stencil = 0 },
    };
    GUARDHR(device.obj->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        // D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optClear,
        IID_PPV_ARGS(obj.GetAddressOf())
    ));
    D3D12_DEPTH_STENCIL_VIEW_DESC depthViewDesc = {
        .Format         = DXGI_FORMAT_D32_FLOAT,
        .ViewDimension  = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags          = D3D12_DSV_FLAG_NONE,
        .Texture2D      = { .MipSlice = 0 },
    };
    device.obj->CreateDepthStencilView(obj.Get(), &depthViewDesc, device.dsvHeap->GetCPUDescriptorHandleForHeapStart());
    return true;
}

}