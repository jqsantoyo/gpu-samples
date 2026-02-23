# GPU Samples

GPU samples using Direct3D, Vulkan, & Metal.

Each sample defines D3D & Vulkan executables for Windows, and Metal & Vulkan executables for MacOS; the Vulkan builds have a `-vk` suffix.


| Sample                | Camera Control  | Shading                  |  |
|-----------------------|-----------------|--------------------------|--|
| **01-info**           |                 |                          |  |
| **02-info-window**    |                 |                          |  |
| **03-compute**        |                 |                          |  |
| **04-triangle**       |                 | vertex color             |  |
| **05-objects**        | ✓               | vertex color             |  |

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
WindowsSdkDir   =<windows sdk installation>            // Windows only
VULKAN_SDK      =<vulkan sdk installation>
VK_ICD_FILENAMES=<MoltenVK_icd.json path>           // MacOS only
```

Use cmake to build the project. 2 presets are provided:
* `debug`
* `release`

In `vscode` select the preset and build with `F7`, then select a launch configuration in `Run and Debug` and press `F5`.

Or enter in the command line:
```
cmake --preset=debug
cmake --build --preset=debug
.build/debug/src/samples/01-info/01-info.exe        // Direct3D or Metal build
.build/debug/src/samples/01-info/01-info-vk.exe     // Vulkan build
```



## Notes

```
measure-command { cmake --preset=debug --fresh; cmake --build --preset=debug --clean-first }
```
