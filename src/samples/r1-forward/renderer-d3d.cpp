#include <rendererInterface/renderer.h>
#include <utils/utils.h>
#include <app/app.h>
#include <gpuD3D/gpu.h>
#include <d3dx12.h>
#include <pix3.h>
#include <wrl.h>
#include <shellapi.h>
#include <string>
#include <stdio.h>
#include <iostream>

using namespace DirectX;

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

        gpu.init(frameCount, 2, 100);
        queue     = gpu.createQueue();
        swapchain = gpu.createSwapchain(queue, hwnd, width, height, frameCount);

        gpu::PsoGraphicsDesc pipelineDesc = {
            .vs                     = "shaders_v",
            .ps                     = "shaders_p",
            .blendState             = gpu::defaultBlend(),
            .sampleMask             = UINT_MAX,
            .rasterizerState        = gpu::defaultFill(),
            .depthStencilState      = gpu::defaultDepth(),
            .inputLayout            = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    1,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "UV",       0, DXGI_FORMAT_R32G32_FLOAT,       2,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            },
            .topology           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .numRenderTargets   = 1,
            .rtvFormats         = { DXGI_FORMAT_R8G8B8A8_UNORM },
            .dsvFormat          = DXGI_FORMAT_D32_FLOAT,
            .sampleDesc         = { .Count = 1, .Quality = 0 },
        };

        gpu::PsoGraphicsDesc pipelineWireDesc = {
            .vs                 = "shadersWire_v",
            .ps                 = "shadersWire_p",
            .blendState         = gpu::noBlend(),
            .sampleMask         = UINT_MAX,
            .rasterizerState    = gpu::defaultWire(),
            .depthStencilState  = gpu::defaultDepth(),
            .inputLayout        = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            },
            .topology           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .numRenderTargets   = 1,
            .rtvFormats         = { DXGI_FORMAT_R8G8B8A8_UNORM },
            .dsvFormat          = DXGI_FORMAT_D32_FLOAT,
            .sampleDesc         = { .Count = 1, .Quality = 0 },
        };
        root = gpu.createRoot(
            {
                gpu::rootCbv(0),
                gpu::rootCbv(1),
                gpu::rootCbv(2),
                gpu::rootTable({ gpu::rangeSrv(5, 0), }),
            },
            {
                CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC), // aniso sampler
                gpu::shadowSampler(),
            }
        );
        pipeline     = gpu.createPipeline(root, pipelineDesc);
        pipelineWire = gpu.createPipeline(root, pipelineWireDesc);
        mainCommand  = gpu.createCommand();
        uploadBuffer = gpu.createBuffer(2048 * 2048 * 4, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COMMON);

        frameControl.init(gpu, 2, maxFrameMemory);
        depthBuffer.init(&gpu, width, height);
        meshRegistry.init(&gpu);
        textureRegistry.init(&gpu, queue, mainCommand, uploadBuffer, 0);
        materialRegistry.init(&gpu);
        shadow.init(&gpu, root, 1024, 1024);

        defaultBaseColorMap = textureRegistry.add({ 1, 1, 1, 1 });   // default base color
        // textureRegistry.addDefault({1, 1, 0, 1});   // default base color (magenta)
        defaultORMMap       = textureRegistry.add({ 1, 1, 0, 1 });   // default ORM
        defaultNormalMap    = textureRegistry.add({ 1, .5, .5, 1 }); // default normals
        defaultEmissiveMap  = textureRegistry.add({ 1, 0, 0, 0 });   // default emissive

        buffersBaseIdx   = gpu.buffers.size();
        texturesBaseIdx  = textureRegistry.getCount();
        meshesBaseIdx    = meshRegistry.getMeshCount();
        materialsBaseIdx = materialRegistry.getCount();
        
        reset();
        return true;
    }

    void terminate() {
        gpu.wait(queue);
    }

    void reset() {
        wait();
        // gpu.resetMain(1);
        gpu.buffers.resize(buffersBaseIdx);
        // gpu.textures.resize(texturesBaseIdx);
        meshRegistry.reset(meshesBaseIdx);
        materialRegistry.reset(materialsBaseIdx);
    }

    void wait() {
        gpu.wait(queue);
    }

    bool resize(int width, int height) {
        return 1;
    }

    bool render(const RenderView& view) {
        static uint64_t frameIdx = 0;
        PIXBeginEvent(PIX_COLOR_DEFAULT, "Render %llu", frameIdx);
        gpu::SwapTarget target = gpu.next(swapchain);


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
            .shadowMap = shadow.srv.idx,
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
        
        gpu::Command cmd = frameControl.next();
        gpu.wait(queue, cmd);
        gpu.begin(cmd);
        gpu.heaps(cmd, gpu.mainHeap.get());

        if (view.enableShadows) {
            D3D12_VIEWPORT shadowViewport = { 0.0f, 0.0f, static_cast<float>(1024), static_cast<float>(1024), 0, 1 };
            D3D12_RECT shadowScissor = { 0, 0, long(1024), long(1024) };
            gpu.viewport     (cmd, shadowViewport);
            gpu.scissor      (cmd, shadowScissor);
            gpu.barrier      (cmd, shadow.target, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            gpu.target       (cmd, shadow.dsv);
            gpu.clear        (cmd, shadow.dsv, 1, 0);

            gpu.pipeline          (cmd, shadow.pipeline);
            gpu.graphicsRoot (cmd, root);
            gpu.graphicsCbv  (cmd, 2, passData);
            gpu.graphicsTable(cmd, 3, gpu.mainHeap.gpu(0));
            gpu.topology(cmd, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
                
                gpu.graphicsCbv  (cmd, 0, objectData);
                gpu.indexBuffer  (cmd, m.indicesView);
                gpu.vertexBuffer (cmd, 0, m.positionView);
                gpu.drawIndexed  (cmd, m.vCount, 1, 0, 0, 0);
            }
            gpu.barrier(cmd, shadow.target, D3D12_RESOURCE_STATE_GENERIC_READ);
        } else {
            gpu.barrier(cmd, shadow.target, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            gpu.clear  (cmd, shadow.dsv, 1, 0);
            gpu.barrier(cmd, shadow.target, D3D12_RESOURCE_STATE_GENERIC_READ);
        }


        gpu.viewport (cmd, viewport);
        gpu.scissor  (cmd, scissorRect);
        gpu.barrier  (cmd, target.texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
        gpu.targets  (cmd, target.rtv, depthBuffer.dsv);
        gpu.clear    (cmd, target.rtv, view.clearColor);
        gpu.clear    (cmd, depthBuffer.dsv, 1, 0);

        if (view.fillMode == Fill || view.fillMode == FillWire) {
            gpu.pipeline(cmd, pipeline);
            gpu.graphicsRoot(cmd, root);
            gpu.graphicsCbv(cmd, 2, passData);
            gpu.graphicsTable(cmd, 3, gpu.mainHeap.gpu(0));
            gpu.topology(cmd, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
                
                gpu.graphicsCbv  (cmd, 0, objectData);
                gpu.graphicsCbv  (cmd, 1, mat);
                gpu.indexBuffer  (cmd, m.indicesView);
                gpu.vertexBuffer (cmd, 0, m.positionView);
                gpu.vertexBuffer (cmd, 1, m.normalView);
                gpu.vertexBuffer (cmd, 2, m.uvView);
                gpu.vertexBuffer (cmd, 3, m.tangentView);
                gpu.drawIndexed  (cmd, m.vCount, 1, 0, 0, 0);
            }
        }


        gpu.barrier(cmd, target.texture, D3D12_RESOURCE_STATE_PRESENT);
        gpu.end(cmd);
        gpu.execute(queue, cmd);
        gpu.present(swapchain, false);
        gpu.signal(queue, cmd);
        gpu.printErrors();
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
        gpu::Buffer buffer = gpu.createBuffer(desc.size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        gpu.write(buffer, desc.offset, desc.size, desc.data);
        return buffer.idx;
    }
    
    int addMesh(const MeshDesc& desc) {
        return meshRegistry.addMesh(desc);
    }

    int addTexture(const char* filename) {
        return textureRegistry.add(filename);
    }

    int addTexture(const char* name, const uint8_t* data, uint32_t size) {
        return textureRegistry.add(name, data, size);
    }
    
    virtual int addMaterial(Material& material) {
        return materialRegistry.addMaterial(material);
    }

    int getTextureDesc(int idx) {
        return textureRegistry.getSrv(idx);
    }

    int getBufferCount() {
        return gpu.buffers.size();
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
    gpu::Gpu            gpu;
    gpu::Swapchain      swapchain;
    gpu::Queue          queue;
    gpu::Buffer         uploadBuffer;
    gpu::Command        mainCommand;
    gpu::Root           root;
    gpu::Pipeline       pipeline;
    gpu::Pipeline       pipelineWire;
    DepthBuffer         depthBuffer;
    FrameControl        frameControl;
    MeshRegistry        meshRegistry;
    TextureRegistry     textureRegistry;
    MaterialRegistry    materialRegistry;
    Shadow              shadow;
    D3D12_VIEWPORT      viewport;
    D3D12_RECT          scissorRect;
    int                 buffersBaseIdx;   // Tracks beginning of user-level buffers
    int                 meshesBaseIdx;    // Tracks beginning of user-level meshes
    int                 texturesBaseIdx;  // Tracks beginning of user-level textures
    int                 materialsBaseIdx; // Tracks beginning of user-level materials
};

std::unique_ptr<IRenderer> createRendererD3D() {
    return std::make_unique<RendererD3D>();
}
}