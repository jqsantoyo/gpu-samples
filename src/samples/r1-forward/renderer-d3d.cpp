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
    XMFLOAT4X4 m;
};

struct PassData {
    XMFLOAT4X4  view;
    XMFLOAT4X4  proj;
    XMFLOAT4X4  viewProj;
    vec3        eye;
    float       pad0;
    XMFLOAT4X4  lightViewProj;
    Light       light[12];
    vec2        screenSize;
    float       deltaTime;
    float       absoluteTime;
    int         shadowMap;
};




class RendererD3D : public IRenderer {
public:

    bool init(void* window, uint32_t width, uint32_t height) {
        auto hwnd = static_cast<HWND>(window);
        viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0, 1 };
        scissorRect = { 0, 0, long(width), long(height) };
        UINT frameCount = 2;
        uint32_t materialData = align256(sizeof(Material));
        uint32_t passData = align256(sizeof(PassData));
        uint32_t objectMemory = sizeof(ObjectData);
        uint32_t alignedObjectMemory = align256(objectMemory);
        uint32_t maxFrameMemory = passData + 10 * materialData + 100 * alignedObjectMemory;

        GUARD(factory.init());
        GUARD(factory.select());
        GUARD(device.init(factory.getSelected(), frameCount, 2, 100));
        GUARD(queue.init(device, D3D12_COMMAND_LIST_TYPE_DIRECT));
        GUARD(swapchain.init(factory, device, queue, hwnd, width, height, frameCount));
        GUARD(frameControl.init(device, &queue, 2, maxFrameMemory));
        GUARD(rootSignature.init3Cbv1TableNSamplers(device));
        GUARD(pso.init(device, rootSignature));
        GUARD(psoWire.init(device, rootSignature));
        GUARD(depthBuffer.init(&device, width, height));
        GUARD(textureRegistry.init(&device, &queue));
        GUARD(materialRegistry.init(&device));
        GUARD(shadow.init(&device, rootSignature.obj.Get(), 1024, 1024));

        
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
        textureRegistry.reset();
        materialRegistry.reset();
        defaultBaseColorMap = textureRegistry.addDefault({1, 1, 1, 1});   // default base color
        // textureRegistry.addDefault({1, 1, 0, 1});   // default base color (magenta)
        defaultORMMap = textureRegistry.addDefault({1, 1, 0, 1});   // default ORM
        defaultNormalMap = textureRegistry.addDefault({1, .5, .5, 1}); // default normals
        defaultEmissiveMap = textureRegistry.addDefault({1, 0, 0, 0});   // default emissive
        shadow.reset();
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


        Light light0 = { { -5, 2, 5 },  2, {  1, -1, -1 }, 10, { 3.0f, 3.0f, 3.0f }, 1 };
        Light light1 = { { 5,  5, 0 },  2, { -1, -1,  0 }, 10, { 3.0f, 3.0f, 3.0f }, 3 };
        Light light2 = { { 0, 1, -5 },  2, {  0,  0,  1 }, 10, { 3.0f, 3.0f, 3.0f }, 4 };
        
        static float t = 0;
        t += .016;
        PassData passData = {
            .eye = view.camera->pos,
            .light = {
                light0,
                light1,
                light2,
            },
            .deltaTime = 0,
            .absoluteTime = t,
            .shadowMap = shadow.srvIdx,
        };

        {
            XMVECTOR posVec      = XMVectorSet(light1.position.x, light1.position.y, light1.position.z, 1.0f);
            XMVECTOR targetVec   = XMVectorSet(0, 0, 0, 1.0f);
            XMVECTOR upVec       = XMVectorSet(0, 1, 0,     1.0f);
            XMMATRIX viewMat     = XMMatrixLookAtLH(posVec, targetVec, upVec);
            XMMATRIX projMat     = XMMatrixPerspectiveFovLH(view.camera->fovY, view.camera->aspect, view.camera->nearZ, view.camera->farZ);
            XMMATRIX viewProjMat = viewMat * projMat;
            XMStoreFloat4x4(&passData.lightViewProj, viewProjMat);
        }

