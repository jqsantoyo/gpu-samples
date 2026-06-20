#include <renderer/renderer.h>
#include <renderer/rendererBase.h>
#include <gpu/gpu.h>
#include <app/app.h>
#include <directxmath.h>

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



class Renderer : public RendererBase {
public:

    void init2() {
        uint32_t width  = static_cast<uint32_t>(viewport.width);
        uint32_t height = static_cast<uint32_t>(viewport.height);
        uint32_t materialData = align256(sizeof(MaterialData));
        uint32_t passData = align256(sizeof(PassData));
        uint32_t objectMemory = sizeof(ObjectData);
        uint32_t alignedObjectMemory = align256(objectMemory);
        uint32_t maxFrameMemory = passData + 10 * materialData + 100 * alignedObjectMemory;

        PsoGraphicsDesc pipelineDesc = {
            .vs                 = "shaders_v",
            .ps                 = "shaders_p",
            .inputElements      = {
                { "POSITION", 0, Format::RGB32f,  0, 0 },
                { "NORMAL",   0, Format::RGB32f,  1, 0 },
                { "UV",       0, Format::RG32f,   2, 0 },
                { "TANGENT",  0, Format::RGBA32f, 3, 0 },
            },
        };

        PsoGraphicsDesc pipelineWireDesc = {
            .vs                 = "shadersWire_v",
            .ps                 = "shadersWire_p",
            .fillMode           = FillMode::Wireframe,
            .inputElements      = {
                { "POSITION", 0, Format::RGB32f, 0, 0 },
            },
        };
        PsoGraphicsDesc pipelineShadowDesc = {
            .vs                 = "shadow_v",
            .ps                 = "shadow_p",
            .inputElements      = {
                { "POSITION", 0, Format::RGB32f,  0, 0 },
            },
            .dsvFormat          = Format::D24S8,
        };
        root = gpu->createRoot({
                RootParam::binding(RootBinding::Constant, 0),
                RootParam::binding(RootBinding::Constant, 1),
                RootParam::binding(RootBinding::Constant, 2),
                RootParam::table({
                    RootRange::range(RootBinding::Read, 10, 0)
                }),
            }, {
                { 0, Filter::Anisotropic },
                { 1, Filter::Linear      },
            }
        );
        pipeline        = gpu->createPipeline(root, pipelineDesc);
        pipelineWire    = gpu->createPipeline(root, pipelineWireDesc);
        pipelineShadow  = gpu->createPipeline(root, pipelineShadowDesc);

        depthTexture    = gpu->createTexture2("depth", TextureUsage::DepthStencil, Format::D32, { width, height, 1 }, 1, 1, { 1, 0u });
        depthView       = gpu->createDepthView(depthTexture);

        shadowTexture   = gpu->createTexture2("ShadowTex", TextureUsage::DepthStencil, Format::D24S8, { width, height, 1 }, 1, 1, ClearValue(1.0f, 0u));
        shadowDepthView = gpu->createDepthView(shadowTexture);
        shadowReadView  = gpu->createTextureView(shadowTexture);
    }

    void terminate2() {
    }

    void reset2() {
    }

    void resize2(int width, int height) {
    }

