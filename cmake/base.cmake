


if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo CACHE STRING "" FORCE)
endif()


function(get_target_output_dir targetName outputDir)
    if (CMAKE_CONFIGURATION_TYPES)
        set(outputDir ${CMAKE_BINARY_DIR}/$<CONFIG>/${targetName} PARENT_SCOPE)
    else()
        set(outputDir ${CMAKE_BINARY_DIR}/${targetName} PARENT_SCOPE)
    endif()
endfunction()

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
    set(assetsDir       ${CMAKE_SOURCE_DIR}/src/samples/${targetName}/assets)
    set(LIBS            ${arg_LIBS} utilsD3D d3d12 d3dx12 dxgi d3dcompiler)
    set(LIBS_VK         ${arg_LIBS} utilsVk vulkan)
    file(GLOB HEADERS    CONFIGURE_DEPENDS ${dir}/*.h)
    file(GLOB SOURCES    CONFIGURE_DEPENDS ${dir}/*.cpp ${dir}/d3d/*.cpp)
    file(GLOB SOURCES_VK CONFIGURE_DEPENDS ${dir}/*.cpp ${dir}/vk/*.cpp)
    file(GLOB SHADERS    CONFIGURE_DEPENDS ${dir}/d3d/*.hlsl)
    file(GLOB SHADERS_VK CONFIGURE_DEPENDS ${dir}/vk/*.vert ${dir}/vk/*.frag ${dir}/vk/*.comp)
    file(GLOB ASSETS     CONFIGURE_DEPENDS ${assetsDir}/*.gltf ${assetsDir}/*.bin ${assetsDir}/*.png)

    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "Sample: ${targetName}")
    message(STATUS "D3D12")
    printList(HEADERS SOURCES SHADERS LIBS)
    message(STATUS "VULKAN")
    printList(HEADERS SOURCES_VK SHADERS_VK LI  BS_VK)

    add_executable              (${targetName})
    target_include_directories  (${targetName} PRIVATE ${CMAKE_SOURCE_DIR}/src/libs ${dir})
    target_sources              (${targetName} PRIVATE ${HEADERS})
    target_sources              (${targetName} PRIVATE ${SOURCES})
    target_shaders_d3d          (${targetName} ${SHADERS})
    target_assets               (${targetName} ${ASSETS})
    target_link_libraries       (${targetName} PRIVATE ${LIBS})
    target_compile_features     (${targetName} PRIVATE cxx_std_20)
    target_compile_definitions  (${targetName} PRIVATE _CRT_SECURE_NO_WARNINGS)
    set_target_properties       (${targetName} PROPERTIES FOLDER "samples")
    set_target_properties       (${targetName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${outputDir})
    
    add_executable              (${targetNameVk})
    target_include_directories  (${targetNameVk} PRIVATE ${CMAKE_SOURCE_DIR}/src/libs)
    target_sources              (${targetNameVk} PRIVATE ${HEADERS_VK})
    target_sources              (${targetNameVk} PRIVATE ${SOURCES_VK})
    target_shaders_vk           (${targetNameVk} ${SHADERS_VK})
    target_assets               (${targetNameVk} ${ASSETS})
    target_link_libraries       (${targetNameVk} PRIVATE ${LIBS_VK})
    target_compile_features     (${targetNameVk} PRIVATE cxx_std_20)
    target_compile_definitions  (${targetNameVk} PRIVATE _CRT_SECURE_NO_WARNINGS)
    set_target_properties       (${targetNameVk} PROPERTIES FOLDER "samples")
    set_target_properties       (${targetNameVk} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${outputDirVk})

    source_group("Source"    FILES ${HEADERS} ${SOURCES} ${HEADERS_VK} ${SOURCES_VK} ${SHADERS} ${SHADERS_VK})
    source_group("Assets"    FILES ${ASSETS})
    source_group("Generated" REGULAR_EXPRESSION ".*\\.(dxil|spv|png|gltf|bin)")
endfunction()




# Specify the D3D shaders of the target.
# - Adds input files (.hlsl) and compiled .dxil files as sources.
function(target_shaders_d3d targetName)
    set(inShaders ${ARGN})
    set(compiler  $ENV{WINDOWS_SDK}/bin/$ENV{WINDOWS_SDK_VERSION}/x64/dxc.exe)
    get_target_output_dir(${targetName} outputDir)
    foreach(inShader IN LISTS inShaders)
        cmake_path(GET inShader STEM shaderStem)
        set(vShader ${outputDir}/${shaderStem}_v.dxil)
        set(pShader ${outputDir}/${shaderStem}_p.dxil)
        list(APPEND outShaders ${vShader} ${pShader})
        add_custom_command(
            OUTPUT ${vShader} ${pShader}
            COMMAND ${compiler} -T vs_6_0 -E VSMain -Fo ${vShader} ${inShader}
            COMMAND ${compiler} -T ps_6_0 -E PSMain -Fo ${pShader} ${inShader}
            DEPENDS ${inShader}
            MAIN_DEPENDENCY ${inShader}
            VERBATIM
        )
    endforeach()
    target_sources(${targetName} PRIVATE ${inShaders} ${outShaders})
    set_source_files_properties(${inShaders} PROPERTIES HEADER_FILE_ONLY ON) # avoids VS compilation of hlsl
endfunction()


# Specify the Vulkan shaders of the target.
# - Adds input files (.vert/.frag/.comp) and compiled .spv files as sources.
function(target_shaders_vk targetName)
    set(inShaders ${ARGN})
    set(compiler  $ENV{VULKAN_SDK}/Bin/glslc.exe)
    get_target_output_dir(${targetName} outputDir)
    foreach(inShader IN LISTS inShaders)
        cmake_path(GET inShader FILENAME shaderFilename)
        set(outShader ${outputDir}/${shaderFilename}.spv)
        list(APPEND outShaders ${outShader})
        add_custom_command(
            OUTPUT ${outShader}
            COMMAND ${compiler} ${inShader} -o ${outShader}
            DEPENDS ${inShader}
            MAIN_DEPENDENCY ${inShader}
            VERBATIM
        )
    endforeach()
    target_sources(${targetName} PRIVATE ${inShaders} ${outShaders})
endfunction()

# Specify the assets of the target.
# - Adds asset files as sources and copies them to the binary directory.
function(target_assets targetName)
    set(inAssets ${ARGN})
    get_target_output_dir(${targetName} outputDir)
    foreach(asset IN LISTS inAssets)
        cmake_path(GET asset FILENAME assetFilename)
        set(outAsset ${outputDir}/${assetFilename})
        list(APPEND outAssets ${outAsset})
        add_custom_command(
            OUTPUT ${outAsset}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${asset} ${outAsset}
            DEPENDS ${asset}
            MAIN_DEPENDENCY ${asset}
            VERBATIM
        )
    endforeach()
    target_sources(${targetName} PRIVATE ${inAssets} ${outAssets})
endfunction()


