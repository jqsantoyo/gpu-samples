
# Prints a list.
function(printList)
    foreach(item IN LISTS ${ARGN})
        message(STATUS "  ${item}")
    endforeach()
endfunction()



# Adds an interface library target.
# Early exits when platform options (WIN/MAC) are provided and do not match the platform.
function(addImported targetName)
    set(options WIN MAC)
    set(oneArgs INC_DIR LIB)
    set(multiArgs)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    if((arg_WIN OR arg_MAC) AND
       (CMAKE_SYSTEM_NAME STREQUAL Windows AND NOT arg_WIN) OR
       (CMAKE_SYSTEM_NAME STREQUAL Darwin AND NOT arg_MAC))
       return()
    endif()

    message(STATUS "----------------------------------------")
    message(STATUS "Library (Imported): ${targetName}")
    message(STATUS "  Include Dir: ${arg_INC_DIR}")
    message(STATUS "  Lib:         ${arg_LIB}")
    add_library             (${targetName} STATIC IMPORTED)
    set_target_properties   (${targetName} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${arg_INC_DIR}
        IMPORTED_LOCATION             ${arg_LIB}
    )
endfunction()

# Adds an imported library target.
# Early exits when platform options (WIN/MAC) are provided and do not match the platform.
function(addInterface targetName)
    set(options WIN MAC)
    set(oneArgs)
    set(multiArgs)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})
    
    if((arg_WIN OR arg_MAC) AND
       (CMAKE_SYSTEM_NAME STREQUAL Windows AND NOT arg_WIN) OR
       (CMAKE_SYSTEM_NAME STREQUAL Darwin AND NOT arg_MAC))
       return()
    endif()

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
# No platform switch provided.
function(addLibrary targetName)
    set(options)
    set(oneArgs)
    set(multiArgs LIBS LIBS_WIN LIBS_MAC FRAMES)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    set(dir ${CMAKE_SOURCE_DIR}/src/libs/${targetName})
    file(GLOB HEADERS CONFIGURE_DEPENDS ${dir}/*.h)
    file(GLOB SOURCES CONFIGURE_DEPENDS ${dir}/*.cpp)
    set(LIBS ${arg_LIBS})
    if(CMAKE_SYSTEM_NAME STREQUAL Windows)
        list(APPEND LIBS ${arg_LIBS_WIN})
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        list(TRANSFORM arg_FRAMES  PREPEND "-framework ")
        list(APPEND LIBS ${arg_LIBS_MAC} ${arg_FRAMES})
    endif()

    message(STATUS "----------------------------------------")
    message(STATUS "Library (Static): ${targetName}")
    printList(HEADERS SOURCES LIBS)

    add_library                 (${targetName} STATIC)
    target_include_directories  (${targetName} PUBLIC  ${dir}/..)
    target_sources              (${targetName} PUBLIC  ${HEADERS})
    target_sources              (${targetName} PRIVATE ${SOURCES})
    target_link_libraries       (${targetName} PRIVATE ${LIBS})
    target_compile_features     (${targetName} PRIVATE cxx_std_20)
    set_target_properties       (${targetName} PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endfunction()


# Adds a sample target (D3D/Vulkan or Metal/Vulkan)
# No platform switch provided.
function(addSample targetName)
    set(options)
    set(oneArgs)
    set(multiArgs LIBS LIBS_WIN LIBS_MAC FRAMES)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    set(dir ${CMAKE_SOURCE_DIR}/src/samples/${targetName})
    file(GLOB HEADERS CONFIGURE_DEPENDS ${dir}/*.h)
    file(GLOB SOURCES_VK CONFIGURE_DEPENDS ${dir}/*.cpp ${dir}/vk/*.cpp)
    file(GLOB SHADERS_VK CONFIGURE_DEPENDS ${dir}/vk/*.vert ${dir}/vk/*.frag)
    if(CMAKE_SYSTEM_NAME STREQUAL Windows)
        file(GLOB SOURCES CONFIGURE_DEPENDS ${dir}/*.cpp ${dir}/d3d/*.cpp)
        file(GLOB SHADERS CONFIGURE_DEPENDS ${dir}/d3d/*.hlsl)
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        file(GLOB SOURCES CONFIGURE_DEPENDS ${dir}/*.cpp ${dir}/mtl/*.cpp)
        file(GLOB SHADERS CONFIGURE_DEPENDS ${dir}/mtl/*.mtl)
    endif()
    set(LIBS ${arg_LIBS})
    set(LIBS_VK ${arg_LIBS})
    if(CMAKE_SYSTEM_NAME STREQUAL Windows)
        list(APPEND LIBS ${arg_LIBS_WIN} d3d12 d3dx12 dxgi d3dcompiler)
        list(APPEND LIBS_VK ${arg_LIBS_WIN} vulkan)
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        list(TRANSFORM arg_FRAMES  PREPEND "-framework ")
        list(APPEND LIBS ${arg_LIBS_MAC} ${arg_FRAMES})
        list(APPEND LIBS_VK ${arg_LIBS_MAC} ${arg_FRAMES} vulkan)
    endif()

    message(STATUS "--------------------------------------------------------------------------------")
    message(STATUS "Sample: ${targetName}")
    if(CMAKE_SYSTEM_NAME STREQUAL Windows)
        message(STATUS "DIRECT3D")
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        message(STATUS "METAL")
    endif()
    printList(HEADERS SOURCES SHADERS LIBS)
    message(STATUS "VULKAN")
    printList(HEADERS SOURCES_VK SHADERS_VK LIBS_VK)


    add_executable(${targetName})
    target_include_directories  (${targetName} PRIVATE ${CMAKE_SOURCE_DIR}/src/libs ${dir})
    target_sources              (${targetName} PRIVATE ${HEADERS})
    target_sources              (${targetName} PRIVATE ${SOURCES})
    target_link_libraries       (${targetName} PRIVATE ${LIBS})
    target_compile_features     (${targetName} PRIVATE cxx_std_20)

    set(shaderDir ${CMAKE_CURRENT_BINARY_DIR}/${targetName}-shaders)
    set(outShaders)
    foreach(inShader IN LISTS SHADERS)
        cmake_path(GET inShader FILENAME shaderName)
        set(outShader ${shaderDir}/${shaderName})
        # message(STATUS "${targetName}: copy ${inShader} -> ${outShader}")
        add_custom_command(
            OUTPUT ${outShader}
            COMMAND ${CMAKE_COMMAND} -E echo "${targetName}: copy: ${inShader} -> ${outShader}"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${shaderDir}
            COMMAND ${CMAKE_COMMAND} -E copy ${inShader} ${outShader}
            DEPENDS ${inShader}
        )
        list(APPEND outShaders ${outShader})
    endforeach()
    add_custom_target(${targetName}-assets DEPENDS ${outShaders})
    add_dependencies(${targetName} ${targetName}-assets)


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
    set(shadersSpv ${arg_SHADERS_VK})
    list(TRANSFORM shadersSpv PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)
    list(TRANSFORM shadersSpv APPEND .spv)
    foreach(shader shaderSpv IN ZIP_LISTS arg_SHADERS_VK shadersSpv)
        add_custom_command(
            TARGET ${targetNameVk} PRE_BUILD
            COMMAND $ENV{VK_SDK_PATH}/Bin/glslc.exe ${CMAKE_CURRENT_SOURCE_DIR}/${shader} -o ${shader}.spv
            # DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${shader}
            BYPRODUCTS ${shaderSpv}
            COMMENT "Compile ${shader}"
            VERBATIM
        )
    endforeach()
    # install(FILES ${shadersSpv}      DESTINATION bin/${targetName})
    # install(FILES ${arg_SHADERS_D3D} DESTINATION bin/${targetName})
    # install(
    #     TARGETS ${targetName}
    #     EXPORT ${targetName}Targets
    #     PUBLIC_HEADER DESTINATION include
    #     LIBRARY DESTINATION lib
    # )
endfunction()