
# Prints a list.
function(printList name list)
    message(STATUS "${name}:")
    foreach(item IN LISTS ${list})
        message(STATUS "  ${item}")
    endforeach()
endfunction()


# Adds a library target.
function(addLib targetName)
    set(options)
    set(oneArgs SHARED)
    set(multiArgs HEADERS SOURCES LIBS)
    list(APPEND multiArgs SOURCES_WIN SOURCES_MAC SOURCES_ANDROID SOURCES_IOS)
    list(APPEND multiArgs LIBS_WIN    LIBS_MAC    LIBS_ANDROID    LIBS_IOS)
    list(APPEND multiArgs FRAMEWORKS_MAC FRAMEWORKS_IOS)
    list(APPEND multiArgs SHADERS_VK SHADERS_D3D SHADERS_MTL)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})

    message(STATUS "----------------------------------------")
    message(STATUS "LIB: ${targetName} ${SHARED} processor:${CMAKE_SYSTEM_PROCESSOR} system:${CMAKE_SYSTEM_NAME}")
    printList("HEADERS"         arg_HEADERS)
    printList("SOURCES"         arg_SOURCES)
    printList("SOURCES WIN"     arg_SOURCES_WIN)
    printList("SOURCES MAC"     arg_SOURCES_MAC)
    printList("SOURCES ANDROID" arg_SOURCES_ANDROID)
    printList("SOURCES IOS"     arg_SOURCES_IOS)
    printList("LIBS"            arg_LIBS)
    printList("LIBS WIN"        arg_LIBS_WIN)
    printList("LIBS MAC"        arg_LIBS_MAC)
    printList("LIBS ANDROID"    arg_LIBS_ANDROID)
    printList("LIBS IOS"        arg_LIBS_IOS)
    printList("FRAMEWORKS MAC"  arg_FRAMEWORKS_MAC)
    printList("FRAMEWORKS IOS"  arg_FRAMEWORKS_IOS)
    list(TRANSFORM arg_FRAMEWORKS_MAC  PREPEND "-framework ")
    list(TRANSFORM arg_FRAMEWORKS_IOS  PREPEND "-framework ")

    message(STATUS "LIB STATIC")
    add_library                 (${targetName} STATIC)
    target_compile_features     (${targetName} PRIVATE cxx_std_14)
    target_include_directories  (${targetName} PUBLIC  ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries       (${targetName} PUBLIC  ${arg_LIBS})
    target_sources              (${targetName} PUBLIC  ${arg_HEADERS})
    target_sources              (${targetName} PRIVATE ${arg_SOURCES})
    if(CMAKE_SYSTEM_NAME STREQUAL Windows)
        target_sources          (${targetName} PRIVATE ${arg_SOURCES_WIN})
        target_link_libraries   (${targetName} PUBLIC  ${arg_LIBS_WIN})
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        target_sources          (${targetName} PRIVATE ${arg_SOURCES_MAC})
        target_link_libraries   (${targetName} PUBLIC  ${arg_LIBS_MAC})
        target_link_libraries   (${targetName} PUBLIC  ${arg_FRAMEWORKS_MAC})
    elseif(CMAKE_SYSTEM_NAME STREQUAL Android)
        target_sources          (${targetName} PRIVATE ${arg_SOURCES_ANDROID})
        target_link_libraries   (${targetName} PUBLIC  ${arg_LIBS_ANDROID})
    elseif(CMAKE_SYSTEM_NAME STREQUAL iOS)
        target_sources          (${targetName} PRIVATE ${arg_SOURCES_IOS})
        target_link_libraries   (${targetName} PUBLIC  ${arg_LIBS_IOS})
        target_link_libraries   (${targetName} PUBLIC  ${arg_FRAMEWORKS_IOS})
    endif()
    
    set(shadersSpv ${arg_SHADERS_VK})
    list(TRANSFORM shadersSpv PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)
    list(TRANSFORM shadersSpv APPEND .spv)
    foreach(shader shaderSpv IN ZIP_LISTS arg_SHADERS_VK shadersSpv)
        add_custom_command(
            TARGET ${targetName} PRE_BUILD
            COMMAND $ENV{VK_SDK_PATH}/Bin/glslc.exe ${CMAKE_CURRENT_SOURCE_DIR}/${shader} -o ${shader}.spv
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${shader}
            BYPRODUCTS ${shaderSpv}
            COMMENT "Compile ${shader}"
            VERBATIM
        )
    endforeach()
    install(FILES ${shadersSpv} DESTINATION bin/${targetName})

    
    set_target_properties(${targetName} PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )

    install(
        TARGETS ${targetName}
        EXPORT ${targetName}Targets
        PUBLIC_HEADER DESTINATION include
        LIBRARY DESTINATION lib
    )
endfunction()



# Adds an executable target (only for windows and macos platforms).
function(addExe targetName)
    set(options)
    set(oneArgs)
    set(multiArgs SOURCES LIBS SHADERS)
    list(APPEND multiArgs SHADERS_VK SHADERS_D3D SHADERS_MTL)
    cmake_parse_arguments(arg "${options}" "${oneArgs}" "${multiArgs}" ${ARGN})


    if(CMAKE_SYSTEM_NAME STREQUAL Android)
        return()
    elseif(CMAKE_SYSTEM_NAME STREQUAL iOS)
        return()
    endif()

    message(STATUS "----------------------------------------")
    message(STATUS "EXE: ${targetName}")
    printList("SOURCES"         arg_SOURCES)
    printList("LIBS"            arg_LIBS)
    printList("SHADERS_VK"      arg_SHADERS_VK)
    
    add_executable              (${targetName})
    target_compile_features     (${targetName} PRIVATE cxx_std_14)
    target_include_directories  (${targetName} PUBLIC  ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries       (${targetName} PRIVATE ${arg_LIBS})
    target_sources              (${targetName} PRIVATE ${arg_SOURCES})
    
    set(shadersSpv ${arg_SHADERS_VK})
    list(TRANSFORM shadersSpv PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)
    list(TRANSFORM shadersSpv APPEND .spv)
    foreach(shader shaderSpv IN ZIP_LISTS arg_SHADERS_VK shadersSpv)
        add_custom_command(
            TARGET ${targetName} PRE_BUILD
            COMMAND $ENV{VK_SDK_PATH}/Bin/glslc.exe ${CMAKE_CURRENT_SOURCE_DIR}/${shader} -o ${shader}.spv
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${shader}
            BYPRODUCTS ${shaderSpv}
            COMMENT "Compile ${shader}"
            VERBATIM
        )
    endforeach()
    install(FILES ${shadersSpv} DESTINATION bin/${targetName})

    install(
        TARGETS ${targetName}
        PUBLIC_HEADER DESTINATION include
        LIBRARY DESTINATION lib/${KRA_ARCH}
    )
endfunction()