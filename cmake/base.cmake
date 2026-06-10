


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


# Adds an imported static library target.
function(addImported targetName)
    set(options)
    set(oneArgs INC_DIR LIB)
    set(multiArgs)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    message(STATUS "----------------------------------------")
    message(STATUS "Library (Imported Static): ${targetName}")
    message(STATUS "  Include Dir: ${arg_INC_DIR}")
    message(STATUS "  Lib:         ${arg_LIB}")

    add_library          (${targetName} STATIC IMPORTED)
    set_target_properties(${targetName} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${arg_INC_DIR}
        IMPORTED_LOCATION             ${arg_LIB}
    )
endfunction()



# Adds an imported dynamic library target.
function(addImportedDynamic targetName)
    set(options)
    set(oneArgs INC_DIR LIB IMPLIB)
    set(multiArgs)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    message(STATUS "----------------------------------------")
    message(STATUS "Library (Imported Dynamic): ${targetName}")
    message(STATUS "  Include Dir: ${arg_INC_DIR}")
    message(STATUS "  Lib:         ${arg_LIB}")
    message(STATUS "  Imp Lib:     ${arg_IMPLIB}")

    add_library          (${targetName} SHARED IMPORTED)
    set_target_properties(${targetName} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${arg_INC_DIR}
        IMPORTED_LOCATION             ${arg_LIB}
        IMPORTED_IMPLIB               ${arg_IMPLIB}
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
    target_sources              (${targetName} PRIVATE ${HEADERS} ${SOURCES})
    target_link_libraries       (${targetName} PRIVATE ${LIBS})
    target_compile_features     (${targetName} PRIVATE cxx_std_20)
    target_compile_definitions  (${targetName} PRIVATE _CRT_SECURE_NO_WARNINGS)
    set_target_properties       (${targetName} PROPERTIES FOLDER "libs")
    set_target_properties       (${targetName} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${outputDir})
    source_group                ("" FILES ${HEADERS} ${SOURCES})
endfunction()



# Adds a sample target
function(addSample targetName)
    set(options)
    set(oneArgs)
    set(multiArgs LIBS)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})
    
    get_target_output_dir(${targetName} outputDir)
    set(dir             ${CMAKE_SOURCE_DIR}/src/samples/${targetName})
    set(assetsDir       ${CMAKE_SOURCE_DIR}/src/assets)
    set(LIBS            ${arg_LIBS})
    file(GLOB HEADERS    CONFIGURE_DEPENDS ${dir}/*.h)
    file(GLOB SOURCES    CONFIGURE_DEPENDS ${dir}/*.cpp)
    file(GLOB SHADERS    CONFIGURE_DEPENDS ${dir}/*.hlsl)
    file(GLOB CSHADERS   CONFIGURE_DEPENDS ${dir}/*.hlslc)
    file(GLOB SHADERS_VK CONFIGURE_DEPENDS ${dir}/*.vert ${dir}/*.frag ${dir}/*.comp)
    file(GLOB_RECURSE TEXTURES CONFIGURE_DEPENDS ${assetsDir}/*.png ${assetsDir}/*.jpeg ${assetsDir}/*.jpg)
    file(GLOB_RECURSE ASSETS   CONFIGURE_DEPENDS ${assetsDir}/*.gltf ${assetsDir}/*.bin ${assetsDir}/*.glb)

    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "Sample: ${targetName}")
    printList(HEADERS SOURCES SHADERS SHADERS_VK LIBS TEXTURES ASSETS)

    add_executable              (${targetName})
    target_include_directories  (${targetName} PRIVATE ${CMAKE_SOURCE_DIR}/src/libs ${dir})
    target_sources              (${targetName} PRIVATE ${HEADERS} ${SOURCES})
    target_shaders_d3d          (${targetName} ${SHADERS})
    target_shaders_compute_d3d  (${targetName} ${CSHADERS})
    target_shaders_vk           (${targetName} ${SHADERS_VK})
    target_assets               (${targetName} ${ASSETS})
    target_textures             (${targetName} ${TEXTURES})
    target_link_libraries       (${targetName} PRIVATE ${LIBS})
    target_compile_features     (${targetName} PRIVATE cxx_std_20)
    target_compile_definitions  (${targetName} PRIVATE _CRT_SECURE_NO_WARNINGS)
    set_target_properties       (${targetName} PROPERTIES FOLDER "samples")
    set_target_properties       (${targetName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${outputDir})

    source_group(""          FILES ${HEADERS} ${SOURCES} ${SHADERS} ${SHADERS_VK})
    source_group("Assets"    FILES ${TEXTURES} ${ASSETS})
    source_group("Generated" REGULAR_EXPRESSION ".*\\.(dxil|spv|dds|gltf|bin)")

    add_custom_command(
        TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${targetName}> $<TARGET_RUNTIME_DLLS:${targetName}>
        COMMAND_EXPAND_LISTS
    )
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
            COMMAND ${compiler} -T vs_6_0 -E VSMain -Zi -Qembed_debug -Od -Fo ${vShader} ${inShader}
            COMMAND ${compiler} -T ps_6_0 -E PSMain -Zi -Qembed_debug -Od -Fo ${pShader} ${inShader}
            DEPENDS ${inShader}
            MAIN_DEPENDENCY ${inShader}
            VERBATIM
        )
    endforeach()
    target_sources(${targetName} PRIVATE ${inShaders} ${outShaders})
    set_source_files_properties(${inShaders} PROPERTIES HEADER_FILE_ONLY ON) # avoids VS compilation of hlsl
endfunction()

function(target_shaders_compute_d3d targetName)
    set(inShaders ${ARGN})
    set(compiler  $ENV{WINDOWS_SDK}/bin/$ENV{WINDOWS_SDK_VERSION}/x64/dxc.exe)
    get_target_output_dir(${targetName} outputDir)
    foreach(inShader IN LISTS inShaders)
        cmake_path(GET inShader STEM shaderStem)
        set(shader ${outputDir}/${shaderStem}.dxil)
        list(APPEND outShaders ${shader})
        add_custom_command(
            OUTPUT ${shader}
            COMMAND ${compiler} -T cs_6_0 -E CSMain -Zi -Qembed_debug -Od -Fo ${shader} ${inShader}
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
        cmake_path(GET asset    PARENT_PATH assetDir)
        cmake_path(GET asset    FILENAME    assetFilename)
        cmake_path(GET asset    EXTENSION   assetExtension)
        cmake_path(GET assetDir FILENAME    assetName)

        set(outAsset ${outputDir}/${assetName}/${assetFilename})
        list(APPEND outAssets ${outAsset})

        if(assetExtension STREQUAL ".gltf")
            file(READ "${asset}" CONTENT)
            string(REGEX REPLACE "\\.(png|jpeg|jpg)" ".dds" CONTENT "${CONTENT}")
            file(WRITE "${outAsset}" "${CONTENT}")
        else()
            add_custom_command(
                OUTPUT ${outAsset}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${asset} ${outAsset}
                DEPENDS ${asset}
                VERBATIM
            )
        endif()
    endforeach()
    target_sources(${targetName} PRIVATE ${inAssets} ${outAssets})
endfunction()


# Specify the textures of the target.
# - Adds input files (.png) and converted .dds files as sources.
function(target_textures targetName)
    set(inFiles ${ARGN})
    get_target_output_dir(${targetName} outputDir)
    foreach(file IN LISTS inFiles)
        cmake_path(GET file     PARENT_PATH fileDir)
        cmake_path(GET fileDir  FILENAME    assetName)
        cmake_path(GET file     STEM        stem)
        set(outFile ${outputDir}/${assetName}/${stem}.dds)
        list(APPEND outFiles ${outFile})
        # Hack to convert sRGB appropriately.
        # Although currently the sRGB format seems to have no effect on sampling in shaders.
        if(file MATCHES "albedo|basecolor|baseColor|diffuse|emissive")
            set(format BC3_UNORM_SRGB)
        else()
            set(format BC3_UNORM)
        endif()
        message(STATUS "Texture ${file} format: ${format}")
        add_custom_command(
            OUTPUT ${outFile}
            COMMAND texconv -f ${format} ${file} -o ${outputDir}/${assetName}
            DEPENDS ${file}
            VERBATIM
        )
    endforeach()
    target_sources(${targetName} PRIVATE ${inFiles} ${outFiles})
endfunction()


