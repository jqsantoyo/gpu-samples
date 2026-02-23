#pragma once
#include <memory>
#include <vector>

namespace gpu {

struct ViewDesc {
    float pos[3];
    float target[3];
    float up[3];
};

struct ProjectionDesc {
    float fovAngleY;
    float aspectRatio;
    float nearZ;
    float farZ;
};

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
    int      bufferId;
    size_t   vCount;
    BufferViewDesc indices;
    BufferViewDesc position;
    BufferViewDesc normal;
    BufferViewDesc uv;
    BufferViewDesc color;
};

struct Vertex {
    float position[3];
    float color[4];
};

struct Color {
    float v[4];
};


struct RenderItem {
    float position[3];
    float scale[3];
    float rotation[4];
    int meshId;
    int materialId;
};

enum FillMode {
    Fill,
    FillWire,
    Wire,
};

struct RendererInitInfo {
    uint32_t width;
    uint32_t height;
    void* window;
    void* view;
    void* device;
};

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool init(const RendererInitInfo& info) = 0;
    virtual void terminate() = 0;
    virtual bool resize(int width, int height) = 0;
    virtual bool render(const Color& clearColor, const std::vector<RenderItem>& items) = 0;

    virtual void setFillMode(FillMode mode) {}
    virtual void setView(ViewDesc& desc) {};
    virtual void setProjection(ProjectionDesc& desc) {};
    virtual int addBuffer(const BufferDesc& desc) { return -1; };
    virtual int addMesh(const MeshDesc& desc) { return -1; };
};

std::unique_ptr<IRenderer> createRenderer();



}