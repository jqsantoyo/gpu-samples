#pragma once
#include <gpu/gpu.h>
#include <utils/utils.h>
#include <memory>
#include <vector>

namespace gpu {



struct StaticBuffer     { int idx; };
struct Mesh             { int idx; };
struct MaterialTexture  { int idx; };
struct Material         { int idx; };

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

struct MeshDesc {
    StaticBuffer     staticBuffer;
    size_t           vCount;
    Format           formatIndices;
    size_t           offsetIndices;
    size_t           offsetPosition;
    size_t           offsetNormal;
    size_t           offsetUv;
    size_t           offsetTangent;
    size_t           offsetColor;
    uint32_t         sizeIndices;
    uint32_t         sizePosition;
    uint32_t         sizeNormal;
    uint32_t         sizeUv;
    uint32_t         sizeTangent;
    uint32_t         sizeColor;
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

struct RendererBaseDesc {
    Backend backend              = DirectX;
    void*   window               = nullptr;
    uvec2   windowSize           = { 512, 512 };
    int     staticBufferCount    = 50;
    int     meshCount            = 100;
    int     materialTextureCount = 100;
    int     materialCount        = 50;
    int     frameMemory          = 1000000;
};

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void init(const RendererBaseDesc& desc) = 0;
    virtual void terminate() = 0;
    virtual void reset() = 0;
    virtual void resize(int width, int height) = 0;
    virtual void render(const RenderView& view) = 0;
    virtual void wait() = 0;

    virtual StaticBuffer    create(const char* name, uint32_t size, const uint8_t* data) = 0;
    virtual Mesh            create(const MeshDesc& desc) = 0;
    virtual MaterialTexture create(const char* name, const uint8_t* data, uint32_t size) = 0;
    virtual Material        create(const char* name, MaterialDesc& desc) = 0;
    virtual void            destroy(StaticBuffer staticBuffer) = 0;
    virtual void            destroy(MaterialTexture materialTexture) = 0;
    virtual void            destroy(Material material) = 0;
};





}