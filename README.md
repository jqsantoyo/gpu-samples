# GPU Samples

GPU samples using Direct3D, Vulkan, Metal.

| Sample |      |
|--------|------|
| first  |      |
| hello  |      |


### Build and Run

To build enter:
```
# On windows
cmake --preset=win-x64-debug
cmake --preset=win-x64-release
cmake --build --preset=win-x64-debug
cmake --build --preset=win-x64-release

# On mac
cmake --preset=mac-arm64-debug
cmake --preset=mac-arm64-release
cmake --build --preset=mac-arm64-debug
cmake --build --preset=mac-arm64-release
```

Or go into VScode or Visual Studio and build with their cmake support out of the box.


The first sample has separate builds for Direct3D, Vulkan and Metal. So to run enter:
```
# On windows
.build/win-x64-debug/src/samples/hello/hello.exe      // Direct3D build
.build/win-x64-debug/src/samples/hello/hello-vk.exe   // Vulkan build

# On mac
.build/mac-arm64-debug/src/samples/hello/hello          // Metal build
.build/mac-arm64-debug/src/samples/hello/hello-vk       // Vulkan build
```

The rest of samples select between Direct3D/Vulkan and Metal/Vulkan at runtime via a command line argument. So to run enter:
```
# On windows
.build/win-x64-debug/src/samples/triangle/triangle.exe      // selects Direct3D
.build/win-x64-debug/src/samples/triangle/triangle.exe -vk  // selects Vulkan

# On mac
.build/mac-arm64-debug/src/samples/triangle/triangle          // selects Metal
.build/mac-arm64-debug/src/samples/triangle/triangle -vk      // selects Vulkan
```

You can also run the samples directly from vscode, by selecting a launch configuration in `Run and Debug` and pressing `F5`.


### FAQ

```
powershell measure-command { cmake --preset=win-x64-debug --fresh; cmake --build --preset=win-x64-debug --clean-first }
```
