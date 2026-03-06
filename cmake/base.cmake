


if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "" FORCE)
endif()



# Prints a list.
function(printList)
    foreach(item IN LISTS ${ARGN})
        message(STATUS "  ${item}")
    endforeach()
endfunction()


# Adds an interface library target.
function(addImported targetName)
    set(options)
    set(oneArgs INC_DIR LIB)
    set(multiArgs)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    message(STATUS "----------------------------------------")
    message(STATUS "Library (Imported): ${targetName}")
    message(STATUS "  Include Dir: ${arg_INC_DIR}")
    message(STATUS "  Lib:         ${arg_LIB}")

    add_library          (${targetName} STATIC IMPORTED)
    set_target_properties(${targetName} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${arg_INC_DIR}
        IMPORTED_LOCATION             ${arg_LIB}
    )
endfunction()



# Adds an imported library target.
function(addInterface targetName)
    set(options)
    set(oneArgs)
    set(multiArgs)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    set(INC_DIR ${CMAKE_SOURCE_DIR}/src/libs/${targetName})
    
    message(STATUS "----------------------------------------")
    message(STATUS "Library (Interface): ${targetName}")
    message(STATUS "  Include Dir: ${INC_DIR}")
    add_library             (${targetName} INTERFACE)
    set_target_properties   (${targetName} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${INC_DIR})
endfunction()



# Adds an interface library from an external source, using FetchContent
function(addExternal targetName)
    set(options)
    set(oneArgs URL TAG)
    set(multiArgs)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    message(STATUS "----------------------------------------")
    message(STATUS "Library (External): ${targetName}")
    message(STATUS "  URL: ${arg_URL}")
    message(STATUS "  Tag: ${arg_TAG}")

    FetchContent_Declare(
        ${targetName}
        GIT_REPOSITORY ${arg_URL}
        GIT_TAG ${arg_TAG}
    )
    FetchContent_MakeAvailable  (${targetName})
    set_target_properties       (${targetName} PROPERTIES FOLDER "libs-external")
endfunction()