    void render2(SwapTarget target, Command cmd, const RenderView& view) {


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
            .shadowMap = gpu->getTextureViewBind(shadowReadView),
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

        gpu->wait(queue, cmd);
        gpu->begin(cmd);
        gpu->bindHeap(cmd);

        if (view.enableShadows) {
            gpu->viewport     (cmd, viewport);
            gpu->scissor      (cmd, scissorRect);
            gpu->barrier      (cmd, shadowTexture, State::DepthWrite);
            gpu->targets      (cmd, {-1}, shadowDepthView);
            gpu->clear        (cmd, {-1}, {}, shadowDepthView, 1, 0);

            gpu->pipeline          (cmd, pipelineShadow);
            gpu->graphicsRoot (cmd, root);
            gpu->graphicsCbv  (cmd, 2, passData);
            gpu->graphicsTable(cmd, 3, 0);
            gpu->topology(cmd, PrimitiveTopology::TriangleList);
            for (int i = 0; i < view.modelCount; i++) {
                const Model& model = view.models[i];
                const mat4& transform = view.transforms[i];
                int meshId = model.meshId;
                int materialId = model.materialId;
                if (meshId < 0) {
                    continue;
                }
                MeshData& m = meshes[meshId];

                XMMATRIX modelMat = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(transform.v));
                ObjectData objectData = {};
                XMStoreFloat4x4(&objectData.m, modelMat);
                
                gpu->graphicsCbv  (cmd, 0, objectData);
                gpu->indexBuffer  (cmd, m.indices);
                gpu->vertexBuffer (cmd, 0, m.position);
                gpu->drawIndexed  (cmd, m.vCount, 1, 0, 0, 0);
            }
            gpu->barrier(cmd, shadowTexture, State::GenericRead);
        } else {
            gpu->barrier(cmd, shadowTexture, State::DepthWrite);
            gpu->clear  (cmd, {-1}, {}, shadowDepthView, 1, 0);
            gpu->barrier(cmd, shadowTexture, State::GenericRead);
        }


        gpu->viewport (cmd, viewport);
        gpu->scissor  (cmd, scissorRect);
        gpu->barrier  (cmd, target.texture, State::RenderTarget);
        gpu->targets  (cmd, target.renderView, depthView);
        gpu->clear    (cmd, target.renderView, view.clearColor, depthView, 1, 0);

        if (view.fillMode == Fill || view.fillMode == FillWire) {
            gpu->pipeline       (cmd, pipeline);
            gpu->graphicsRoot   (cmd, root);
            gpu->graphicsCbv    (cmd, 2, passData);
            gpu->graphicsTable  (cmd, 3, 0);
            gpu->topology       (cmd, PrimitiveTopology::TriangleList);
            for (int i = 0; i < view.modelCount; i++) {
                const Model& model = view.models[i];
                const mat4& transform = view.transforms[i];
                int meshId = model.meshId;
                int materialId = model.materialId;
                if (meshId < 0) {
                    continue;
                }
                MeshData&     m    = meshes[meshId];
                MaterialData& mat  = materials[materialId];
                MaterialData  mat2 = mat;
                mat2.baseColorMap    = gpu->getTextureViewBind({ mat.baseColorMap });
                mat2.ormMap          = gpu->getTextureViewBind({ mat.ormMap });
                mat2.normalMap       = gpu->getTextureViewBind({ mat.normalMap });
                mat2.emissiveMap     = gpu->getTextureViewBind({ mat.emissiveMap });

    
                XMMATRIX modelMat = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(transform.v));
                XMMATRIX mvpMat = modelMat * viewMat * projMat;

                ObjectData objectData = {};
                XMStoreFloat4x4(&objectData.m, modelMat);
                // XMStoreFloat4x4(&objectData.mvp, XMMatrixTranspose(mvp)));
                
                gpu->graphicsCbv  (cmd, 0, objectData);
                gpu->graphicsCbv  (cmd, 1, mat2);
                gpu->indexBuffer  (cmd, m.indices);
                gpu->vertexBuffer (cmd, 0, m.position);
                gpu->vertexBuffer (cmd, 1, m.normal);
                gpu->vertexBuffer (cmd, 2, m.uv);
                gpu->vertexBuffer (cmd, 3, m.tangent);
                gpu->drawIndexed  (cmd, m.vCount, 1, 0, 0, 0);
            }
        }


        gpu->barrier(cmd, target.texture, State::Present);
        gpu->end(cmd);
        gpu->execute(queue, cmd);
        gpu->present(swapchain, false);
        gpu->signal(queue, cmd);
    }


private:
    Root                    root;
    Pipeline                pipeline;
    Pipeline                pipelineWire;
    Pipeline                pipelineShadow;
    Texture                 depthTexture;
    TextureView             depthView;
    Texture                 shadowTexture;
    TextureView             shadowDepthView;
    TextureView             shadowReadView;
};  

std::unique_ptr<IRenderer> createRendererPbr() {
    return std::make_unique<Renderer>();
}
}