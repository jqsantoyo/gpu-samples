
message(STATUS "Vulkan SDK path: $ENV{VK_SDK_PATH}")

add_library(vulkan STATIC IMPORTED)
set_target_properties(vulkan PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES $ENV{VK_SDK_PATH}/Include
  IMPORTED_LOCATION             $ENV{VK_SDK_PATH}/Lib/vulkan-1.lib
)