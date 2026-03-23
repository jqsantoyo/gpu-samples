#include <utils/utils.h>
#include <utils/app.h>
#include <utils/renderer.h>
#include <utilsD3D/utilsD3D.h>
#include <pix3.h>
#include <wrl.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>
#include <iostream>

using namespace DirectX;
using namespace Microsoft::WRL;
namespace gpu {


struct ObjectData {
    XMFLOAT4X4 mvp;
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
        GUARD(factory.select());
        GUARD(device.init(factory.getSelected(), frameCount, 1, 100));
        GUARD(queue.init(device, queueDesc));
        GUARD(swapchain.init(factory, device, queue, hwnd, width, height, frameCount));
        GUARD(frameControl.init(device, &queue, 1));
        GUARD(rootSignature.init1Cbv1TableNSamplers(device));
        GUARD(pso.init(device, rootSignature));
        GUARD(psoWire.init(device, rootSignature));
        GUARD(depthBuffer.init(device, width, height));
        GUARD(textures.init(&device, &queue));
        objectConstants.init(device.obj.Get(), 100);

        int texIdx = textures.addTexture("crate.dds");

        queue.wait();



        return true;
    }

    void terminate() {
        queue.wait();
    }

    bool resize(int width, int height) {
        return 1;
    }
    
    void setView(vec3 pos, vec3 target, vec3 up) {
        XMVECTOR posVec    = XMVectorSet(pos.x,    pos.y,    pos.z,    1.0f);
        XMVECTOR targetVec = XMVectorSet(target.x, target.y, target.z, 1.0f);
        XMVECTOR upVec     = XMVectorSet(up.x,     up.y,     up.z,     1.0f);
        XMMATRIX view   = XMMatrixLookAtLH(posVec, targetVec, upVec);
        XMStoreFloat4x4(&viewMat, view);
    }
    
    void setProjection(float fovY, float aspect, float nearZ, float farZ) {
        XMMATRIX proj = XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
        XMStoreFloat4x4(&projMat, proj);
    }

