# GPU Samples (WIP)

Ongoing exploration of rendering and compute topics using D3D12 and Vulkan.

Each sample supports both backends, using D3D12 by default, or using Vulkan when passing `-vk`.


| Render sample         | Shading      | Camera | Lights | Shadows | D3D12 status | Vulkan status |
|-----------------------|--------------|--------|------- |---------|--------------|---------------|
| **r0-color**          | vertex color | ✓      |        |         | ✓            | ✓             |
| **r1-forward**        | PBR          | ✓      | ✓      | ✓       | ✓            | ...           |


| Compute sample       | D3D12 status | Vulkan status |
|----------------------|--------------|---------------|
| **c0-compute**       | WIP          | ...           |


| Library            | Internal |
|--------------------|----------|
| app                | ✓        |
| utils              | ✓        |
| gpu                | ✓        |
| renderer           | ✓        |
| scene              | ✓        |
| vulkan             |          |
| d3d12              |          |
| dxgi               |          |
| d3dcompiler        |          |
| d3dx12             |          |
| ddsTextureLoader   |          |
| pixEvents          |          |
| tinygltf           |          |


| Asset         | Original Author | Original asset | License | Notes |
| ------------- |---------------- | -------------- | ------- | ----- |
| damagedHelmet | [theblueturtle_](https://sketchfab.com/leonardo-carrion) | [Battle Damaged Sci-fi Helmet - PBR](https://sketchfab.com/models/b81008d513954189a063ff901f7abfe4) | [CC Attribution-NonCommercial](https://creativecommons.org/licenses/)  | Reimported in Blender and modified. |
| ponyCar       | [Slava Z.](https://sketchfab.com/slava)                  | [Pony Cartoon](https://sketchfab.com/3d-models/pony-cartoon-885d9f60b3a9429bb4077cfac5653cf9)       | [CC-BY-4.0](http://creativecommons.org/licenses/by/4.0/).              | Reimported in Blender and modified. |
| ground        | Textures from [AmbientCG](https://ambientcg.com/)          | [Road 009 C](https://ambientcg.com/view?id=Road009C)                                                | [CC0](https://docs.ambientcg.com/license/)                             | Authored model in Blender           |


### Previews
![r0-color](docs/r0-color.jpg)

![r1-forward-0](docs/r1-forward-0.jpg)

![r1-forward-1](docs/r1-forward-1.jpg)



## Build and Run

Ensure `Git LFS` is installed and initialized before cloning.

Ensure the following programs are installed:
```
texconv
```

Ensure the following environment variables are defined:
```
WINDOWS_SDK         = <windows sdk installation>
WINDOWS_SDK_VERSION = <windows sdk version>
VULKAN_SDK          = <vulkan sdk installation>
```

Use cmake to build the project. 3 presets are provided:
* `debug` (Ninja)
* `release` (Ninja)
* `vs` (Visual Studio 18 2026)

In the command line enter:
```
cmake --preset=debug
cmake --build --preset=debug
.build/debug/r0-triangle/r0-triangle.exe            // D3D12
.build/debug/r0-triangle/r0-triangle.exe -vk        // Vulkan

cmake --preset=release
cmake --build --preset=release
.build/release/r0-triangle/r0-triangle.exe          // D3D12
.build/release/r0-triangle/r0-triangle.exe -vk      // Vulkan

cmake --preset=vs
cmake --build --preset=vs --config=Debug            // or Release or RelWithDebInfo
.build/vs/Debug/r0-triangle/r0-triangle.exe         // D3D12
.build/vs/Debug/r0-triangle/r0-triangle.exe -vk     // Vulkan
```


## Architectural Notes

### Cmake
This repo follows the modern cmake practice of using targets and presets. In addition, utility functions are created to declaratively define the various targets:
* `addImported`/`addImportedDynamic`: defines an imported static/dynamic library.
* `addExternal`: defines targets acquired with FetchContent. Could be static/dynamic, could be 1 or many, depends on the repo.
* `addLibrary`/`addInterface`: defines a static/interface library.
* `addSample`: defines an executable target.

While these are fairly general purpose, `addSample` does perform tasks specific to the samples in this repo, like shader/asset processing and GPU API linking.

Some of these functions glob files, which some consider bad practice. However, `CONFIGURE_DEPENDS` helps mitigate the cons: it rebuilds the project upon file changes after a first build. All in all, for the scope of this project I consider globbing does more good than bad.

Defining a project in this way is scalable in 2 ways:
1. As the project grows and needs more complex descriptions, the functions can be modified to cater to the new requirements.
For example, adding a platform argument if macOS is to be supported, to exclude targets based on platform:
    ```
    addImported(d3d12     WIN INC_DIR ... LIB ...) # Only imports in windows
    addImported(metal-cpp MAC INC_DIR ... LIB ...) # Only imports in macOS (although we know metal-cpp is not usually imported)
    ```

2. Even when the functions are not further adpated to new requirements, one can always write extra cmake logic for a target outside the functions, without conflict. For example:
    ```
    # Extra switch definition outside of addExternal (I don't want to mess with passing said data into the function for now)
    set(TINYGLTF_BUILD_LOADER_EXAMPLE   OFF CACHE BOOL "" FORCE)
    addExternal(tinygltf URL https://github.com/syoyo/tinygltf.git TAG v2.9.7)

    # Extra piece of build definition (given its uniqueness, I just write this at the top level)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT r1-forward)
    ```

Overall, by hiding away cmake logic and sticking to declarative descriptions, these functions result in a very thin and scalable project description. So much so, that there is only 1 cmake listing file.

### Scene

The `Scene` class holds the simulation state.
It's mostly a data class with public structure of arrays (SOA) providing direct and efficient data access to operations defined outside the class.
This design follows a DOD approach, most importantly to have a data layout that is easy for the renderer to consume, but it's not a full ECS.
Further, the SOA implementation is minimal to keep it grounded to the current requirements
(no single removal, no pool behavior, no persistent identifiers, not templated, redundant usage of `std::vector`, etc.).


Current SOAs and operations are:

| SOA          | Arrays      |
|--------------|------------|
| `Cameras`    | `CameraPos`<br>`Camera`   |
| `Lights`     | `Light`    |
| `Objects`    | `Trs`<br>`mat4`<br> `Model` |


| Operations   | Array access |
|--------------|--------------|
| programmatic scene definition  | `all` |
| scene definition from `gltf`  | `all` |
| camera control                | `Cameras::CameraPos`<br>`Cameras::Camera` |
| rendering of objects          | `Cameras::Camera`<br>`Lights::Light`<br>`Objects::mat4`<br>`Objects::Model` |

The current design is scalable in 2 ways:
1. We can massively increase the number of elements in the scene without compromising performance.
2. We can add new operations and data components without conflict to other parts of the system. For example, some operations we might want to add in some augmented context (e.g. in a game project) are:

    | Operations   | Array access | Note |
    |--------------|--------------|------|
    | scene definition from a custom file format | `all` | Authored independently from `gltf` loader, no monolith risk. |
    | light flicker control                      | `Lights::LightFlicker`<br>`Lights::Light` | Added `Lights::LightFlicker`, but renderer work is not affected by it |
    | object animation                           | `Objects::Animator`<br>`Objects::Trs` | Added `Objects::Animator`, but renderer work is not affected by it |







### Gpu

The repo started with bare API code in a hello world sample, but such code is highly unmantainable and verbose.
Later on, utilities for D3D12 and Vulkan were gradually created, evolving into an `RHI`, implemented by the `GpuD3D/GpuVk` classes in the `gpu` library. Said classes initialize the device and let the user create resources (queues, commands, swapchain, root signatures, pipelines, buffers, textures, and views). Each of these objects is associated with a handle for persistent reference. `GpuD3D/GpuVk` is exposed through an API agnostic interface: `Gpu`.
This `RHI` was first created with D3D12 in mind, and the current Vulkan implementation is missing some components.
Next steps are bumping up to Vulkan 1.3/1.4, completing the missing Vulkan bits and improving a shared binding model.

Usage examples:
```
std::unique_ptr<IGpu> gpu          = createGpu({ .window = window }, backend);
Queue                 queue        = gpu->createQueue();
Swapchain             swapchain    = gpu->createSwapchain(queue, window, windowSize.x, windowSize.y, frameCount);
Command               mainCommand  = gpu->createCommand();
Buffer                uploadBuffer = gpu->createBuffer("UploadBuffer", BufferUpload, 2048 * 2048 * 4);
```

```
gpu->wait           (queue, cmd);
gpu->begin          (cmd);
gpu->viewport       (cmd, viewport);
gpu->scissor        (cmd, scissorRect);
gpu->barrier        (cmd, swapTexture, State::RenderTarget);
gpu->targets        (cmd, swapRenderView, depthView);
gpu->clear          (cmd, swapRenderView, clearColor, depthView, 1, 0);
gpu->pipeline       (cmd, pso);
gpu->graphicsRoot   (cmd, root);
gpu->topology       (cmd, PrimitiveTopology::TriangleList);
gpu->vertexBuffer   (cmd, 0, m.position);
gpu->draw           (cmd, m.vCount, 1, 0, 0);
gpu->barrier        (cmd, swapTexture, State::Present);
gpu->end            (cmd);
gpu->execute        (queue, cmd);
gpu->present        (swapchain, false);
gpu->signal         (queue, cmd);
```


### Renderer

Each sample contains an API agnostic renderer, leveraging  a `Gpu` object, and complying with the `IRenderer` interface.

```
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
```

In particular:
* Resource management is exposed with `IRenderer::create/destroy` methods, and it is the responsibility of the caller to determine their exact lifetimes.
* `IRenderer::render` issues draw calls by traversing a `RenderView` object that describes the scene. The caller builds a `RenderView` from the above mentioned `Scene` object and passes it to the renderer, but it could come from anywhere, so the renderer is not coupled with `Scene`.

Additionally, each renderer inherits from `RendererBase:IRenderer`, to reuse code around gpu setup, resource creation and frame scheduling.
The split between `IRenderer` and `RendererBase` is done to keep `IRenderer` opaque and to keep `RendererBase`'s implementation visible only to derived classes.
In a given project, `RendererBase` would be superfluous, but here it is favourable due to having multiple renderers.


... *more documentation soon* ...




## Notes

```
measure-command { cmake --preset=debug --fresh; cmake --build --preset=debug --clean-first }
```
