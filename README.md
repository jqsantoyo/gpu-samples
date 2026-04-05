# GPU Samples (WIP)

Ongoing exploration of rendering and compute topics using D3D12 and Vulkan.

Each sample supports both backends, using D3D12 by default, or using Vulkan when passing `-vk`.



| Render sample         | Camera | Shading      | Lights | D3D12 status | Vulkan status |
|-----------------------|--------|--------------|--------|--------------|---------------|
| **r0-color**          | ✓      | vertex color |        | ✓            | ✓            |
| **r1-forward**        | ✓      | pbr          | ✓      | WIP          | ...          |



| Compute sample        |                 |                          | D3D12 status | Vulkan status |
|-----------------------|-----------------|--------------------------|--------------|---------------|
| **c0-compute**        |                 |                          | ...          | ...           |



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
.build/debug/r0-triangle/r0-triangle.exe -vk       // Vulkan

cmake --preset=release
cmake --build --preset=release
.build/release/r0-triangle/r0-triangle.exe          // D3D12
.build/release/r0-triangle/r0-triangle.exe -vk      // Vulkan

cmake --preset=vs
cmake --build --preset=vs --config=Debug            // or Release or RelWithDebInfo
.build/vs/Debug/r0-triangle/r0-triangle.exe         // D3D12
.build/vs/Debug/r0-triangle/r0-triangle.exe -vk     // Vulkan
```




## Notes

```
measure-command { cmake --preset=debug --fresh; cmake --build --preset=debug --clean-first }
```
