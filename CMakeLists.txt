cmake_minimum_required(VERSION 3.29)
project(gpuSamples)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(FetchContent)
include(base)

message(STATUS "----------------------------------------")
message(STATUS "Project:   GpuSamples")
message(STATUS "System:    ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")


set(CMAKE_BUILD_RPATH "$ENV{VULKAN_SDK}/macOS/lib")

addImported(vulkan
    INC_WIN $ENV{VULKAN_SDK}/Windows/include
    INC_MAC $ENV{VULKAN_SDK}/macOS/include
    LIB_WIN $ENV{VULKAN_SDK}/Windows/Lib/vulkan-1.lib
    LIB_MAC $ENV{VULKAN_SDK}/macOS/lib/libvulkan.1.4.341.dylib
)
addImported(d3d12 WIN
    INC_DIR $ENV{WindowsSdkDir}/Include/$ENV{WindowsSdkVersion}/um
    LIB     $ENV{WindowsSdkDir}/Lib/$ENV{WindowsSdkVersion}/um/x64/D3d12.lib
)
addImported(dxgi WIN
    INC_DIR $ENV{WindowsSdkDir}/Include/$ENV{WindowsSdkVersion}/shared
    LIB     $ENV{WindowsSdkDir}/Lib/$ENV{WindowsSdkVersion}/um/x64/dxgi.lib
)
addImported(d3dcompiler WIN
    INC_DIR $ENV{WindowsSdkDir}/Include/$ENV{WindowsSdkVersion}/shared
    LIB     $ENV{WindowsSdkDir}/Lib/$ENV{WindowsSdkVersion}/um/x64/d3dcompiler.lib
)
addExternal(tinygltf URL https://github.com/syoyo/tinygltf.git TAG v2.9.7)
# addExternal(glm URL https://github.com/g-truc/glm.git TAG 1.0.3)
# -DGLM_BUILD_TESTS=OFF
# -DBUILD_SHARED_LIBS=OFF

addInterface(d3dx12 WIN)

addInterface(input         )
addInterface(renderer      )
addInterface(metal-cpp     )
addLibrary  (utils         )
addLibrary  (camera        LIBS input)
addLibrary  (scene         LIBS renderer)
addLibrary  (assets        LIBS tinygltf utils renderer scene)
addLibrary  (app           LIBS input renderer LIBS_MAC metal-cpp)
addLibrary  (utilsVk       LIBS vulkan utils)
addLibrary  (utilsD3D  WIN LIBS utils d3d12 dxgi d3dcompiler d3dx12)

# addSample(00-info         LIBS utilsVk          LIBS_WIN utilsD3D LIBS_MAC metal-cpp FRAMES Metal Foundation QuartzCore AppKit)
addSample(01-window       LIBS utilsVk app renderer LIBS_WIN utilsD3D LIBS_MAC metal-cpp FRAMES Metal Foundation QuartzCore AppKit)
# addSample(02-compute    LIBS utilsVk utilsD3D)
# addSample(03-triangle   LIBS app utils renderer input utilsVk utilsD3D)
# addSample(04-objects    LIBS app utils renderer assets scene camera input utilsVk utilsD3D)
