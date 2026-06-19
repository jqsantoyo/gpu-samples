#pragma once
#include <gpu/gpu.h>


namespace gpu {




class Shadow {
public:
    bool init(IGpu* gpu, Root root, uint32_t width, uint32_t height);
    Pipeline    pipeline;
    Texture     target;
    TextureView depthView;
    TextureView readView;
    uint32_t    width;
    uint32_t    height;
};




class FrameControl {
public:
    bool init(IGpu* gpu, int frameCount, uint32_t maxMemory = 0);
    Command next();
private:
    std::vector<Command>   cmds;
    int                         frameIdx;
};


struct Mesh { int idx; };
struct MeshData {
    size_t           vCount;
    IndexBufferView  indices;
    VertexBufferView position;
    VertexBufferView normal;
    VertexBufferView uv;
    VertexBufferView tangent;
    VertexBufferView color;
};

class MeshRegistry {
public:
    bool        init(IGpu* gpu, int meshCount);
    void        reset();
    Mesh        create(const MeshData& desc);
    MeshData&   get(Mesh mesh);
private:
    IGpu*               gpu;
    Vec<Mesh, MeshData> meshes;
};


struct MaterialTexture { int idx; };

struct MaterialTextureData {
    Texture     texture;
    TextureView textureView;
};

struct Material { int idx; };
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

struct MaterialDesc {
    vec4 baseColor;
    vec3 emissive;
    float metallic;
    float roughness;
    MaterialTexture baseColorMap;
    MaterialTexture ormMap;
    MaterialTexture normalMap;
    MaterialTexture emissiveMap;
};

class MaterialRegistry {
public:
    bool            init    (IGpu* gpu, Queue queue, Command cmd, Buffer uploadBuffer, int materialTextureCount, int materialCount);
    MaterialTexture create  (const char* name, vec4 color);
    MaterialTexture create  (const char* name, const uint8_t* data, uint32_t size);
    Material        create  (const char* name, MaterialDesc& desc);
    void            destroy (MaterialTexture materialTexture);
    void            destroy (Material material);
    MaterialData&   get     (Material material);

private:
    bool upload(Texture texture, const std::vector<ResourceData>& subresources);
    IGpu*                                       gpu;
    Queue                                       queue;
    Command                                     cmd;
    Buffer                                      uploadBuffer;
    Vec<MaterialTexture, MaterialTextureData>   materialTextures;
    Vec<Material, MaterialData>                 materials;
    MaterialTexture                             defaultBaseColorMap;
    MaterialTexture                             defaultORMMap;
    MaterialTexture                             defaultEmissiveMap;
    MaterialTexture                             defaultNormalMap;
};



}