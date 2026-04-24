# GPU Samples (WIP)

Ongoing exploration of rendering and compute topics using D3D12 and Vulkan.

Each sample supports both backends, using D3D12 by default, or using Vulkan when passing `-vk`.


| Render sample         | Shading      | Camera | Lights | Shadows | D3D12 status | Vulkan status |
|-----------------------|--------------|--------|------- |---------|--------------|---------------|
| **r0-color**          | vertex color | ✓      |        |         | ✓            | WIP           |
| **r1-forward**        | PBR          | ✓      | ✓      | ✓       | ✓            | ...           |


| Compute sample       | D3D12 status | Vulkan status |
|----------------------|--------------|---------------|
| **c0-compute**       | WIP          | ...           |


| Libraries         | Internal |
|-------------------|----------|
| utils             | ✓        |
| utilsVk           | ✓        |
| utilsD3D          | ✓        |
| vulkan            |          |
| d3d12             |          |
| dxgi              |          |
| d3dcompiler       |          |
| d3dx12            |          |
| ddsTextureLoader  |          |
| pixEvents         |          |
| tinygltf          |          |

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







### Renderer

The `IRenderer` interface is the contract all renderer samples in this repo comply with, for uniformity.
It also lets us opaquely implement both a D3D12 and a Vulkan backend, selectable at runtime.
The most important method is  `IRenderer::render`, which takes a `RenderView` argument that describes the contents and settings of the scene to be rendered.
The caller extracts and passes a `RenderView` to the renderer from the above mentioned `Scene` object, but it could come from anywhere, so the renderer is not coupled with `Scene`.

The repo started with bare API code in a hello world sample but evolved into utilities which greatly simplify the definition of each renderer, described below:

* Factory-Device-Swapchain / Instance-PhysicalDevice-Device-Surface-Swapchain: General API setup.
* FrameControl: Frames in flight management, with a frame memory allocator.
* Shader: Shader loading from file.
* MeshRegistry: Manages the creation of vertex/index buffer data.
* TextureRegistry: Manages the creation of texture data.
* Pipelines: Some reusable pipeline/root definitions (might take them back to each sample)

... *more documentation soon* ...




## Notes

```
measure-command { cmake --preset=debug --fresh; cmake --build --preset=debug --clean-first }
```
