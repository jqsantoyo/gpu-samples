#pragma once
#include <utils/utils.h>
#include <memory>
#include <vector>

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
    int      bufferId;
    size_t   vCount;
    BufferViewDesc indices;
    BufferViewDesc position;
    BufferViewDesc normal;
    BufferViewDesc uv;
    BufferViewDesc color;
};

enum FillMode {
    Fill,
    FillWire,
    Wire,
};

struct Camera {
    vec3 pos;
    vec3 target;
    vec3 up;
    float fovY;
    float aspect;
    float nearZ;
    float farZ;
};

struct Light {
    vec3  position;
    float fallOffStart;
    vec3  direction;
    float fallOffEnd;
    vec3  intensity;
    float spotPower;
};

struct Model {
    int meshId;
    int materialId;
};


struct RenderView {
    vec4 clearColor;
    FillMode fillMode;
    int lightCount;
    int modelCount;
    const Camera* camera;
    const Light* lights;
    const mat4* transforms;
    const Model* models;
};


class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool init(void* window, uint32_t width, uint32_t height) = 0;
    virtual void terminate() = 0;
    virtual void wait() = 0;
    virtual bool resize(int width, int height) = 0;
    virtual bool render(const RenderView& view) = 0;
    virtual void trs2Transform(int count, const Trs* trs, mat4* transforms) = 0; // temporarily inside IRenderer

    virtual int addBuffer(const BufferDesc& desc) { return -1; };
    virtual int addMesh(const MeshDesc& desc) { return -1; };
    virtual int addTexture(const char* filename) { return -1; };
};

std::unique_ptr<IRenderer> createRenderer(bool vulkan);
std::unique_ptr<IRenderer> createRendererD3D();
std::unique_ptr<IRenderer> createRendererVk();



}