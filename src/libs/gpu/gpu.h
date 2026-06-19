#pragma once
#include <utils/utils.h>
#include <functional>
#include <span>

#define GUARDHR(x) if (FAILED(x)) {  printf("Error: "#x"\n"); return false; }

namespace gpu {


struct Queue       { int idx; };
struct Command     { int idx; };
struct Root        { int idx; };
struct Pipeline    { int idx; };
struct Swapchain   { int idx; };
struct Buffer      { int idx; };
struct Texture     { int idx; };
struct TextureView { int idx; };

enum class CullMode {
    None,
    Front,
    Back,
};

enum class FillMode {
    Wireframe,
    Solid
};

enum class PrimitiveTopologyType {
    Undefined,
    Point,
    Line,
    Triangle,
    Patch,
};

enum class PrimitiveTopology {
    Undefined,
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    TriangleFan,
    LineListAdj,
    LineStripAdj,
    TriangleListAdj,
    TriangleStripAdj,
};

enum class Format {
    Unknown,

    D16,            // depth
    D24S8,          // depth
    D32,            // depth
    D32S8,          // depth

    R16f,
    RG16f,
    RGBA16f,
    R32f,
    RG32f,
    RGB32f,
    RGBA32f,
    R11G11B10f,

    R8u,
    RG8u,
    RGBA8u,
    R16u,
    RG16u,
    RGBA16u,
    R32u,
    RG32u,
    RGB32u,
    RGBA32u,
    R10G10B10A2u,

    R8s,
    RG8s,
    RGBA8s,
    R16s,
    RG16s,
    RGBA16s,
    R32s,
    RG32s,
    RGB32s,
    RGBA32s,

    R8un,
    RG8un,
    RGBA8un,        // color
    BGRA8un,        // color
    RGBA8un_sRGB,   // color
    BGRA8un_sRGB,   // color
    R16un,
    RG16un,
    RGBA16un,
    R10G10B10A2un,
    
