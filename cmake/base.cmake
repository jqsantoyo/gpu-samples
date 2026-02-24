
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
    FetchContent_MakeAvailable(${targetName})
endfunction()

# Adds a static library target.
function(addLibrary targetName)
    set(options)
    set(oneArgs)
    set(multiArgs LIBS)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    set(dir ${CMAKE_SOURCE_DIR}/src/libs/${targetName})
    file(GLOB HEADERS CONFIGURE_DEPENDS ${dir}/*.h)
    file(GLOB SOURCES CONFIGURE_DEPENDS ${dir}/*.cpp)
    set(LIBS ${arg_LIBS})

    message(STATUS "----------------------------------------")
    message(STATUS "Library (Static): ${targetName}")
    printList(HEADERS SOURCES LIBS)

    add_library                 (${targetName} STATIC)
    target_include_directories  (${targetName} PUBLIC  ${dir} ${dir}/..)
    target_sources              (${targetName} PUBLIC  ${HEADERS})
    target_sources              (${targetName} PRIVATE ${SOURCES})
    target_link_libraries       (${targetName} PRIVATE ${LIBS})
    target_compile_features     (${targetName} PRIVATE cxx_std_20)
    target_compile_definitions  (${targetName} PRIVATE _CRT_SECURE_NO_WARNINGS)
endfunction()


# Adds a sample target (D3D12/Vulkan)
function(addSample targetName)
    set(options)
    set(oneArgs)
    set(multiArgs LIBS)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    set(dir ${CMAKE_SOURCE_DIR}/src/samples/${targetName})
    file(GLOB HEADERS    CONFIGURE_DEPENDS ${dir}/*.h)
    file(GLOB SOURCES    CONFIGURE_DEPENDS ${dir}/*.cpp ${dir}/d3d/*.cpp)
    file(GLOB SOURCES_VK CONFIGURE_DEPENDS ${dir}/*.cpp ${dir}/vk/*.cpp)
    file(GLOB SHADERS    CONFIGURE_DEPENDS ${dir}/d3d/*.hlsl)
    file(GLOB SHADERS_VK CONFIGURE_DEPENDS ${dir}/vk/*.vert ${dir}/vk/*.frag ${dir}/vk/*.comp)

    set(LIBS ${arg_LIBS} d3d12 d3dx12 dxgi d3dcompiler)
    set(LIBS_VK ${arg_LIBS} vulkan)

    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "Sample: ${targetName}")
    message(STATUS "D3D12")
    printList(HEADERS SOURCES SHADERS LIBS)
    message(STATUS "VULKAN")
    printList(HEADERS SOURCES_VK SHADERS_VK LIBS_VK)


    add_executable(${targetName})
    target_include_directories  (${targetName} PRIVATE ${CMAKE_SOURCE_DIR}/src/libs ${dir})
    target_sources              (${targetName} PRIVATE ${HEADERS})
    target_sources              (${targetName} PRIVATE ${SOURCES})
    target_link_libraries       (${targetName} PRIVATE ${LIBS})
    target_compile_features     (${targetName} PRIVATE cxx_std_20)
    target_compile_definitions  (${targetName} PRIVATE _CRT_SECURE_NO_WARNINGS)

    # set(shaderDir ${CMAKE_CURRENT_BINARY_DIR}/${targetName}-shaders)
    # set(outShaders)
    # foreach(inShader IN LISTS SHADERS)
    #     cmake_path(GET inShader FILENAME shaderName)
    #     set(outShader ${shaderDir}/${shaderName})
    #     # message(STATUS "${targetName}: copy ${inShader} -> ${outShader}")
    #     add_custom_command(
    #         OUTPUT ${outShader}
    #         COMMAND ${CMAKE_COMMAND} -E echo "${targetName}: copy: ${inShader} -> ${outShader}"
    #         COMMAND ${CMAKE_COMMAND} -E make_directory ${shaderDir}
    #         COMMAND ${CMAKE_COMMAND} -E copy ${inShader} ${outShader}
    #         DEPENDS ${inShader}
    #     )
    #     list(APPEND outShaders ${outShader})
    # endforeach()
    # add_custom_target(${targetName}-assets DEPENDS ${outShaders})
    # add_dependencies(${targetName} ${targetName}-assets)


    set(shaderDir ${CMAKE_CURRENT_BINARY_DIR}/${targetName}-shaders)
    set(outShaders)
    foreach(inShader IN LISTS SHADERS)
        cmake_path(GET inShader STEM shaderName)
        set(outShaderV ${shaderDir}/${shaderName}_v.dxil)
        set(outShaderP ${shaderDir}/${shaderName}_p.dxil)
        message(STATUS "${targetName}: dxc: ${inShader} -> ${outShaderV}/${outShaderP}")
        add_custom_command(
            OUTPUT ${outShaderV} ${outShaderP}
            COMMAND ${CMAKE_COMMAND} -E echo "${targetName}: dxc: ${inShader} -> ${outShaderV}/${outShaderP}"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${shaderDir}
            COMMAND $ENV{WINDOWS_SDK}/bin/$ENV{WINDOWS_SDK_VERSION}/x64/dxc.exe  -T vs_5_0 -E VSMain -Fo ${outShaderV} ${inShader}
            COMMAND $ENV{WINDOWS_SDK}/bin/$ENV{WINDOWS_SDK_VERSION}/x64/dxc.exe  -T ps_5_0 -E PSMain -Fo ${outShaderP} ${inShader}
            DEPENDS ${inShader}
            VERBATIM
        )
        list(APPEND outShaders ${outShaderV} ${outShaderP})
    endforeach()
    add_custom_target(${targetName}-shaders DEPENDS ${outShaders})
    add_dependencies(${targetName} ${targetName}-shaders)




    # install(FILES ${shadersSpv}      DESTINATION bin/${targetName})
    # install(FILES ${arg_SHADERS_D3D} DESTINATION bin/${targetName})
    # install(
    #     TARGETS ${targetName}
    #     EXPORT ${targetName}Targets
    #     PUBLIC_HEADER DESTINATION include
    #     LIBRARY DESTINATION lib
    # )

    set(targetNameVk ${targetName}-vk)
    add_executable(${targetNameVk})
    target_include_directories  (${targetName} PRIVATE ${CMAKE_SOURCE_DIR}/src/libs)
    target_sources              (${targetNameVk} PRIVATE ${HEADERS_VK})
    target_sources              (${targetNameVk} PRIVATE ${SOURCES_VK})
    target_link_libraries       (${targetNameVk} PRIVATE ${LIBS_VK})
    target_compile_features     (${targetNameVk} PRIVATE cxx_std_20)
    target_compile_definitions  (${targetName} PRIVATE _CRT_SECURE_NO_WARNINGS)
    set(shaderDirVk ${CMAKE_CURRENT_BINARY_DIR}/${targetName}-shaders-vk)
    set(outShadersVk)
    foreach(inShader IN LISTS SHADERS_VK)
        cmake_path(GET inShader FILENAME shaderName)
        set(outShaderVk ${shaderDirVk}/${shaderName}.spv)
        message(STATUS "${targetName}-vk: glslc: ${inShader} -> ${outShaderVk}")
        add_custom_command(
            OUTPUT ${outShaderVk}
            COMMAND ${CMAKE_COMMAND} -E echo "${targetName}-vk: glslc: ${inShader} -> ${outShaderVk}"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${shaderDirVk}
            COMMAND $ENV{VULKAN_SDK}/Bin/glslc.exe ${inShader} -o ${outShaderVk}
            DEPENDS ${inShader}
            VERBATIM
        )
        list(APPEND outShadersVk ${outShaderVk})
    endforeach()
    add_custom_target(${targetName}-shaders-vk DEPENDS ${outShadersVk})
    add_dependencies(${targetName}-vk ${targetName}-shaders-vk)

    # install(FILES ${shadersSpv}      DESTINATION bin/${targetName})
    # install(FILES ${arg_SHADERS_D3D} DESTINATION bin/${targetName})
    # install(
    #     TARGETS ${targetName}
    #     EXPORT ${targetName}Targets
    #     PUBLIC_HEADER DESTINATION include
    #     LIBRARY DESTINATION lib
    # )
endfunction()