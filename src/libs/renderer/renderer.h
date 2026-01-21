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


class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual int start(void* window, uint32_t screenWidth, uint32_t screenHeight) = 0;
    virtual void stop() = 0;
    virtual void setView(ViewDesc& desc) = 0;
    virtual void setProjection(ProjectionDesc& desc) = 0;
    virtual int addBuffer(const BufferDesc& desc) = 0;
    virtual int addMesh(const MeshDesc& desc) = 0;
    virtual int render(const Color& clearColor, const std::vector<RenderItem>& items) = 0;
    virtual void setFillMode(FillMode mode) = 0;
};

std::unique_ptr<IRenderer> createRenderer();



}