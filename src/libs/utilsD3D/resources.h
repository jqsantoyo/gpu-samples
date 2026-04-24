#pragma once
#include "core.h"

namespace gpu {




struct Buffer2 {
    uint32_t                                size;
    D3D12_HEAP_TYPE                         type;
    D3D12_RESOURCE_STATES                   state;
    Microsoft::WRL::ComPtr<ID3D12Resource>  obj;

    ID3D12Resource* get();
};

class WriteBuffer : public Buffer2 {
public:
    ~WriteBuffer();
    bool init(ID3D12Device* device, uint32_t size);
    bool write(uint32_t offset, uint32_t size, uint8_t* data);
private:
    uint8_t* data;
};

class UAVBuffer : public Buffer2 {
public:
    bool init(ID3D12Device* device, uint32_t size);
};

class ReadBuffer : public Buffer2 {
public:
    bool init(ID3D12Device* device, uint32_t size);
    uint8_t* start();
    void stop();
};


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
    void reset(int buffersIdx, int meshesIdx);
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
    void reset(int idx);
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
    void reset(int idx);
    int addMaterial(Material& material);
    Material& getMaterial(int idx);
    int getCount();
private:
    Device* device;
    std::vector<Material> materials;
};



}