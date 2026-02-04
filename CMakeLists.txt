cmake_minimum_required(VERSION 3.29)
project(gpuSamples)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(FetchContent)
include(base)

message(STATUS "----------------------------------------")
message(STATUS "Project:   GpuSamples")
message(STATUS "System:    ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")



addImported(vulkan
    INC_DIR $ENV{VK_SDK_PATH}/Include
    LIB     $ENV{VK_SDK_PATH}/Lib/vulkan-1.lib
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

addInterface(d3dx12 WIN)
addInterface(input)
addLibrary(utils)
addInterface(renderer WIN)
addLibrary(camera LIBS input)
addLibrary(scene LIBS renderer)
addLibrary(assets LIBS tinygltf utils renderer scene)
addLibrary(app LIBS input)
addLibrary(utilsVk LIBS vulkan)
addLibrary(utilsD3D LIBS utils d3d12 dxgi d3dcompiler d3dx12)

addSample(02-info    LIBS app renderer utilsVk utilsD3D)
addSample(02-compute LIBS utilsVk utilsD3D)
addSample(01-hello   LIBS app utils renderer input utilsVk utilsD3D)
addSample(03-color   LIBS app utils renderer assets scene camera input utilsVk utilsD3D)
