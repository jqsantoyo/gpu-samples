# GPU Samples (WIP)

GPU samples using D3D12 and Vulkan for Windows.

Each sample has a D3D12 backend and may have a Vulkan backend, switch by passing the command line argument "-vk".


| Sample                | Camera Control  | Shading                  |  |
|-----------------------|-----------------|--------------------------|--|
| **00-triangle**       |                 | vertex color             |  |
| **01-compute**        |                 |                          |  |
| **02-objects**        | ✓               | vertex color             |  |
| **03-texture**        | ✓               | diffuse texture          |  |
| **04-forward**        | ✓               |                          |  |

| Libraries         | Internal |
|-------------------|----------|
| utils             | ✓        |
| app               | ✓        |
| camera            | ✓        |
| scene             | ✓        |
| assets            | ✓        |
| utilsVk           | ✓        |
| utilsD3D          | ✓        |
| renderer          | ✓        |
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
.build/debug/00-triangle/00-triangle.exe            // D3D12
.build/debug//00-triangle/00-triangle.exe -vk       // Vulkan

cmake --preset=release
cmake --build --preset=release
.build/release/00-triangle/00-triangle.exe          // D3D12
.build/release/00-triangle/00-triangle.exe -vk      // Vulkan

cmake --preset=vs
cmake --build --preset=vs --config=Debug            // or Release or RelWithDebInfo
.build/vs/Debug/00-triangle/00-triangle.exe         // D3D12
.build/vs/Debug/00-triangle/00-triangle.exe -vk     // Vulkan
```




## Notes

```
measure-command { cmake --preset=debug --fresh; cmake --build --preset=debug --clean-first }
```
