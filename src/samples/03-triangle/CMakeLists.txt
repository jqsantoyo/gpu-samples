

# addTarget(triangle-renderer
# TYPE
#     EXE
# SOURCES
#     main.cpp
#     renderer.h
# SOURCES_WIN
#     d3d/renderer.cpp
# SOURCES_MAC
#     mtl/renderer.cpp
# SHADERS_D3D
#     d3d/shaders.hlsl
# LIBRARIES
#     d3d12
#     d3dx12
#     dxgi
#     d3dcompiler
#     app
# )



# addTarget(triangle-renderer-vk
# TYPE
#     EXE
# SOURCES
#     main.cpp
#     renderer.h
#     vk/renderer.cpp
# # SHADERS_VK
# #     vk/shaders.hlsl
# LIBRARIES
#     app
# )
