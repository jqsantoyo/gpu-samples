cmake_minimum_required(VERSION 3.29)
project(gpuSamples)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(base)

include(modules/d3d12)
#add_subdirectory(src/triangle-vk)
add_subdirectory(src/triangle-d3d)