    bool render(const Color& clearColor, const std::vector<RenderItem>& items) {
        static uint64_t frameIdx = 0;
        PIXBeginEvent(PIX_COLOR_DEFAULT, "Render %llu", frameIdx);
        RenderTarget target = swapchain.next();
        D3D12_CPU_DESCRIPTOR_HANDLE depthView = device.dsvHeap->GetCPUDescriptorHandleForHeapStart();
        auto barr0 = CD3DX12_RESOURCE_BARRIER::Transition(target.resource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        auto barr1 = CD3DX12_RESOURCE_BARRIER::Transition(target.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        ID3D12DescriptorHeap* heaps[] = { device.cbvHeap.Get() };

        frameControl.begin(nullptr);
        frameControl.cmdList->RSSetViewports(1, &viewport);
        frameControl.cmdList->RSSetScissorRects(1, &scissorRect);
        frameControl.cmdList->ResourceBarrier(1, &barr0); // Use back buffer as a render target.
        frameControl.cmdList->OMSetRenderTargets(1, &target.view, 1, &depthView);
        frameControl.cmdList->ClearRenderTargetView(target.view, clearColor.v, 0, nullptr);
        frameControl.cmdList->ClearDepthStencilView(depthView, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
        frameControl.cmdList->SetDescriptorHeaps(1, heaps);

        
        for (int i = 0; i < items.size(); i++) {
            const RenderItem& item = items[i];
            int meshId = item.meshId;
            Mesh& m = meshControl.getMesh(meshId);
    
            XMVECTOR q = XMVectorSet(item.rotation[0], item.rotation[1], -item.rotation[2], -item.rotation[3]);
            q = XMQuaternionNormalize(q);
            XMMATRIX S = XMMatrixScaling(item.scale[0], item.scale[1], -item.scale[2]);
            XMMATRIX R = XMMatrixRotationQuaternion(q);
            XMMATRIX T = XMMatrixTranslation(item.position[0], item.position[1], -item.position[2]);
            XMMATRIX model = S * R * T;

            XMMATRIX view = XMLoadFloat4x4(&viewMat);
            XMMATRIX proj = XMLoadFloat4x4(&projMat);
            XMMATRIX mat = model * view * proj;

            ObjectData objectData = {};
            XMStoreFloat4x4(&objectData.mvp, mat);
            // XMStoreFloat4x4(&objectData.mvp, XMMatrixTranspose(mvp)));
            objectConstants.set(i, objectData);
        }

        if (fillMode == Fill || fillMode == FillWire) {
            frameControl.cmdList->SetPipelineState(pso.obj.Get());
            frameControl.cmdList->SetGraphicsRootSignature(rootSignature.obj.Get());
            frameControl.cmdList->SetGraphicsRootDescriptorTable(1, device.cbvHeap->GetGPUDescriptorHandleForHeapStart());
            frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            for (int i = 0; i < items.size(); i++) {
                const RenderItem& item = items[i];
                int meshId = item.meshId;
                Mesh& m = meshControl.getMesh(meshId);
                D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = objectConstants.getAddress(i);
                frameControl.cmdList->SetGraphicsRootConstantBufferView(0, cbvAddress);
                frameControl.cmdList->IASetIndexBuffer(&m.indicesView);
                frameControl.cmdList->IASetVertexBuffers(0, 1, &m.positionView);
                frameControl.cmdList->IASetVertexBuffers(1, 1, &m.uvView);
                frameControl.cmdList->DrawIndexedInstanced(m.vCount, 1, 0, 0, 0);
            }
        }

        if (fillMode == Wire || fillMode == FillWire) {
            // PIXBeginEvent(queue.obj.Get(), PIX_COLOR_DEFAULT, "Wire %llu", frameIdx);
            frameControl.cmdList->SetPipelineState(psoWire.obj.Get());
            frameControl.cmdList->SetGraphicsRootSignature(rootSignature.obj.Get());
            frameControl.cmdList->SetGraphicsRootDescriptorTable(1, device.cbvHeap->GetGPUDescriptorHandleForHeapStart());
            frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            for (int i = 0; i < items.size(); i++) {
                int meshId = items[i].meshId;
                Mesh& m = meshControl.getMesh(meshId);
                D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = objectConstants.getAddress(i);
                frameControl.cmdList->SetGraphicsRootConstantBufferView(0, cbvAddress);
                frameControl.cmdList->IASetIndexBuffer(&m.indicesView);
                frameControl.cmdList->IASetVertexBuffers(0, 1, &m.positionView);
                // frameControl.cmdList->IASetVertexBuffers(1, 1, &m.colorView);
                frameControl.cmdList->DrawIndexedInstanced(m.vCount, 1, 0, 0, 0);
                // device.printErrors();
            }
            // PIXEndEvent(queue.obj.Get());
        }


        frameControl.cmdList->ResourceBarrier(1, &barr1); // Use back buffer to present.
        GUARD(frameControl.execute());
        GUARD(swapchain.present(false));
        GUARD(frameControl.end());
        PIXEndEvent();
        frameIdx++;
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
    DepthBuffer                         depthBuffer;
    Queue                               queue;
    FrameControl                        frameControl;
    MeshControl                         meshControl;
    TextureRegistry                     textures;
    RootSig                             rootSignature;
    PipelineTex                         pso;
    PipelineWire                        psoWire;
    D3D12_VIEWPORT                      viewport;
    D3D12_RECT                          scissorRect;
    float                               screenAR;
    FillMode                            fillMode = Fill;
    ConstantBuffer<ObjectData>          objectConstants;
    XMFLOAT4X4                          viewMat;
    XMFLOAT4X4                          projMat;
};

std::unique_ptr<IRenderer> createRendererD3D() {
    return std::make_unique<RendererD3D>();
}
}