        XMVECTOR posVec      = XMVectorSet(view.camera->pos.x,    view.camera->pos.y,    view.camera->pos.z,    1.0f);
        XMVECTOR targetVec   = XMVectorSet(view.camera->target.x, view.camera->target.y, view.camera->target.z, 1.0f);
        XMVECTOR upVec       = XMVectorSet(view.camera->up.x,     view.camera->up.y,     view.camera->up.z,     1.0f);
        XMMATRIX viewMat     = XMMatrixLookAtLH(posVec, targetVec, upVec);
        XMMATRIX projMat     = XMMatrixPerspectiveFovLH(view.camera->fovY, view.camera->aspect, view.camera->nearZ, view.camera->farZ);
        XMMATRIX viewProjMat = viewMat * projMat;
        XMStoreFloat4x4(&passData.view,         viewMat);
        XMStoreFloat4x4(&passData.proj,         projMat);
        XMStoreFloat4x4(&passData.viewProj,     viewProjMat);
        
        frameControl.begin(nullptr);
        frameControl.heaps(device.cbvHeap.get());

        if (view.enableShadows) {
            D3D12_VIEWPORT shadowViewport = { 0.0f, 0.0f, static_cast<float>(1024), static_cast<float>(1024), 0, 1 };
            D3D12_RECT shadowScissor = { 0, 0, long(1024), long(1024) };
            D3D12_CPU_DESCRIPTOR_HANDLE shadowDsv = device.dsvHeap.getCpu(shadow.dsvIdx);
            frameControl.cmdList->RSSetViewports(1, &shadowViewport);
            frameControl.cmdList->RSSetScissorRects(1, &shadowScissor);
            frameControl.barrier(shadow.target.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            frameControl.cmdList->OMSetRenderTargets(0, nullptr, false, &shadowDsv);
            frameControl.cmdList->ClearDepthStencilView(shadowDsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

            frameControl.cmdList->SetPipelineState(shadow.pso.Get());
            frameControl.cmdList->SetGraphicsRootSignature(rootSignature.obj.Get());
            frameControl.setConstantBuffer(2, passData);
            frameControl.cmdList->SetGraphicsRootDescriptorTable(3, device.cbvHeap.getGpu(0));
            frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            for (int i = 0; i < view.modelCount; i++) {
                const Model& model = view.models[i];
                const mat4& transform = view.transforms[i];
                int meshId = model.meshId;
                int materialId = model.materialId;
                if (meshId < 0) {
                    continue;
                }
                Mesh& m = meshRegistry.getMesh(meshId);

                XMMATRIX modelMat = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(transform.v));
                ObjectData objectData = {};
                XMStoreFloat4x4(&objectData.m, modelMat);
                
                frameControl.setConstantBuffer(0, objectData);
                frameControl.cmdList->IASetIndexBuffer(&m.indicesView);
                frameControl.cmdList->IASetVertexBuffers(0, 1, &m.positionView);
                frameControl.cmdList->DrawIndexedInstanced(m.vCount, 1, 0, 0, 0);
            }
            frameControl.barrier(shadow.target.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
        } else {
            D3D12_CPU_DESCRIPTOR_HANDLE shadowDsv = device.dsvHeap.getCpu(shadow.dsvIdx);
            frameControl.barrier(shadow.target.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            frameControl.cmdList->ClearDepthStencilView(shadowDsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
            frameControl.barrier(shadow.target.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
        }


        
        frameControl.cmdList->RSSetViewports(1, &viewport);
        frameControl.cmdList->RSSetScissorRects(1, &scissorRect);
        frameControl.barrier(target.resource.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        frameControl.cmdList->OMSetRenderTargets(1, &target.view, true, &depthView);
        frameControl.cmdList->ClearRenderTargetView(target.view, &view.clearColor.x, 0, nullptr);
        frameControl.cmdList->ClearDepthStencilView(depthView, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);


        if (view.fillMode == Fill || view.fillMode == FillWire) {
            frameControl.cmdList->SetPipelineState(pso.obj.Get());
            frameControl.cmdList->SetGraphicsRootSignature(rootSignature.obj.Get());
            frameControl.setConstantBuffer(2, passData);
            frameControl.cmdList->SetGraphicsRootDescriptorTable(3, device.cbvHeap.getGpu(0));
            frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            for (int i = 0; i < view.modelCount; i++) {
                const Model& model = view.models[i];
                const mat4& transform = view.transforms[i];
                int meshId = model.meshId;
                int materialId = model.materialId;
                if (meshId < 0) {
                    continue;
                }
                Mesh& m = meshRegistry.getMesh(meshId);
                Material& mat = materialRegistry.getMaterial(materialId);
    
                XMMATRIX modelMat = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(transform.v));
                XMMATRIX mvpMat = modelMat * viewMat * projMat;

                ObjectData objectData = {};
                XMStoreFloat4x4(&objectData.m, modelMat);
                // XMStoreFloat4x4(&objectData.mvp, XMMatrixTranspose(mvp)));
                
                frameControl.setConstantBuffer(0, objectData);
                frameControl.setConstantBuffer(1, mat);
                frameControl.cmdList->IASetIndexBuffer(&m.indicesView);
                frameControl.cmdList->IASetVertexBuffers(0, 1, &m.positionView);
                frameControl.cmdList->IASetVertexBuffers(1, 1, &m.normalView);
                frameControl.cmdList->IASetVertexBuffers(2, 1, &m.uvView);
                frameControl.cmdList->IASetVertexBuffers(3, 1, &m.tangentView);
                frameControl.cmdList->DrawIndexedInstanced(m.vCount, 1, 0, 0, 0);
            }
        }

        // if (fillMode == Wire || fillMode == FillWire) {
        //     // PIXBeginEvent(queue.obj.Get(), PIX_COLOR_DEFAULT, "Wire %llu", frameIdx);
        //     frameControl.cmdList->SetPipelineState(psoWire.obj.Get());
        //     frameControl.cmdList->SetGraphicsRootSignature(rootSignature.obj.Get());
        //     frameControl.cmdList->SetGraphicsRootDescriptorTable(1, device.cbvHeap->GetGPUDescriptorHandleForHeapStart());
        //     frameControl.cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        //     for (int i = 0; i < items.size(); i++) {
        //         int meshId = items[i].meshId;
        //         Mesh& m = meshRegistry.getMesh(meshId);
        //         D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = objectConstants.getAddress(i);
        //         frameControl.cmdList->SetGraphicsRootConstantBufferView(0, cbvAddress);
        //         frameControl.cmdList->IASetIndexBuffer(&m.indicesView);
        //         frameControl.cmdList->IASetVertexBuffers(0, 1, &m.positionView);
        //         // frameControl.cmdList->IASetVertexBuffers(1, 1, &m.colorView);
        //         frameControl.cmdList->DrawIndexedInstanced(m.vCount, 1, 0, 0, 0);
        //         // device.printErrors();
        //     }
        //     // PIXEndEvent(queue.obj.Get());
        // }


        frameControl.barrier(target.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        GUARD(frameControl.execute());
        GUARD(swapchain.present(false));
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
            XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(transforms->v), transformMat);
        }
    }

    int addBuffer(const BufferDesc& desc) {
        wait();
        return meshRegistry.addBuffer(device, desc);
    }
    
    int addMesh(const MeshDesc& desc) {
        return meshRegistry.addMesh(desc);
    }

    int addTexture(const char* filename) {
        wait();
        return textureRegistry.addTexture(filename);
    }

    int addTexture(const uint8_t* data, uint32_t size) {
        wait();
        return textureRegistry.addTexture(data, size);
    }
    
    virtual int addMaterial(Material& material) {
        return materialRegistry.addMaterial(material);
    }

    int getTextureDesc(int idx) {
        return textureRegistry.get(idx).descriptor;
    }

    int getBufferCount() {
        return meshRegistry.getBufferCount();
    }

    int getMeshCount() {
        return meshRegistry.getMeshCount();
    }
    
    int getTextureCount() {
        return textureRegistry.getCount();
    }
    
    int getMaterialCount() {
        return materialRegistry.getCount();
    }

private:
    Factory             factory;
    Device              device;
    Swapchain           swapchain;
    DepthBuffer         depthBuffer;
    Queue               queue;
    FrameControl        frameControl;
    MeshRegistry        meshRegistry;
    TextureRegistry     textureRegistry;
    MaterialRegistry    materialRegistry;
    Shadow              shadow;
    RootSig             rootSignature;
    PipelineLights      pso;
    PipelineWire        psoWire;
    D3D12_VIEWPORT      viewport;
    D3D12_RECT          scissorRect;
};

std::unique_ptr<IRenderer> createRendererD3D() {
    return std::make_unique<RendererD3D>();
}
}