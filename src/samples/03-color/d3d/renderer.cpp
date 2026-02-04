#include <renderer/renderer.h>
#include <app/app.h>
#include <utils/utils.h>
#include <utilsD3D/utilsD3D.h>
#include <wrl.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>
#include <iostream>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace gpu {


const int frameCount = 2;



template<typename T>
class CBuffer {
public:
    CBuffer() {};

    ~CBuffer() {
        if (cb != nullptr) {
            cb->Unmap(0, nullptr);
        }
        data = nullptr;
    }

    void init(ID3D12Device* device, uint32_t elementCount) {
        this->device = device;
        this->elementCount = elementCount;
        this->elementSize = align256(sizeof(T));
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(elementSize * elementCount);
        HRESULT res = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&cb)
        );
        cb->Map(0, nullptr, reinterpret_cast<void**>(&data));
        this->device = device;
    }

    void set(int idx, T& element) {
        memcpy(data + elementSize * idx, &element, sizeof(T));
    }

    D3D12_CONSTANT_BUFFER_VIEW_DESC getBufferViewDesc(int idx) {
        return {
            .BufferLocation = cb->GetGPUVirtualAddress() + idx * elementSize,
            .SizeInBytes = elementSize,
        };
    }

    ID3D12Resource* get() {
        return cb.Get();
    }
    
private:
    ID3D12Device* device;
    ComPtr<ID3D12Resource> cb;
    uint8_t* data;
    uint32_t elementCount;
    uint32_t elementSize;
};

struct ObjectData {
    XMFLOAT4X4 mvp;
    float v[4];
};


struct Buffer {
    ComPtr<ID3D12Resource>      vbUp;
};

struct Mesh {
    int                         bufferId;
    size_t                      vCount;
    D3D12_INDEX_BUFFER_VIEW     indicesView;
    D3D12_VERTEX_BUFFER_VIEW    positionView;
    D3D12_VERTEX_BUFFER_VIEW    colorView;
};



class RendererD3D : public IRenderer {
public:

    int start(void* window, uint32_t screenWidth, uint32_t screenHeight) {
        auto hwnd = static_cast<HWND>(window);
        screenAR = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
        viewport = { 0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0, 1 };
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
        GUARDHR(cmdAllocator->Reset());
        GUARDHR(cmdList->Reset(cmdAllocator.Get(), nullptr));

        GUARDHR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent == nullptr) {
            GUARDHR(HRESULT_FROM_WIN32(GetLastError()));
        }
        fenceValue = 1;
        rtvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        dsvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        cbvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

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



