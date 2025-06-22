

message(STATUS "Direct3D12: Windows SDK path: $ENV{WindowsSdkDir}")
message(STATUS "Direct3D12: Windows SDK version: $ENV{WindowsSdkVersion}")

add_library(d3d12 STATIC IMPORTED)
set_target_properties(d3d12 PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES $ENV{WindowsSdkDir}/Include/$ENV{WindowsSdkVersion}/um
  IMPORTED_LOCATION             $ENV{WindowsSdkDir}/Lib/$ENV{WindowsSdkVersion}/um/x64/D3d12.lib
)

add_library(dxgi STATIC IMPORTED)
set_target_properties(dxgi PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES $ENV{WindowsSdkDir}/Include/$ENV{WindowsSdkVersion}/shared
  IMPORTED_LOCATION             $ENV{WindowsSdkDir}/Lib/$ENV{WindowsSdkVersion}/um/x64/dxgi.lib
)

add_library(d3dcompiler STATIC IMPORTED)
set_target_properties(d3dcompiler PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES $ENV{WindowsSdkDir}/Include/$ENV{WindowsSdkVersion}/shared
  IMPORTED_LOCATION             $ENV{WindowsSdkDir}/Lib/$ENV{WindowsSdkVersion}/um/x64/d3dcompiler.lib
)