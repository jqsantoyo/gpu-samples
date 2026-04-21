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
        viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0, 1 };
        scissorRect = { 0, 0, long(width), long(height) };
        UINT frameCount = 2;
        uint32_t objectMemory = sizeof(ObjectData);
        uint32_t alignedObjectMemory = align256(objectMemory);
        uint32_t maxFrameMemory = 100 * alignedObjectMemory;

        GUARD(factory.init());
        GUARD(factory.select());
        GUARD(device.init(factory.getSelected(), frameCount, 1, 100));
        GUARD(queue.init(device, D3D12_COMMAND_LIST_TYPE_DIRECT));
        GUARD(swapchain.init(factory, device, queue, hwnd, width, height, frameCount));
        GUARD(frameControl.init(device, &queue, 2, maxFrameMemory));
        GUARD(rootSignature.init1Cbv(device));
        GUARD(psoFill.init(device, rootSignature));
        GUARD(psoWire.init(device, rootSignature));
        GUARD(depthBuffer.init(&device, width, height));
        reset();
        return true;
    }

    void terminate() {
        queue.wait();
    }

    void reset() {
        wait();
        device.reset();
        depthBuffer.reset();
        meshRegistry.reset();
    }

    void wait() {
        queue.wait();
    }

    bool resize(int width, int height) {
        return 1;
    }

    bool render(const RenderView& view) {
        static uint64_t frameIdx = 0;
        PIXBeginEvent(PIX_COLOR_DEFAULT, "Render %llu", frameIdx);
        RenderTarget target = swapchain.next();
        D3D12_CPU_DESCRIPTOR_HANDLE depthView = device.dsvHeap.getCpu(depthBuffer.dsvIdx);

        frameControl.begin(nullptr);
        frameControl.cmdList->RSSetViewports(1, &viewport);
        frameControl.cmdList->RSSetScissorRects(1, &scissorRect);
        frameControl.barrier(target.resource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        frameControl.cmdList->OMSetRenderTargets(1, &target.view, 1, &depthView);
        frameControl.cmdList->ClearRenderTargetView(target.view, &view.clearColor.x, 0, nullptr);
        frameControl.cmdList->ClearDepthStencilView(depthView, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
        frameControl.heaps(device.cbvHeap.get());
        
        XMVECTOR posVec      = XMVectorSet(view.camera->pos.x,    view.camera->pos.y,    view.camera->pos.z,    1.0f);
        XMVECTOR targetVec   = XMVectorSet(view.camera->target.x, view.camera->target.y, view.camera->target.z, 1.0f);
        XMVECTOR upVec       = XMVectorSet(view.camera->up.x,     view.camera->up.y,     view.camera->up.z,     1.0f);
        XMMATRIX viewMat     = XMMatrixLookAtLH(posVec, targetVec, upVec);
        XMMATRIX projMat     = XMMatrixPerspectiveFovLH(view.camera->fovY, view.camera->aspect, view.camera->nearZ, view.camera->farZ);
        XMMATRIX viewProjMat = viewMat * projMat;

        if (view.fillMode == Fill || view.fillMode == FillWire) {
            // PIXBeginEvent(queue.obj.Get(), PIX_COLOR_DEFAULT, "Fill %llu", frameIdx);
            frameControl.cmdList->SetPipelineState(psoFill.obj.Get());
            frameControl.cmdList->SetGraphicsRootSignature(rootSignature.obj.Get());
            frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            for (int i = 0; i < view.modelCount; i++) {
                const Model& model = view.models[i];
                const mat4& transform = view.transforms[i];
                int meshId = model.meshId;
                Mesh& m = meshRegistry.getMesh(meshId);
    
                XMMATRIX modelMat = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&transform));
                XMMATRIX mvpMat = modelMat * viewMat * projMat;

                ObjectData objectData = {};
                XMStoreFloat4x4(&objectData.mvp, mvpMat);
                // XMStoreFloat4x4(&objectData.mvp, XMMatrixTranspose(mvp)));
                
                
                frameControl.setConstantBuffer(0, objectData);
                frameControl.cmdList->IASetVertexBuffers(0, 1, &m.positionView);
                frameControl.cmdList->IASetVertexBuffers(1, 1, &m.colorView);
                if (m.indicesView.SizeInBytes != 0) {
                    frameControl.cmdList->IASetIndexBuffer(&m.indicesView);
                    frameControl.cmdList->DrawIndexedInstanced(m.vCount, 1, 0, 0, 0);
                } else {
                    frameControl.cmdList->DrawInstanced(m.vCount, 1, 0, 0);
                }
            }
            // PIXEndEvent(queue.obj.Get());
        }

        // if (fillMode == Wire || fillMode == FillWire) {
        //     // PIXBeginEvent(queue.obj.Get(), PIX_COLOR_DEFAULT, "Wire %llu", frameIdx);
        //     frameControl.cmdList->SetPipelineState(psoWire.obj.Get());
        //     frameControl.cmdList->SetGraphicsRootSignature(rootSignature.obj.Get());
        //     frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        //     for (int i = 0; i < items.size(); i++) {
        //         int meshId = items[i].meshId;
        //         Mesh& m = meshRegistry.getMesh(meshId);
        //         D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = objectConstants.getAddress(i);
        //         frameControl.cmdList->SetGraphicsRootConstantBufferView(0, cbvAddress);
        //         frameControl.cmdList->IASetIndexBuffer(&m.indicesView);
        //         frameControl.cmdList->IASetVertexBuffers(0, 1, &m.positionView);
        //         frameControl.cmdList->DrawIndexedInstanced(m.vCount, 1, 0, 0, 0);
        //     }
        //     // PIXEndEvent(queue.obj.Get());
        // }


        frameControl.barrier(target.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        GUARD(frameControl.execute());
        GUARD(swapchain.present(true));
        GUARD(frameControl.end());
        PIXEndEvent();
        frameIdx++;
        return 1;
    }
    
    void trs2Transform(int count, const Trs* trs, mat4* transforms) {
        for (int i = 0; i < count; i++, trs++, transforms++) {
            XMVECTOR q = XMVectorSet(trs->rotation.x, trs->rotation.y, -trs->rotation.z, -trs->rotation.w);
            q = XMQuaternionNormalize(q);
            XMMATRIX S = XMMatrixScaling(trs->scale.x, trs->scale.y, -trs->scale.z);
            XMMATRIX R = XMMatrixRotationQuaternion(q);
            XMMATRIX T = XMMatrixTranslation(trs->position.x, trs->position.y, -trs->position.z);
            XMMATRIX transformMat = S * R * T;
            XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(transforms), transformMat);
        }
    }
    

    int addBuffer(const BufferDesc& desc) {
        return meshRegistry.addBuffer(device, desc);
    }
    
    int addMesh(const MeshDesc& desc) {
        return meshRegistry.addMesh(desc);
    }

private:
    Factory                             factory;
    Device                              device;
    Swapchain                           swapchain;
    DepthBuffer                         depthBuffer;
    Queue                               queue;
    FrameControl                        frameControl;
    MeshRegistry                        meshRegistry;
    RootSig                             rootSignature;
    PipelineFill                        psoFill;
    PipelineWire                        psoWire;
    D3D12_VIEWPORT                      viewport;
    D3D12_RECT                          scissorRect;
};

std::unique_ptr<IRenderer> createRendererD3D() {
    return std::make_unique<RendererD3D>();
}
}

// 563