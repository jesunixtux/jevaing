cmake_minimum_required(VERSION 3.20)

project(
    Jevaing
    VERSION 0.0.11
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(JEVAING_BUILD_SANDBOX "Build the Jevaing Sandbox executable." ON)
option(JEVAING_BUILD_EDITOR "Build the Jevaing first-party editor." ON)
option(JEVAING_BUILD_XBOX_SMOKE_TEST "Build the experimental Xbox smoke test sample." ON)
option(JEVAING_ENABLE_PHYSICS_2D "Build Jevaing 2D physics support." ON)
option(JEVAING_ENABLE_PHYSICS_3D "Build Jevaing 3D physics support." ON)

include(FetchContent)

# External model import. Prefer an already-installed Assimp package and
# otherwise fetch a pinned version for reproducible development builds.
find_package(assimp CONFIG QUIET)

if(TARGET assimp AND NOT TARGET assimp::assimp)
    add_library(assimp::assimp ALIAS assimp)
endif()

if(NOT TARGET assimp::assimp)
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

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20
)

FetchContent_MakeAvailable(stb)

if(JEVAING_ENABLE_PHYSICS_2D)
    set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
    set(BOX2D_DOCS OFF CACHE BOOL "" FORCE)
    set(BOX2D_PROFILE OFF CACHE BOOL "" FORCE)
    set(BOX2D_VALIDATE OFF CACHE BOOL "" FORCE)
    set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        box2d
        GIT_REPOSITORY https://github.com/erincatto/box2d.git
        GIT_TAG v3.1.1
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(box2d)
endif()

if(JEVAING_ENABLE_PHYSICS_3D)
    set(DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
    set(GENERATE_DEBUG_SYMBOLS ON CACHE BOOL "" FORCE)
    set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
    set(CROSS_PLATFORM_DETERMINISTIC OFF CACHE BOOL "" FORCE)
    set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
    set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF CACHE BOOL "" FORCE)
    set(CPP_EXCEPTIONS_ENABLED ON CACHE BOOL "" FORCE)
    set(CPP_RTTI_ENABLED ON CACHE BOOL "" FORCE)
    set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
    set(OBJECT_LAYER_BITS 16 CACHE STRING "" FORCE)
    set(USE_AVX512 OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.6.0
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR Build
    )

    FetchContent_MakeAvailable(JoltPhysics)
endif()

if(JEVAING_BUILD_EDITOR AND WIN32)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG e0b59689461f03cbdc9aa394ea1512c1f0e6e246
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(imgui)

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    )

    target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )

    target_link_libraries(imgui PUBLIC d3d11 dxgi)
endif()

# Public API
add_subdirectory(Api)

# Jevaing core
add_library(JevaingEngine STATIC
    Engine/Build/BuildSystem.cpp
    Engine/Core/AssetManager.cpp
    Engine/Core/Application.cpp
    Engine/Core/CommandLine.cpp
    Engine/Core/Input.cpp
    Engine/Core/Logger.cpp
    Engine/Core/Project.cpp
    Engine/Core/Scene.cpp
    Engine/Core/Timer.cpp
    Engine/Core/Version.cpp
    Engine/Core/Window.cpp
    Engine/Physics/PhysicsCommon.cpp
    Engine/Physics/2D/Box2D/Box2DPhysicsBackend.cpp
    Engine/Physics/3D/Jolt/JoltPhysicsBackend.cpp
    Engine/Renderer/Renderer.cpp
    geometry/3D/ModelLoader.cpp
    geometry/3D/TextureLoader.cpp
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
            xinput
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
        ${stb_SOURCE_DIR}
)

target_link_libraries(JevaingEngine
    PUBLIC
        JevaingApi
    PRIVATE
        assimp::assimp
)

if(JEVAING_ENABLE_PHYSICS_2D AND TARGET box2d)
    target_compile_definitions(JevaingEngine PRIVATE JEVAING_HAS_BOX2D=1)
    target_link_libraries(JevaingEngine PRIVATE box2d)
endif()

if(JEVAING_BUILD_EDITOR AND WIN32)
    add_executable(JevaingEditor
        Editor/Application/EditorMain.cpp
    )

    target_link_libraries(JevaingEditor
        PRIVATE
            JevaingEngine
            imgui
            d3d11
            dxgi
    )

    target_include_directories(JevaingEditor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )

    target_compile_definitions(JevaingEditor PRIVATE
        UNICODE
        _UNICODE
    )

    set_target_properties(JevaingEditor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG
            "${CMAKE_CURRENT_SOURCE_DIR}/bin/Debug"

        RUNTIME_OUTPUT_DIRECTORY_RELEASE
            "${CMAKE_CURRENT_SOURCE_DIR}/bin/Release"
    )
endif()

if(JEVAING_BUILD_XBOX_SMOKE_TEST AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Samples/XboxSmokeTest/CMakeLists.txt")
    add_subdirectory(Samples/XboxSmokeTest)
endif()

if(JEVAING_ENABLE_PHYSICS_3D AND TARGET Jolt)
    target_compile_definitions(JevaingEngine PRIVATE JEVAING_HAS_JOLT=1)
    target_include_directories(JevaingEngine PRIVATE ${joltphysics_SOURCE_DIR}/..)
    target_link_libraries(JevaingEngine PRIVATE Jolt)
endif()

if(JEVAING_BUILD_SANDBOX)
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
endif()

if(MSVC)
    target_compile_options(JevaingEngine PRIVATE /W4)
    if(TARGET JevaingSandbox)
        target_compile_options(JevaingSandbox PRIVATE /W4)
    endif()
    if(TARGET JevaingEditor)
        target_compile_options(JevaingEditor PRIVATE /W4)
    endif()
else()
    target_compile_options(
        JevaingEngine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    if(TARGET JevaingSandbox)
        target_compile_options(
            JevaingSandbox PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()

    if(TARGET JevaingEditor)
        target_compile_options(
            JevaingEditor PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()
endif()
