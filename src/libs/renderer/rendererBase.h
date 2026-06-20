#include "renderer.h"
#include <utils/utils.h>
#include <directxmath.h>

namespace gpu {

struct StaticBufferData {
    Buffer buffer;
};

struct MeshData {
    size_t           vCount;
    IndexBufferView  indices;
    VertexBufferView position;
    VertexBufferView normal;
    VertexBufferView uv;
    VertexBufferView tangent;
    VertexBufferView color;
};

struct MaterialTextureData {
    Texture     texture;
    TextureView textureView;
};

struct MaterialData {
    vec4 baseColor;
    vec3 emissive;
    float metallic;
    float roughness;
    int baseColorMap;
    int ormMap;
    int normalMap;
    int emissiveMap;
};

class RendererBase: public IRenderer {

public:
    virtual void    init2() = 0;
    virtual void    terminate2() = 0;
    virtual void    reset2() = 0;
    virtual void    resize2(int width, int height) = 0;
    virtual void    render2(SwapTarget target, Command cmd, const RenderView& view) = 0;

    void            init(const RendererBaseDesc& desc);
    void            terminate();
    void            reset();
    void            resize(int width, int height);
    void            render(const RenderView& view);
    void            wait();
    StaticBuffer    create(const char* name, uint32_t size, const uint8_t* data);
    Mesh            create(const MeshDesc& desc);
    MaterialTexture create(const char* name, vec4 color);
    MaterialTexture create(const char* name, const uint8_t* data, uint32_t size);
    Material        create(const char* name, MaterialDesc& desc);
    void            destroy(StaticBuffer staticBuffer);
    void            destroy(MaterialTexture materialTexture);
    void            destroy(Material material);

protected:

    bool upload(Texture texture, const std::vector<ResourceData>& subresources);

    bool                                        vulkan;
    std::unique_ptr<IGpu>                       gpu;
    Queue                                       queue;
    Swapchain                                   swapchain;
    Command                                     mainCommand;
    Buffer                                      uploadBuffer;
    Viewport                                    viewport;
    Rect                                        scissorRect;
    std::vector<Command>                        cmds;
    int                                         frameIdx;
    Vec<StaticBuffer,    StaticBufferData>      staticBuffers;
    Vec<Mesh,            MeshData>              meshes;
    Vec<MaterialTexture, MaterialTextureData>   materialTextures;
    Vec<Material,        MaterialData>          materials;
    MaterialTexture                             defaultBaseColorMap;
    MaterialTexture                             defaultORMMap;
    MaterialTexture                             defaultEmissiveMap;
    MaterialTexture                             defaultNormalMap;
};

}
