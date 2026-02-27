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
    INC_DIR $ENV{VULKAN_SDK}/include
    LIB     $ENV{VULKAN_SDK}/Lib/vulkan-1.lib
)
addImported(d3d12
    INC_DIR $ENV{WINDOWS_SDK}/Include/$ENV{WINDOWS_SDK_VERSION}/um
    LIB     $ENV{WINDOWS_SDK}/Lib/$ENV{WINDOWS_SDK_VERSION}/um/x64/D3d12.lib
)
addImported(dxgi
    INC_DIR $ENV{WINDOWS_SDK}/Include/$ENV{WINDOWS_SDK_VERSION}/shared
    LIB     $ENV{WINDOWS_SDK}/Lib/$ENV{WINDOWS_SDK_VERSION}/um/x64/dxgi.lib
)
addImported(d3dcompiler
    INC_DIR $ENV{WINDOWS_SDK}/Include/$ENV{WINDOWS_SDK_VERSION}/shared
    LIB     $ENV{WINDOWS_SDK}/Lib/$ENV{WINDOWS_SDK_VERSION}/um/x64/d3dcompiler.lib
)
addExternal(tinygltf URL https://github.com/syoyo/tinygltf.git TAG v2.9.7)
# addExternal(glm URL https://github.com/g-truc/glm.git TAG 1.0.3)
# -DGLM_BUILD_TESTS=OFF
# -DBUILD_SHARED_LIBS=OFF

addInterface(d3dx12)

addInterface(input      )
addInterface(renderer   )
addLibrary  (utils      )
addLibrary  (app        )
addLibrary  (camera     LIBS app)
addLibrary  (scene      LIBS renderer)
addLibrary  (assets     LIBS tinygltf utils renderer scene)
addLibrary  (utilsVk    LIBS utils vulkan)
addLibrary  (utilsD3D   LIBS utils d3d12 dxgi d3dcompiler d3dx12)

addSample(00-info       LIBS utilsVk utilsD3D)
addSample(01-window     LIBS utilsVk utilsD3D app renderer)
addSample(02-compute    LIBS utilsVk utilsD3D)
addSample(03-triangle   LIBS utilsVk utilsD3D app renderer utils)
addSample(04-objects    LIBS utilsVk utilsD3D app renderer utils assets scene camera)
