

addTarget(triangle-renderer
TYPE
    LIB
SOURCES
    renderer/renderer.h
    renderer/renderer-vk.cpp
SOURCES_WIN
    renderer/renderer-d3d.cpp
SOURCES_MAC
    renderer/renderer-metal.cpp
SHADERS_D3D
    renderer/shaders.hlsl
LIBRARIES
    d3d12
    d3dx12
    dxgi
    d3dcompiler
)



addTarget(triangle
TYPE
    EXE
SOURCES
    app.cpp
LIBRARIES
    triangle-renderer
    app
)