    R8sn,
    RG8sn,
    RGBA8sn,
    R16sn,
    RG16sn,
    RGBA16sn,
};

enum class State {
    Common,
    RenderTarget,
    UnorderedAccess,
    DepthWrite,
    DepthRead,
    PSRead,
    CopyDest,
    CopySource,
    GenericRead,
    Present,
};

struct VertexBufferView {
    Buffer      buffer;
    size_t      offset;
    uint32_t    size;
    uint32_t    stride;
};

struct IndexBufferView {
    Buffer      buffer;
    size_t      offset;
    uint32_t    size;
    Format      format;
};

struct Rect {
    uint32_t left;
    uint32_t top;
    uint32_t right;
    uint32_t bottom;
};

struct Viewport {
    float topLeftX;
    float topLeftY;
    float width;
    float height;
    float minDepth;
    float maxDepth;
};

enum class Filter {
    Point,
    Linear,
    Anisotropic,
};

enum class AddressMode {
    Wrap,
    Mirror,
    Clamp,
    Border,
    Mirror_once,
};

enum class SamplerBorderColor {
    TransparentBlack,
    OpaqueBlack,
    OpaqueWhite,
};

struct Sampler {
    uint32_t            shaderRegister;
    Filter              filter;
    AddressMode         addressU        = AddressMode::Wrap;
    AddressMode         addressV        = AddressMode::Wrap;
    AddressMode         addressW        = AddressMode::Wrap;
    SamplerBorderColor  borderColor     = SamplerBorderColor::OpaqueWhite;
    uint32_t            registerSpace   = 0;
};


enum class RootBinding {
    Constant,
    Read,
    ReadWrite,
};

enum class RootParamType {
    Binding,
    Data,
    Table,
};

struct RootRange {
    RootBinding binding;
    uint32_t    bindingCount;
    uint32_t    baseShaderRegister;
    uint32_t    registerSpace = 0;
    static RootRange range(RootBinding binding, uint32_t bindingCount, uint32_t baseShaderRegister, uint32_t space = 0);
};

struct RootParam {
    RootParamType           paramType;
    RootBinding             bindingType;
    uint32_t                dataCount;
    uint32_t                shaderRegister;
    uint32_t                registerSpace;
    std::vector<RootRange>  tableRanges;
    static RootParam binding(RootBinding binding, uint32_t shaderRegister, uint32_t space = 0);
    static RootParam data   (uint32_t count, uint32_t shaderRegister, uint32_t space = 0);
    static RootParam table  (std::initializer_list<RootRange> ranges);
};


struct InputElement {
    const char* semanticName;
    uint32_t    semanticIndex;
    Format      format;
    uint32_t    inputSlot;
    uint32_t    alignedOffset;
};

struct RenderTargetDesc {
    Format  format;
    bool    enableBlend;
};

struct PsoGraphicsDesc {
    const char*                     vs;
    const char*                     ps;
    FillMode                        fillMode        = FillMode::Solid;
    CullMode                        cullMode        = CullMode::None;
    bool                            enableDepth     = true;
    bool                            enableStencil   = false;
    PrimitiveTopologyType           topologyType    = PrimitiveTopologyType::Triangle;
    std::vector<InputElement>       inputElements;
    std::vector<RenderTargetDesc>   renderTargets   = { { Format::RGBA8un, true } };
    Format                          dsvFormat       = Format::D32;
    uint32_t                        samples         = 1;
};

enum QueueType {
    QueueGraphics,
    QueueCompute,
    QueueCopy
};

enum BufferUsage {
    BufferUpload,
    BufferDefault,
    BufferWrite,
    BufferRead,
};

enum class TextureUsage {
    Default         = 0,
    DepthStencil    = 1u << 1,
    RenderTarget    = 1u << 2,
    Write           = 1u << 3,
};

inline constexpr TextureUsage operator&(TextureUsage lhs, TextureUsage rhs) {
    using T = std::underlying_type_t<TextureUsage>;
    return static_cast<TextureUsage>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline constexpr TextureUsage operator|(TextureUsage lhs, TextureUsage rhs) {
    using T = std::underlying_type_t<TextureUsage>;
    return static_cast<TextureUsage>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline constexpr bool hasFlag(TextureUsage v, TextureUsage flag) {
    return (v & flag) == flag;
}

class ClearValue {
public:
    ClearValue() : color(0, 0, 0, 0), depth(0), stencil(0) {}
    ClearValue(float r, float g, float b, float a) : color(r, g, b, a), depth(0), stencil(0) {}
    ClearValue(float d, uint8_t s) : color(0, 0, 0, 0), depth(d), stencil(s) {}
    vec4 color;
    float depth;
    uint8_t stencil;
};

struct SwapTarget {
    Texture     texture;
    TextureView renderView;
};

struct ResourceData {
    const void* data;
    int64_t     rowPitch;
    int64_t     slicePitch;
};

struct GpuDesc {
    uint32_t queueCount     = 8;
    uint32_t commandCount   = 8;
    uint32_t rootCount      = 8;
    uint32_t pipelineCount  = 8;
    uint32_t swapchainCount = 1;
    uint32_t bufferCount    = 256;
    uint32_t textureCount   = 512;
    uint32_t maxColorViews  = 512;
    uint32_t maxRenderViews = 16;
    uint32_t maxDepthViews  = 16;
};

class IGpu {
public:
    virtual             ~IGpu               () = default;
    virtual void        terminate           () = 0;
    virtual void        print               () = 0;
    virtual void        printErrors         () = 0;

    virtual Queue       createQueue         (QueueType type = QueueGraphics) = 0;
    virtual Command     createCommand       (QueueType type = QueueGraphics, uint32_t maxMemory = 0) = 0;
    virtual Root        createRoot          (const std::vector<RootParam>& params, const std::vector<Sampler>& samplers) = 0;
    virtual Pipeline    createPipeline      (Root root, const char* shader) = 0;
    virtual Pipeline    createPipeline      (Root root, PsoGraphicsDesc desc) = 0;
    virtual Swapchain   createSwapchain     (Queue& queue, void* window, uint32_t w, uint32_t h, uint32_t frameCount) = 0;
    virtual Buffer      createBuffer        (const char* name, BufferUsage usage, uint32_t size) = 0;
    virtual Texture     createTexture       (const char* name, const uint8_t* data, uint32_t size, std::vector<ResourceData>& subresources) = 0;
    virtual Texture     createTexture1      (TextureUsage usage, Format format, uvec3 dims, uint16_t mips = 1, ClearValue value = {}) = 0;
    virtual Texture     createTexture2      (const char* name, TextureUsage usage, Format format, uvec3 dims, uint16_t mips = 1, uint8_t samples = 1, ClearValue value = {}) = 0;
    virtual Texture     createTexture3      (TextureUsage usage, Format format, uvec3 dims, uint16_t mips = 1, ClearValue value = {}) = 0;
    virtual TextureView createTextureView   (Texture texture) = 0;
    virtual TextureView createDepthView     (Texture texture) = 0;
    virtual TextureView createRenderView    (Texture texture) = 0;
    virtual int         getTextureViewBind  (TextureView textureView) = 0;

    virtual void        destroy             (Buffer buffer) = 0;
    virtual void        destroy             (Texture texture) = 0;
    virtual void        destroy             (TextureView textureView) = 0;

    virtual void        execute             (Queue queue, Command& command) = 0;
    virtual void        signal              (Queue queue, Command& command) = 0;
    virtual void        wait                (Queue queue, Command& command) = 0;
    virtual void        signal              (Queue queue, uint64_t& value) = 0;
    virtual void        wait                (Queue queue, uint64_t value) = 0;
    virtual void        wait                (Queue queue) = 0;

    virtual void        begin               (Command command, Pipeline pipeline = { -1 }) = 0;
    virtual void        bindHeap            (Command command) = 0;
    virtual void        viewport            (Command command, Viewport& viewport) = 0;
    virtual void        scissor             (Command command, Rect& rect) = 0;
    virtual void        targets             (Command command, TextureView renderTexture, TextureView depthTexture) = 0;
    virtual void        clear               (Command command, TextureView renderTexture, vec4 color, TextureView depthTexture, float depth, uint8_t stencil) = 0;
    virtual void        pipeline            (Command command, Pipeline pipeline) = 0;
    virtual void        computeRoot         (Command command, Root& root) = 0;
    virtual void        computeSrv          (Command command, int idx, Buffer& buffer) = 0;
    virtual void        computeUav          (Command command, int idx, Buffer& buffer) = 0;
    virtual void        dispatch            (Command command, uint32_t groupsX, uint32_t  groupsY, uint32_t groupsZ) = 0;
    virtual void        graphicsRoot        (Command command, Root& root) = 0;
    virtual void        graphicsCbv         (Command command, int idx, const void* data, int dataSize) = 0;
    template<typename T>
            void        graphicsCbv         (Command command, int idx, const T& t) { graphicsCbv(command, idx, &t, align256(sizeof(T))); }
    virtual void        graphicsTable       (Command command, int idx, int descriptorIdx) = 0;
    virtual void        barrier             (Command command, Buffer buffer, State state) = 0;
    virtual void        barrier             (Command command, Texture texture, State state) = 0;
    virtual void        copy                (Command command, Buffer& dst, Buffer& src) = 0;
    virtual void        subresources        (Command command, Texture texture, Buffer uploadBuffer, int offset, int subresourceOffset, const std::vector<ResourceData>& subresources) = 0;
    virtual void        topology            (Command command, PrimitiveTopology primitiveTopology) = 0;
    virtual void        vertexBuffer        (Command command, uint32_t slot, VertexBufferView view) = 0;
    virtual void        indexBuffer         (Command command, IndexBufferView view) = 0;
    virtual void        drawIndexed         (Command command, uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, int baseVertexLocation, uint32_t startInstanceLocation) = 0;
    virtual void        draw                (Command command, uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) = 0;
    virtual void        end                 (Command command) = 0;


    virtual void        present             (Swapchain sw, bool vsync) = 0;
    virtual SwapTarget  next                (Swapchain sw) = 0;

    virtual void        write               (Buffer buffer, uint32_t offset, uint32_t size, const uint8_t* data) = 0;
    virtual void        read                (Buffer buffer, const std::function<void(uint8_t*)>& f) = 0;


};

void beginEvent(const char* fmt, ...);
void endEvent();


std::unique_ptr<IGpu> createGpu(const GpuDesc& desc = {});







}