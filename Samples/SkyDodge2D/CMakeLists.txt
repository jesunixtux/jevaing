cmake_minimum_required(VERSION 3.20)
project(SkyDodge2D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(JEVAING_ENGINE_ROOT "" CACHE PATH "Path to Jevaing source")
if(NOT JEVAING_ENGINE_ROOT)
    get_filename_component(JEVAING_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()
if(NOT EXISTS "${JEVAING_ENGINE_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "Set JEVAING_ENGINE_ROOT to the Jevaing source directory")
endif()

set(JEVAING_BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
set(JEVAING_BUILD_EDITOR OFF CACHE BOOL "" FORCE)
set(JEVAING_BUILD_XBOX_SMOKE_TEST OFF CACHE BOOL "" FORCE)
set(JEVAING_ENABLE_PHYSICS_3D OFF CACHE BOOL "" FORCE)
add_subdirectory("${JEVAING_ENGINE_ROOT}" JevaingBuild)

add_executable(SkyDodge2D Source/Game.cpp)
target_link_libraries(SkyDodge2D PRIVATE JevaingEngine)
set_target_properties(SkyDodge2D PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/bin/Release")
add_custom_command(TARGET SkyDodge2D POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets" "$<TARGET_FILE_DIR:SkyDodge2D>/Assets"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Scenes" "$<TARGET_FILE_DIR:SkyDodge2D>/Scenes"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/jevaing.project"
        "$<TARGET_FILE_DIR:SkyDodge2D>/jevaing.project"
    VERBATIM)
