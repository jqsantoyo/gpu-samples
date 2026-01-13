cmake_minimum_required(VERSION 3.29)
project(gpuSamples)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(base)

include(modules/d3d12)
include(modules/vulkan)
add_subdirectory(src/libs/d3dx12)
add_subdirectory(src/libs/app)
add_subdirectory(src/samples/01-hello)
add_subdirectory(src/samples/02-hello-compute)
add_subdirectory(src/samples/03-triangle)


