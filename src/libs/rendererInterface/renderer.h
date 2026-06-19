#pragma once
#include "renderUtils.h"
#include <utils/utils.h>
#include <memory>
#include <vector>

namespace gpu {


enum ViewMode {
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
    vec4            clearColor;
    ViewMode        fillMode;
    bool            enableShadows;
    int             lightCount;
    int             modelCount;
    const Camera*   camera;
    const Light*    lights;
    const mat4*     transforms;
    const Model*    models;
};


class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool init(void* window, uint32_t width, uint32_t height) = 0;
    virtual void terminate() = 0;
    virtual void reset() = 0;
    virtual void wait() = 0;
    virtual bool resize(int width, int height) = 0;
    virtual bool render(const RenderView& view) = 0;

    virtual Buffer          create(const char* name, uint32_t size, const uint8_t* data) = 0;
    virtual Mesh            create(const MeshData& desc) = 0;
    virtual MaterialTexture create(const char* name, const uint8_t* data, uint32_t size) = 0;
    virtual Material        create(const char* name, MaterialDesc& desc) = 0;
    virtual void            destroy(Buffer buffer) = 0;
    virtual void            destroy(MaterialTexture materialTexture) = 0;
    virtual void            destroy(Material material) = 0;
};



}