cmake_minimum_required(VERSION 3.20)

project(Physics3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(JEVAING_BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../.." JevaingBuild)

add_executable(Physics3D
    Source/Game.cpp
)

target_link_libraries(Physics3D
    PRIVATE
        JevaingEngine
)

set_target_properties(Physics3D PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG
        "${CMAKE_CURRENT_BINARY_DIR}/bin/Debug"

    RUNTIME_OUTPUT_DIRECTORY_RELEASE
        "${CMAKE_CURRENT_BINARY_DIR}/bin/Release"
)
