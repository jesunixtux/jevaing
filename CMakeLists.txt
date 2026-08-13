cmake_minimum_required(VERSION 3.20)

project(
    Jevaing
    VERSION 0.0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# API pública
add_subdirectory(Api)

# Núcleo de Jevaing
add_library(JevaingEngine STATIC
    Engine/Core/Application.cpp
    Engine/Core/Version.cpp
)

if(WIN32)

    target_sources(JevaingEngine PRIVATE
        Engine/Platform/Windows/WindowsWindow.cpp
    )

    target_link_libraries(
        JevaingEngine
        PRIVATE
            user32
    )

endif()

target_include_directories(JevaingEngine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Core
)

target_link_libraries(JevaingEngine
    PUBLIC
        JevaingApi
)

# Sandbox de prueba
add_executable(JevaingSandbox
    Sandbox/main.cpp
)

target_link_libraries(JevaingSandbox
    PRIVATE
        JevaingEngine
)

# Salida de ejecutables
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