# Adds a static library target.
function(addLibrary targetName)
    set(options)
    set(oneArgs)
    set(multiArgs LIBS)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    if (CMAKE_CONFIGURATION_TYPES)
        set(outputDir ${CMAKE_BINARY_DIR}/$<CONFIG>/${targetName})
    else()
        set(outputDir   ${CMAKE_BINARY_DIR}/${targetName})
    endif()
    set(dir ${CMAKE_SOURCE_DIR}/src/libs/${targetName})
    file(GLOB HEADERS CONFIGURE_DEPENDS ${dir}/*.h)
    file(GLOB SOURCES CONFIGURE_DEPENDS ${dir}/*.cpp)
    set(LIBS ${arg_LIBS})

    message(STATUS "----------------------------------------")
    message(STATUS "Library (Static): ${targetName}")
    printList(HEADERS SOURCES LIBS)
    add_library                 (${targetName} STATIC)
    target_include_directories  (${targetName} PUBLIC  ${dir} ${dir}/..)
    target_sources              (${targetName} PRIVATE  ${HEADERS})
    target_sources              (${targetName} PRIVATE ${SOURCES})
    target_link_libraries       (${targetName} PRIVATE ${LIBS})
    target_compile_features     (${targetName} PRIVATE cxx_std_20)
    target_compile_definitions  (${targetName} PRIVATE _CRT_SECURE_NO_WARNINGS)
    set_target_properties       (${targetName} PROPERTIES FOLDER "libs")
    set_target_properties       (${targetName} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${outputDir})
    source_group                ("" FILES ${HEADERS} ${SOURCES})
endfunction()



# Adds a sample target (D3D12/Vulkan)
function(addSample targetName)
    set(options)
    set(oneArgs)
    set(multiArgs LIBS)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})
    
    if (CMAKE_CONFIGURATION_TYPES)
        set(outputDir   ${CMAKE_BINARY_DIR}/$<CONFIG>/${targetName})
    else()
        set(outputDir   ${CMAKE_BINARY_DIR}/${targetName})
    endif()
    set(targetNameVk    ${targetName}-vk)
    set(outputDirVk     ${outputDir}-vk)
    set(dir             ${CMAKE_SOURCE_DIR}/src/samples/${targetName})
    set(LIBS            ${arg_LIBS} utilsD3D d3d12 d3dx12 dxgi d3dcompiler)
    set(LIBS_VK         ${arg_LIBS} utilsVk vulkan)
    file(GLOB HEADERS    CONFIGURE_DEPENDS ${dir}/*.h)
    file(GLOB SOURCES    CONFIGURE_DEPENDS ${dir}/*.cpp ${dir}/d3d/*.cpp)
    file(GLOB SOURCES_VK CONFIGURE_DEPENDS ${dir}/*.cpp ${dir}/vk/*.cpp)
    file(GLOB SHADERS    CONFIGURE_DEPENDS ${dir}/d3d/*.hlsl)
    file(GLOB SHADERS_VK CONFIGURE_DEPENDS ${dir}/vk/*.vert ${dir}/vk/*.frag ${dir}/vk/*.comp)

    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "Sample: ${targetName}")
    message(STATUS "D3D12")
    printList(HEADERS SOURCES SHADERS LIBS)
    message(STATUS "VULKAN")
    printList(HEADERS SOURCES_VK SHADERS_VK LIBS_VK)

    add_executable              (${targetName})
    target_include_directories  (${targetName} PRIVATE ${CMAKE_SOURCE_DIR}/src/libs ${dir})
    target_sources              (${targetName} PRIVATE ${HEADERS})
    target_sources              (${targetName} PRIVATE ${SOURCES})
    target_shaders_d3d          (${targetName} ${SHADERS})
    target_assets               (${targetName})
    target_link_libraries       (${targetName} PRIVATE ${LIBS})
    target_compile_features     (${targetName} PRIVATE cxx_std_20)
    target_compile_definitions  (${targetName} PRIVATE _CRT_SECURE_NO_WARNINGS)
    set_target_properties       (${targetName} PROPERTIES FOLDER "samples")
    set_target_properties       (${targetName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${outputDir})
    source_group                ("" FILES ${HEADERS} ${SOURCES})
    
    add_executable              (${targetNameVk})
    target_include_directories  (${targetNameVk} PRIVATE ${CMAKE_SOURCE_DIR}/src/libs)
    target_sources              (${targetNameVk} PRIVATE ${HEADERS_VK})
    target_sources              (${targetNameVk} PRIVATE ${SOURCES_VK})
    target_shaders_vk           (${targetNameVk} ${SHADERS_VK})
    target_assets               (${targetName})
    target_link_libraries       (${targetNameVk} PRIVATE ${LIBS_VK})
    target_compile_features     (${targetNameVk} PRIVATE cxx_std_20)
    target_compile_definitions  (${targetNameVk} PRIVATE _CRT_SECURE_NO_WARNINGS)
    set_target_properties       (${targetNameVk} PROPERTIES FOLDER "samples")
    set_target_properties       (${targetNameVk} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${outputDirVk})
    source_group                ("" FILES ${HEADERS_VK} ${SOURCES_VK})
endfunction()



# Specify the D3D shaders of the target.
# - Adds input files (.hlsl) and generated .dxil files as sources.
# - Place .hlsl files under no group, and .dxil files under "Generated" group.
# - Compiles debug and release .dxil variants.
# - Works with single and multi config.
function(target_shaders_d3d targetName)
    set(inShaders ${ARGN})
    set(compiler  $ENV{WINDOWS_SDK}/bin/$ENV{WINDOWS_SDK_VERSION}/x64/dxc.exe)

    # Don't use GENEXP so source_group can work.
    if (CMAKE_CONFIGURATION_TYPES)
        set(dirDebug   ${CMAKE_BINARY_DIR}/Debug/${targetName})
        set(dirRelease ${CMAKE_BINARY_DIR}/Release/${targetName})
    else()
        set(dirDebug   ${CMAKE_BINARY_DIR}/${targetName})
        set(dirRelease ${CMAKE_BINARY_DIR}/${targetName})
    endif()

    foreach(inShader IN LISTS inShaders)
        cmake_path(GET inShader STEM shaderName)
        set(vertexDebug   ${dirDebug}/${shaderName}_v_debug.dxil)
        set(pixelDebug    ${dirDebug}/${shaderName}_p_debug.dxil)
        set(vertexRelease ${dirRelease}/${shaderName}_v.dxil)
        set(pixelRelease  ${dirRelease}/${shaderName}_p.dxil)
        list(APPEND outShadersDebug   ${vertexDebug}   ${pixelDebug})
        list(APPEND outShadersRelease ${vertexRelease} ${pixelRelease})

        # Add commands for both configurations here, but later select correct files per config.
        add_custom_command(
            OUTPUT ${vertexDebug} ${pixelDebug}
            COMMAND ${compiler} -T vs_6_0 -E VSMain -Fo ${vertexDebug} ${inShader}
            COMMAND ${compiler} -T ps_6_0 -E PSMain -Fo ${pixelDebug}  ${inShader}
            DEPENDS ${inShader}
            VERBATIM
        )
        add_custom_command(
            OUTPUT ${vertexRelease} ${pixelRelease}
            COMMAND ${compiler} -T vs_6_0 -E VSMain -Fo ${vertexRelease} ${inShader}
            COMMAND ${compiler} -T ps_6_0 -E PSMain -Fo ${pixelRelease}  ${inShader}
            DEPENDS ${inShader}
            VERBATIM
        )
    endforeach()

    # Single config selects files correctly by config, multi-config does not...
    # i.e., when building debug, it will compile both debug and release shaders.
    # But both only rebuild upon changes correctly.
    target_sources(${targetName} PRIVATE
        ${inShaders}
        $<$<CONFIG:Debug>:${outShadersDebug}>
        $<$<CONFIG:Release>:${outShadersRelease}>
    )
    set_source_files_properties(${inShaders} PROPERTIES HEADER_FILE_ONLY ON) # avoids VS compilation of hlsl
    source_group("" FILES ${inShaders})
    source_group("Generated" FILES ${outShadersDebug} ${outShadersRelease})
endfunction()



# Specify the vulkan shaders of the target.
# - Adds input files (.vert/.frag/.comp) and generated .spv files as sources.
# - Place source files under no group, and .spv files under "Generated" group.
# - Compiles debug and release .spv variants.
# - Works with single and multi config.
function(target_shaders_vk targetName)
    set(inShaders ${ARGN})
    set(compiler  $ENV{VULKAN_SDK}/Bin/glslc.exe)

    # Don't use GENEXP so source_group can work.
    if (CMAKE_CONFIGURATION_TYPES)
        set(dirDebug   ${CMAKE_BINARY_DIR}/Debug/${targetName})
        set(dirRelease ${CMAKE_BINARY_DIR}/Release/${targetName})
    else()
        set(dirDebug   ${CMAKE_BINARY_DIR}/${targetName})
        set(dirRelease ${CMAKE_BINARY_DIR}/${targetName})
    endif()

    foreach(inShader IN LISTS inShaders)
        cmake_path(GET inShader FILENAME shaderName)
        set(spvDebug   ${dirDebug}/${shaderName}_debug.spv)
        set(spvRelease ${dirRelease}/${shaderName}.spv)
        list(APPEND outShadersDebug   ${spvDebug})
        list(APPEND outShadersRelease ${spvRelease})

        # Add commands for both configurations here, but later select correct files per config.
        add_custom_command(
            OUTPUT ${spvDebug}
            COMMAND ${compiler} ${inShader} -o ${spvDebug}
            DEPENDS ${inShader}
            VERBATIM
        )
        add_custom_command(
            OUTPUT ${spvRelease}
            COMMAND ${compiler} ${inShader} -o ${spvRelease}
            DEPENDS ${inShader}
            VERBATIM
        )
    endforeach()

    # Single config selects files correctly by config, multi-config does not...
    # i.e., when building debug, it will compile both debug and release shaders.
    # But both only rebuild upon changes correctly.
    target_sources(${targetName} PRIVATE
        ${inShaders}
        $<$<CONFIG:Debug>:${outShadersDebug}>
        $<$<CONFIG:Release>:${outShadersRelease}>
    )
    set_source_files_properties(${inShaders} PROPERTIES HEADER_FILE_ONLY ON) # just to mirror d3d variant
    source_group("" FILES ${inShaders})
    source_group("Generated" FILES ${outShadersDebug} ${outShadersRelease})
endfunction()


function(target_assets targetName)
    if (CMAKE_CONFIGURATION_TYPES)
        set(outputDir ${CMAKE_BINARY_DIR}/$<CONFIG>/${targetName})
    else()
        set(outputDir ${CMAKE_BINARY_DIR}/${targetName})
    endif()
    set(assetsDir     ${CMAKE_SOURCE_DIR}/src/samples/${targetName}/assets)
    file(GLOB inAssets CONFIGURE_DEPENDS
        ${assetsDir}/*.gltf
        ${assetsDir}/*.bin
        ${assetsDir}/*.png
    )
    target_sources(${targetName} PRIVATE ${inAssets})
    source_group("Assets" FILES ${inAssets})
    add_custom_command(
        TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${assetsDir} ${outputDir}
    )
endfunction()