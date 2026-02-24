# GPU Samples (WIP)

GPU samples using D3D12 and Vulkan for Windows.

Each sample defines D3D12 & Vulkan executables, the Vulkan builds have a `-vk` suffix.


| Sample                | Camera Control  | Shading                  |  |
|-----------------------|-----------------|--------------------------|--|
| **00-info**           |                 |                          |  |
| **01-window**         |                 |                          |  |
| **02-compute**        |                 |                          |  |
| **03-triangle**       |                 | vertex color             |  |
| **04-objects**        | ✓               | vertex color             |  |

| Libraries         | Internal |
|-------------------|----------|
| utils             | ✓        |
| input             | ✓        |
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
| tinygltf          |          |


## Build and Run

Ensure the following environment variables are defined:
```
WINDOWS_SDK         = <windows sdk installation>
WINDOWS_SDK_VERSION = <windows sdk version>
VULKAN_SDK          = <vulkan sdk installation>
```

Use cmake to build the project. 2 presets are provided:
* `debug`
* `release`

In `vscode` select the preset and build with `F7`, then select a launch configuration in `Run and Debug` and press `F5`.

Or enter in the command line:
```
cmake --preset=debug
cmake --build --preset=debug
.build/debug/src/samples/01-info/01-info.exe        // D3D12
.build/debug/src/samples/01-info/01-info-vk.exe     // Vulkan
```



## Notes

```
measure-command { cmake --preset=debug --fresh; cmake --build --preset=debug --clean-first }
```
