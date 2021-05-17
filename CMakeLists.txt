cmake_minimum_required(VERSION 3.20)
project(diorama)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/SDL2/lib/x64
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/sketchup/binaries/sketchup/x64)

add_executable(diorama
    mathutils.cpp
    material.cpp
    component.cpp
    world.cpp
    game.cpp
    load_skp.cpp
    main.cpp
    libraries/gl3w/src/gl3w.c)

target_include_directories(diorama PRIVATE
    libraries/SDL2/include
    libraries/gl3w/include
    libraries/sketchup/headers
    libraries/glm)

# TODO static vs shared?
target_link_libraries(diorama SDL2 SDL2main SketchUpAPI)

add_custom_command(TARGET diorama POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/libraries/SDL2/lib/x64/SDL2.dll"
        $<TARGET_FILE_DIR:diorama>)
add_custom_command(TARGET diorama POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/libraries/sketchup/binaries/sketchup/x64/SketchUpAPI.dll"
        $<TARGET_FILE_DIR:diorama>)
add_custom_command(TARGET diorama POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/libraries/sketchup/binaries/sketchup/x64/SketchUpCommonPreferences.dll"
        $<TARGET_FILE_DIR:diorama>)
