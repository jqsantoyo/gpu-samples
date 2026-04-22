#pragma once
#include "core.h"

namespace gpu {

struct Buffer {
    Microsoft::WRL::ComPtr<ID3D12Resource>      vbUp;
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

struct Texture {
    Microsoft::WRL::ComPtr<ID3D12Resource> res;
    Microsoft::WRL::ComPtr<ID3D12Resource> resUp;
    int descriptor;
};



class MeshRegistry {
public:
    bool init();
    void reset();
    int addBuffer(Device& device, const BufferDesc& desc);
    int addMesh(const MeshDesc& desc);
    int getBufferCount();
    int getMeshCount();
    Mesh& getMesh(int idx);
private:
    std::vector<Buffer> buffers;
    std::vector<Mesh>   meshes;
};



class TextureRegistry {
public:
    bool init(Device* device, Queue* queue);
    void reset();
    int addTexture(const char* filename);
    int addTexture(const uint8_t* data, uint32_t size);
    int getCount();
    int addDefault(vec4 color);
    Texture& get(int idx);
private:
    Device* device;
    Queue* queue;
    std::vector<Texture> textures;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>  cmdAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   cmdList;
};



class MaterialRegistry {
public:
    bool init(Device* device);
    void reset();
    int addMaterial(Material& material);
    Material& getMaterial(int idx);
    int getCount();
private:
    Device* device;
    std::vector<Material> materials;
};



}