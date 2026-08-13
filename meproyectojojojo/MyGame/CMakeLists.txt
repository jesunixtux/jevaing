cmake_minimum_required(VERSION 3.20)

project(MyGame LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(JEVAING_ENGINE_ROOT "" CACHE PATH "Path to Jevaing engine source")
if(NOT JEVAING_ENGINE_ROOT)
    message(FATAL_ERROR "Set JEVAING_ENGINE_ROOT to the Jevaing engine source directory.")
endif()

set(JEVAING_BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
set(JEVAING_BUILD_EDITOR OFF CACHE BOOL "" FORCE)
set(JEVAING_BUILD_XBOX_SMOKE_TEST OFF CACHE BOOL "" FORCE)

add_subdirectory("${JEVAING_ENGINE_ROOT}" JevaingBuild)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    Source/*.c
    Source/*.cpp
)

add_executable(MyGame ${GAME_SOURCES})

target_include_directories(MyGame PRIVATE Source)
target_link_libraries(MyGame PRIVATE JevaingEngine)

set_target_properties(MyGame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}/bin/Release"
)
