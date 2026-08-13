cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(XboxSmokeTest LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set(JEVAING_ENGINE_ROOT "" CACHE PATH "Path to Jevaing engine source")
    if(NOT JEVAING_ENGINE_ROOT)
        message(FATAL_ERROR "Set JEVAING_ENGINE_ROOT to the Jevaing engine source directory.")
    endif()

    add_subdirectory("${JEVAING_ENGINE_ROOT}" JevaingBuild)
endif()

add_executable(XboxSmokeTest
    Source/Game.cpp
)

target_link_libraries(XboxSmokeTest PRIVATE JevaingEngine)

if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    target_sources(XboxSmokeTest PRIVATE Package.appxmanifest)
    set_target_properties(XboxSmokeTest PROPERTIES
        VS_WINRT_COMPONENT TRUE
        VS_GLOBAL_AppxBundle "Always"
        VS_GLOBAL_AppxBundlePlatforms "x64"
        VS_GLOBAL_AppxPackageSigningEnabled "false"
    )
    target_compile_definitions(XboxSmokeTest PRIVATE JEVAING_XBOX_DEV_MODE_UWP_SMOKE=1)
endif()

set_target_properties(XboxSmokeTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG
        "${CMAKE_CURRENT_BINARY_DIR}/bin/Debug"

    RUNTIME_OUTPUT_DIRECTORY_RELEASE
        "${CMAKE_CURRENT_BINARY_DIR}/bin/Release"
)
