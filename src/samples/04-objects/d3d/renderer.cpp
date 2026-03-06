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



struct ObjectData {
    XMFLOAT4X4 mvp;
    float v[4];
};




class RendererD3D : public IRenderer {
public:

    bool init(void* window, uint32_t width, uint32_t height) {
        auto hwnd = static_cast<HWND>(window);
        screenAR = static_cast<float>(width) / static_cast<float>(height);
        viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0, 1 };
        scissorRect = { 0, 0, long(width), long(height) };
        UINT frameCount = 2;
        D3D12_COMMAND_QUEUE_DESC queueDesc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
        };

        GUARD(factory.init());
        Adapter* adapter = factory.select();
        GUARD(adapter != nullptr);
        GUARD(device.init(adapter, frameCount, 0, 100));
        GUARD(queue.init(device, queueDesc));
        GUARD(swapchain.init(factory, device, queue, hwnd, width, height, frameCount));
        GUARD(frameControl.init(device, &queue, 1));

        

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




        objectCBuffers.init(device.obj.Get(), 100); 
        for (int i = 0; i < 100; i++) {
            D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = objectCBuffers.getBufferViewDesc(i);
            CD3DX12_CPU_DESCRIPTOR_HANDLE cbvView(device.cbvHeap->GetCPUDescriptorHandleForHeapStart(), i, device.cbvDescSize);
            device.obj->CreateConstantBufferView(&viewDesc, cbvView);
        }


        // ////////////////////////////////////////////////////////////////////////////////////////////
        // // Depth
        // D3D12_RESOURCE_DESC depthDesc = {
        //     .Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        //     .Alignment          = 0,
        //     .Width              = width,
        //     .Height             = height,
        //     .DepthOrArraySize   = 1,
        //     .MipLevels          = 1,
        //     .Format             = DXGI_FORMAT_D32_FLOAT,
        //     .SampleDesc         = sampleDesc,
        //     .Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        //     .Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        // };
        // D3D12_CLEAR_VALUE optClear = {
        //     .Format = DXGI_FORMAT_D32_FLOAT,
        //     .DepthStencil = { .Depth = 1, .Stencil = 0 },
        // };
        // CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);
        // GUARDHR(device->CreateCommittedResource(
        //     &defaultHeap,
        //     D3D12_HEAP_FLAG_NONE,
        //     &depthDesc,
        //     D3D12_RESOURCE_STATE_COMMON,
        //     &optClear,
        //     IID_PPV_ARGS(depthTarget.GetAddressOf())
        // ));
        // auto barrDepth = CD3DX12_RESOURCE_BARRIER::Transition(depthTarget.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // ID3D12CommandList* cmdLists[] = { cmdList.Get() };
        // cmdList->ResourceBarrier(1, &barrDepth);
        // GUARDHR(cmdList.Get()->Close());
        // cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
        // GUARD(waitForPreviousFrame());

        // D3D12_DEPTH_STENCIL_VIEW_DESC depthViewDesc = {
        //     .Format         = DXGI_FORMAT_D32_FLOAT,
        //     .ViewDimension  = D3D12_DSV_DIMENSION_TEXTURE2D,
        //     .Flags          = D3D12_DSV_FLAG_NONE,
        //     .Texture2D      = { .MipSlice = 0 },
        // };
        // device.obj->CreateDepthStencilView(depthTarget.Get(), &depthViewDesc, device.dsvHeap->GetCPUDescriptorHandleForHeapStart());


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
        GUARDHR(device.obj->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
        

        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        Shader vShader;
        Shader pShader;
        Shader vShaderWire;
        Shader pShaderWire;
        GUARD(vShader.load("shaders_v"));
        GUARD(pShader.load("shaders_p"));
        GUARD(vShaderWire.load("shadersWire_v"));
        GUARD(pShaderWire.load("shadersWire_p"));
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
        GUARDHR(device.obj->CreateGraphicsPipelineState(&psoFillDesc, IID_PPV_ARGS(&psoFill)));

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
        GUARDHR(device.obj->CreateGraphicsPipelineState(&psoWireDesc, IID_PPV_ARGS(&psoWire)));

        return 1;
    }

    void terminate() {
        queue.wait();
    }

    bool resize(int width, int height) {
        return 1;
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

    bool render(const Color& clearColor, const std::vector<RenderItem>& items) {
        RenderTarget target = swapchain.next();
        // D3D12_CPU_DESCRIPTOR_HANDLE depthView = device.dsvHeap->GetCPUDescriptorHandleForHeapStart();
        auto barr0 = CD3DX12_RESOURCE_BARRIER::Transition(target.resource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        auto barr1 = CD3DX12_RESOURCE_BARRIER::Transition(target.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        ID3D12DescriptorHeap* heaps[] = { device.cbvHeap.Get() };

        frameControl.begin(nullptr);
        frameControl.cmdList->RSSetViewports(1, &viewport);
        frameControl.cmdList->RSSetScissorRects(1, &scissorRect);
        frameControl.cmdList->ResourceBarrier(1, &barr0); // Use back buffer as a render target.
        frameControl.cmdList->OMSetRenderTargets(1, &target.view, 0, nullptr);//&depthView);
        frameControl.cmdList->ClearRenderTargetView(target.view, clearColor.v, 0, nullptr);
        // frameControl.cmdList->ClearDepthStencilView(depthView, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
        frameControl.cmdList->SetDescriptorHeaps(1, heaps);

        
        for (int i = 0; i < items.size(); i++) {
            const RenderItem& item = items[i];
            int meshId = item.meshId;
            Mesh& m = meshControl.getMesh(meshId);
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
            frameControl.cmdList->SetPipelineState(psoFill.Get());
            frameControl.cmdList->SetGraphicsRootSignature(rootSignature.Get());
            frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            for (int i = 0; i < items.size(); i++) {
                const RenderItem& item = items[i];
                int meshId = item.meshId;
                Mesh& m = meshControl.getMesh(meshId);
                CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(device.cbvHeap->GetGPUDescriptorHandleForHeapStart(), i, device.cbvDescSize);
                frameControl.cmdList->SetGraphicsRootDescriptorTable(0, cbv);
                frameControl.cmdList->IASetIndexBuffer(&m.indicesView);
                frameControl.cmdList->IASetVertexBuffers(0, 1, &m.positionView);
                frameControl.cmdList->IASetVertexBuffers(1, 1, &m.colorView);
                frameControl.cmdList->DrawIndexedInstanced(m.vCount, 1, 0, 0, 0);
            }
        }

        if (fillMode == Wire || fillMode == FillWire) {
            frameControl.cmdList->SetPipelineState(psoWire.Get());
            frameControl.cmdList->SetGraphicsRootSignature(rootSignature.Get());
            frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            for (int i = 0; i < items.size(); i++) {
                int meshId = items[i].meshId;
                Mesh& m = meshControl.getMesh(meshId);
                CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(device.cbvHeap->GetGPUDescriptorHandleForHeapStart(), i, device.cbvDescSize);
                frameControl.cmdList->SetGraphicsRootDescriptorTable(0, cbv);
                frameControl.cmdList->IASetIndexBuffer(&m.indicesView);
                frameControl.cmdList->IASetVertexBuffers(0, 1, &m.positionView);
                frameControl.cmdList->IASetVertexBuffers(1, 1, &m.colorView);
                frameControl.cmdList->DrawIndexedInstanced(m.vCount, 1, 0, 0, 0);
            }
        }


        frameControl.cmdList->ResourceBarrier(1, &barr1); // Use back buffer to present.
        GUARD(frameControl.execute());
        GUARD(swapchain.present());
        GUARD(frameControl.end());
        return 1;
    }
    

    int addBuffer(const BufferDesc& desc) {
        return meshControl.addBuffer(device, desc);
    }
    
    int addMesh(const MeshDesc& desc) {
        return meshControl.addMesh(desc);
    }

    
    void setFillMode(FillMode mode) {
        fillMode = mode;
    }

private:
    Factory                             factory;
    Device                              device;
    Swapchain                           swapchain;
    Queue                               queue;
    FrameControl                        frameControl;
    MeshControl                         meshControl;
    ComPtr<ID3D12Resource>              depthTarget;
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         psoFill;
    ComPtr<ID3D12PipelineState>         psoWire;
    D3D12_VIEWPORT                      viewport;
    D3D12_RECT                          scissorRect;
    float                               screenAR;
    FillMode                            fillMode = Fill;
    CBuffer<ObjectData>                 objectCBuffers;
    XMFLOAT4X4                          viewMat;
    XMFLOAT4X4                          projMat;
};

std::unique_ptr<IRenderer> createRenderer() {
    return std::make_unique<RendererD3D>();
}
}

// 563