        ////////////////////////////////////////////////////////////////////////////////////////////
        // SWAPCHAIN
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width       = screenWidth,
            .Height      = screenHeight,
            .Format      = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc  = { .Count = 1, .Quality = 0 },
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
            .NodeMask       = 0,
        };
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            .NumDescriptors = 1,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            .NodeMask       = 0,
        };
        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = 100,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            .NodeMask       = 0,
        };
        GUARDHR(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
        GUARDHR(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap)));
        GUARDHR(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));

        for (int i = 0; i < frameCount; i++) {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), i, rtvDescSize);
            GUARDHR(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
            device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
        }
        objectCBuffers.init(device.Get(), 100);
        for (int i = 0; i < 100; i++) {
            D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = objectCBuffers.getBufferViewDesc(i);
            CD3DX12_CPU_DESCRIPTOR_HANDLE cbvView(cbvHeap->GetCPUDescriptorHandleForHeapStart(), i, cbvDescSize);
            device->CreateConstantBufferView(&viewDesc, cbvView);
        }


        ////////////////////////////////////////////////////////////////////////////////////////////
        // Depth
        D3D12_RESOURCE_DESC depthDesc = {
            .Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            .Alignment          = 0,
            .Width              = screenWidth,
            .Height             = screenHeight,
            .DepthOrArraySize   = 1,
            .MipLevels          = 1,
            .Format             = DXGI_FORMAT_D32_FLOAT,
            .SampleDesc         = sampleDesc,
            .Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        };
        D3D12_CLEAR_VALUE optClear = {
            .Format = DXGI_FORMAT_D32_FLOAT,
            .DepthStencil = { .Depth = 1, .Stencil = 0 },
        };
        CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
        GUARDHR(device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &depthDesc,
            D3D12_RESOURCE_STATE_COMMON,
            &optClear,
            IID_PPV_ARGS(depthTarget.GetAddressOf())
        ));
        auto barrDepth = CD3DX12_RESOURCE_BARRIER::Transition(depthTarget.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        ID3D12CommandList* cmdLists[] = { cmdList.Get() };
        cmdList->ResourceBarrier(1, &barrDepth);
        GUARDHR(cmdList.Get()->Close());
        cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
        GUARD(waitForPreviousFrame());

        D3D12_DEPTH_STENCIL_VIEW_DESC depthViewDesc = {
            .Format         = DXGI_FORMAT_D32_FLOAT,
            .ViewDimension  = D3D12_DSV_DIMENSION_TEXTURE2D,
            .Flags          = D3D12_DSV_FLAG_NONE,
            .Texture2D      = { .MipSlice = 0 },
        };
        device->CreateDepthStencilView(depthTarget.Get(), &depthViewDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());


        ////////////////////////////////////////////////////////////////////////////////////////////
        // PSOs
        ComPtr<ID3DBlob>            sig;
        ComPtr<ID3DBlob>            error;

        CD3DX12_DESCRIPTOR_RANGE cbvTable;
        cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
        CD3DX12_ROOT_PARAMETER rootParam[1];
        rootParam[0].InitAsDescriptorTable(1, &cbvTable);


        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(1, rootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        GUARDHR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error));
        GUARDHR(device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
        

        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        Shader vShader;
        Shader pShader;
        Shader vShaderWire;
        Shader pShaderWire;
        std::string shaderDir = "02-color-shaders";
        GUARD(loadShader(vShader,     shaderDir, "shaders_v.dxil"));
        GUARD(loadShader(pShader,     shaderDir, "shaders_p.dxil"));
        GUARD(loadShader(vShaderWire, shaderDir, "shadersWire_v.dxil"));
        GUARD(loadShader(pShaderWire, shaderDir, "shadersWire_p.dxil"));
        // GUARD(compileShader(vShader,     shaderDir, L"shaders.hlsl",     "VSMain", "vs_5_0", compileFlags));
        // GUARD(compileShader(pShader,     shaderDir, L"shaders.hlsl",     "PSMain", "ps_5_0", compileFlags));
        // GUARD(compileShader(vShaderWire, shaderDir, L"shadersWire.hlsl", "VSMain", "vs_5_0", compileFlags));
        // GUARD(compileShader(pShaderWire, shaderDir, L"shadersWire.hlsl", "PSMain", "ps_5_0", compileFlags));

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
            .pRootSignature         = rootSignature.Get(),
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
        GUARDHR(device->CreateGraphicsPipelineState(&psoFillDesc, IID_PPV_ARGS(&psoFill)));

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
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoWireDesc = {
            .pRootSignature         = rootSignature.Get(),
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
        GUARDHR(device->CreateGraphicsPipelineState(&psoWireDesc, IID_PPV_ARGS(&psoWire)));

        return 1;
    }

    void stop() {
        waitForPreviousFrame();
        CloseHandle(fenceEvent);
    }

    void setView(ViewDesc& desc) {
        XMVECTOR pos    = XMVectorSet(desc.pos[0],    desc.pos[1],    desc.pos[2],    1.0f);
        XMVECTOR target = XMVectorSet(desc.target[0], desc.target[1], desc.target[2], 1.0f);
        XMVECTOR up     = XMVectorSet(desc.up[0],     desc.up[1],     desc.up[2],     1.0f);
        XMMATRIX view   = XMMatrixLookAtLH(pos, target, up);
        XMStoreFloat4x4(&viewMat, view);
    }
    
    void setProjection(ProjectionDesc& desc) {
        XMMATRIX proj = XMMatrixPerspectiveFovLH(desc.fovAngleY, desc.aspectRatio, desc.nearZ, desc.farZ);
        XMStoreFloat4x4(&projMat, proj);
    }

    int render(const Color& clearColor, const std::vector<RenderItem>& items) {
        D3D12_CPU_DESCRIPTOR_HANDLE depthView = dsvHeap->GetCPUDescriptorHandleForHeapStart();
        auto barr0 = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        auto barr1 = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIdx, rtvDescSize);
        ID3D12CommandList* cmdLists[] = { cmdList.Get() };
        ID3D12DescriptorHeap* heaps[] = { cbvHeap.Get() };

        GUARDHR(cmdAllocator->Reset());
        GUARDHR(cmdList->Reset(cmdAllocator.Get(), nullptr));
        cmdList->RSSetViewports(1, &viewport);
        cmdList->RSSetScissorRects(1, &scissorRect);
        cmdList->ResourceBarrier(1, &barr0); // Use back buffer as a render target.
        cmdList->OMSetRenderTargets(1, &rtvHandle, 1, &depthView);
        cmdList->ClearRenderTargetView(rtvHandle, clearColor.v, 0, nullptr);
        cmdList->ClearDepthStencilView(depthView, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
        cmdList->SetDescriptorHeaps(1, heaps);

        
        for (int i = 0; i < items.size(); i++) {
            const RenderItem& item = items[i];
            int meshId = item.meshId;
            Mesh& m = meshes[meshId];
            ObjectData objectData = {
                {},
                {1, 0, 0, 1},
            };
            
            XMVECTOR q = XMVectorSet(item.rotation[0], item.rotation[1], -item.rotation[2], -item.rotation[3]);
            q = XMQuaternionNormalize(q);
            XMMATRIX S = XMMatrixScaling(item.scale[0], item.scale[1], -item.scale[2]);
            XMMATRIX R = XMMatrixRotationQuaternion(q);
            XMMATRIX T = XMMatrixTranslation(item.position[0], item.position[1], -item.position[2]);
            XMMATRIX model = S * R * T;
            
            XMMATRIX view = XMLoadFloat4x4(&viewMat);
            XMMATRIX proj = XMLoadFloat4x4(&projMat);
            XMMATRIX mat = model * view * proj;
            // XMMATRIX mat = XMMatrixIdentity();

            XMStoreFloat4x4(&objectData.mvp, mat);
            // XMStoreFloat4x4(&objectData.mvp, XMMatrixTranspose(mvp)));
            objectCBuffers.set(i, objectData);
        }

        if (fillMode == Fill || fillMode == FillWire) {
            cmdList->SetPipelineState(psoFill.Get());
            cmdList->SetGraphicsRootSignature(rootSignature.Get());
            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            for (int i = 0; i < items.size(); i++) {
                const RenderItem& item = items[i];
                int meshId = item.meshId;
                Mesh& m = meshes[meshId];
                CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(cbvHeap->GetGPUDescriptorHandleForHeapStart(), i, cbvDescSize);
                cmdList->SetGraphicsRootDescriptorTable(0, cbv);
                cmdList->IASetIndexBuffer(&m.indicesView);
                cmdList->IASetVertexBuffers(0, 1, &m.positionView);
                cmdList->IASetVertexBuffers(1, 1, &m.colorView);
                cmdList->DrawIndexedInstanced(m.vCount, 1, 0, 0, 0);
            }
        }

        if (fillMode == Wire || fillMode == FillWire) {
            cmdList->SetPipelineState(psoWire.Get());
            cmdList->SetGraphicsRootSignature(rootSignature.Get());
            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            for (int i = 0; i < items.size(); i++) {
                int meshId = items[i].meshId;
                Mesh& m = meshes[meshId];
                CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(cbvHeap->GetGPUDescriptorHandleForHeapStart(), i, cbvDescSize);
                cmdList->SetGraphicsRootDescriptorTable(0, cbv);
                cmdList->IASetIndexBuffer(&m.indicesView);
                cmdList->IASetVertexBuffers(0, 1, &m.positionView);
                cmdList->IASetVertexBuffers(1, 1, &m.colorView);
                cmdList->DrawIndexedInstanced(m.vCount, 1, 0, 0, 0);
            }
        }


        cmdList->ResourceBarrier(1, &barr1); // Use back buffer to present.
        GUARDHR(cmdList->Close());
        cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
        GUARDHR(swapChain->Present(1, 0));
        GUARD(waitForPreviousFrame());
        return 1;
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

    int addBuffer(const BufferDesc& desc) {
        buffers.push_back(Buffer());
        Buffer& b = buffers.back();

        HRESULT res;
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.size);
        res = device->CreateCommittedResource(
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
        res = b.vbUp->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin));
        if (FAILED(res)) {
            return false;
        }
        memcpy(vertexDataBegin, desc.data + desc.offset, desc.size);
        b.vbUp->Unmap(0, nullptr);
        waitForPreviousFrame();
        return buffers.size() - 1;
    }
    
    int addMesh(const MeshDesc& desc) {
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

    
    void setFillMode(FillMode mode) {
        fillMode = mode;
    }

    void printErrors() {
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
    UINT                                dsvDescSize = 0;
    UINT                                cbvDescSize = 0;
    ComPtr<ID3D12DescriptorHeap>        rtvHeap;
    ComPtr<ID3D12DescriptorHeap>        dsvHeap;
    ComPtr<ID3D12DescriptorHeap>        cbvHeap;
    ComPtr<IDXGISwapChain3>             swapChain;
    ComPtr<ID3D12Resource>              renderTargets[frameCount];
    ComPtr<ID3D12Resource>              depthTarget;
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         psoFill;
    ComPtr<ID3D12PipelineState>         psoWire;
    D3D12_VIEWPORT                      viewport;
    D3D12_RECT                          scissorRect;
    UINT                                frameIdx;
    float                               screenAR;
    FillMode                            fillMode = Fill;
    std::vector<Buffer>                 buffers;
    std::vector<Mesh>                   meshes;
    CBuffer<ObjectData>                 objectCBuffers;
    XMFLOAT4X4                          viewMat;
    XMFLOAT4X4                          projMat;
};

std::unique_ptr<IRenderer> createRenderer() {
    return std::make_unique<RendererD3D>();
}
}

