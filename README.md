# GPU Samples

GPU samples using Direct3D, Vulkan, & Metal.

Each sample defines D3D & Vulkan executables for Windows, and Metal & Vulkan executables for MacOS; the Vulkan builds have a `-vk` suffix.

The samples leverage the library `app` for easier program setup.

| Sample         | Camera Control  | Shading                  |  |
|----------------|-----------------|--------------------------|--|
| **01-hello**   |                 |                          |
| **02-color**   | ✓               | vertex color             |
| **03-diffuse** | ✓               | diffuse + light + shadow |



## Build and Run

Use cmake to build the project. 4 presets are provided:
* `win-x64-debug`
* `win-x64-release`
* `mac-arm64-debug`
* `mac-arm64-release`

In `vscode` select the preset and build with `F7`, then select a launch configuration in `Run and Debug` and press `F5`.

Or enter in the command line:
```
# On windows
cmake --preset=win-x64-debug
cmake --build --preset=win-x64-debug
.build/win-x64-debug/src/samples/01-hello/01-hello.exe        // Direct3D build
.build/win-x64-debug/src/samples/01-hello/01-hello-vk.exe     // Vulkan build

# On mac
cmake --preset=mac-arm64-debug
cmake --build --preset=mac-arm64-debug
.build/mac-arm64-debug/src/samples/01-hello/01-hello          // Metal build
.build/mac-arm64-debug/src/samples/01-hello/01-hello-vk       // Vulkan build
```




## Notes

```
measure-command { cmake --preset=win-x64-debug --fresh; cmake --build --preset=win-x64-debug --clean-first }
```
