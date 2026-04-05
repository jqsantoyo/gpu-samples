cmake_minimum_required(VERSION 4.0.0)
project(gpuSamples)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(FetchContent)
include(base)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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
addImportedDynamic(pixEvents
    INC_DIR ${CMAKE_SOURCE_DIR}/src/libs/pixEvents/Include
    LIB     ${CMAKE_SOURCE_DIR}/src/libs/pixEvents/bin/x64/WinPixEventRuntime.dll
    IMPLIB  ${CMAKE_SOURCE_DIR}/src/libs/pixEvents/bin/x64/WinPixEventRuntime.lib
)

set(TINYGLTF_BUILD_LOADER_EXAMPLE   OFF CACHE BOOL "" FORCE)
# set(TINYGLTF_HEADER_ONLY            ON  CACHE BOOL "" FORCE)
addExternal(tinygltf URL https://github.com/syoyo/tinygltf.git TAG v2.9.7)
# addExternal(glm URL https://github.com/g-truc/glm.git TAG 1.0.3)
# -DGLM_BUILD_TESTS=OFF
# -DBUILD_SHARED_LIBS=OFF

addInterface(d3dx12)
addLibrary  (ddsTextureLoader   LIBS d3dx12)
addLibrary  (utils              LIBS tinygltf)
addLibrary  (utilsVk            LIBS utils vulkan)
addLibrary  (utilsD3D           LIBS utils d3d12 dxgi d3dcompiler d3dx12 ddsTextureLoader)

addSample   (r0-color           LIBS utils pixEvents)
addSample   (r1-forward         LIBS utils pixEvents ddsTextureLoader)
addSample   (c0-compute         LIBS utils)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT r3-forward)