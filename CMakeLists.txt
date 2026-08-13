cmake_minimum_required(VERSION 3.20)

project(
    Jevaing
    VERSION 0.0.7
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# External model import. Prefer an already-installed Assimp package and
# otherwise fetch a pinned version for reproducible development builds.
find_package(assimp CONFIG QUIET)

if(TARGET assimp AND NOT TARGET assimp::assimp)
    add_library(assimp::assimp ALIAS assimp)
endif()

if(NOT TARGET assimp::assimp)
    include(FetchContent)

    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    set(ASSIMP_IGNORE_GIT_HASH ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.5
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(assimp)
endif()

# Public API
add_subdirectory(Api)

# Jevaing core
add_library(JevaingEngine STATIC
    Engine/Core/Application.cpp
    Engine/Core/CommandLine.cpp
    Engine/Core/Input.cpp
    Engine/Core/Logger.cpp
    Engine/Core/Timer.cpp
    Engine/Core/Version.cpp
    Engine/Core/Window.cpp
    Engine/Renderer/Renderer.cpp
    geometry/3D/ModelLoader.cpp
    geometry/3D/Primitives/Cube.cpp
)

if(WIN32)
    target_sources(JevaingEngine PRIVATE
        Engine/Platform/Windows/WindowsWindow.cpp
        Engine/Renderer/DirectX/D3D11Renderer.cpp
    )

    target_compile_definitions(JevaingEngine PRIVATE
        UNICODE
        _UNICODE
    )

    target_link_libraries(
        JevaingEngine
        PRIVATE
            user32
            gdi32
            d3d11
            dxgi
            d3dcompiler
    )
endif()

target_compile_definitions(
    JevaingEngine
    PRIVATE
        JEVAING_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(JevaingEngine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core
)

target_link_libraries(JevaingEngine
    PUBLIC
        JevaingApi
    PRIVATE
        assimp::assimp
)

# Sandbox is now a real client of the public API.
add_executable(JevaingSandbox
    Sandbox/main.cpp
)

target_link_libraries(JevaingSandbox
    PRIVATE
        JevaingEngine
)

set_target_properties(JevaingSandbox PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG
        "${CMAKE_CURRENT_SOURCE_DIR}/bin/Debug"

    RUNTIME_OUTPUT_DIRECTORY_RELEASE
        "${CMAKE_CURRENT_SOURCE_DIR}/bin/Release"
)

if(MSVC)
    target_compile_options(JevaingEngine PRIVATE /W4)
    target_compile_options(JevaingSandbox PRIVATE /W4)
else()
    target_compile_options(
        JevaingEngine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    target_compile_options(
        JevaingSandbox PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
