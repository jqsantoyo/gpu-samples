#pragma once
#include <gpuD3D/gpu.h>


namespace gpu {


struct BufferDesc {
    size_t offset;
    size_t size;
    const uint8_t* data;
};

struct BufferViewDesc {
    size_t offset;
    size_t size;
};

struct MeshDesc {
    int             bufferId;
    bool            indexFormatU16;
    size_t          vCount;
    BufferViewDesc  indices;
    BufferViewDesc  position;
    BufferViewDesc  normal;
    BufferViewDesc  uv;
    BufferViewDesc  tangent;
    BufferViewDesc  color;
};

struct Material
{
    vec4 baseColor;
    vec3 emissive;
    float metallic;
    float roughness;
    int baseColorMap;
    int ormMap;
    int normalMap;
    int emissiveMap;
};

struct DepthBuffer {
    gpu::Texture texture;
    gpu::Dsv dsv;
    bool init(gpu::Gpu* gpu, uint32_t width, uint32_t height);
};


class Shadow {
public:
    bool init(gpu::Gpu* gpu, gpu::Root root, uint32_t width, uint32_t height);
    gpu::Pipeline   pipeline;
    gpu::Texture    target;
    gpu::Srv        srv;
    gpu::Dsv        dsv;
    uint32_t        width;
    uint32_t        height;
};




class FrameControl {
public:
    bool init(gpu::Gpu& gpu, int frameCount, uint32_t maxMemory = 0);
    gpu::Command next();
private:
    std::vector<gpu::Command>   cmds;
    int                         frameIdx;
};




struct Mesh {
    int                         bufferId;
    size_t                      vCount;
    D3D12_INDEX_BUFFER_VIEW     indicesView;
    D3D12_VERTEX_BUFFER_VIEW    positionView;
    D3D12_VERTEX_BUFFER_VIEW    normalView;
    D3D12_VERTEX_BUFFER_VIEW    uvView;
    D3D12_VERTEX_BUFFER_VIEW    tangentView;
    D3D12_VERTEX_BUFFER_VIEW    colorView;
};




class MeshRegistry {
public:
    bool init(gpu::Gpu* gpu);
    void reset(int meshesIdx);
    int addMesh(const MeshDesc& desc);
    int getMeshCount();
    Mesh& getMesh(int idx);
private:
    gpu::Gpu*           gpu;
    std::vector<Mesh>   meshes;
};


struct Texture {
    gpu::Texture    id;
    gpu::Srv        srv;
};

class TextureRegistry {
public:
    bool init(gpu::Gpu* gpu, gpu::Queue queue, gpu::Command cmd, gpu::Buffer uploadBuffer, int page);
    int  getCount();
    int  getSrv(int idx);
    int  add(vec4 color);
    int  add(const char* filename);
    int  add(const char* name, const uint8_t* data, uint32_t size);

private:
    gpu::Gpu*               gpu;
    gpu::Queue              queue;
    gpu::Command            cmd;
    gpu::Buffer             uploadBuffer;
    int                     page;
    std::vector<Texture>    textures;
    bool upload(gpu::Texture texture, std::vector<D3D12_SUBRESOURCE_DATA>& subresources);
    bool createSrv(Texture& texture, ID3D12Resource* res);
};


class MaterialRegistry {
public:
    bool init(gpu::Gpu* gpu);
    void reset(int idx);
    int addMaterial(Material& material);
    Material& getMaterial(int idx);
    int getCount();
private:
    gpu::Gpu* gpu;
    std::vector<Material> materials;
};



}