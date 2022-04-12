cmake_minimum_required(VERSION 3.20)
project(doon)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Looking for lld")
find_program(LLD_FOUND lld)
if (LLD_FOUND)
    message(STATUS "Looking for lld - found")
    set(CMAKE_C_LINKER_FLAGS "-fuse-ld=lld")
    set(CMAKE_CXX_LINKER_FLAGS "-fuse-ld=lld")
    set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
else()
    message(STATUS "Looking for lld - not found")
endif()

set(PIVOT_ASSET_DEFAULT_DIRECTORY "${CMAKE_SOURCE_DIR}/assets/")
set(BUILD_EDITOR OFF)
set(BUILD_LAUNCHER OFF)
set(LOCAL_PIVOT_DIRECTORY "" CACHE STRING "Path to a local copy of the Pivot game engine sources")
if (LOCAL_PIVOT_DIRECTORY STREQUAL "")
    FetchContent_Declare(
        pivot
        GIT_REPOSITORY https://github.com/EIP-Pivot/pivot.git
        GIT_TAG a23172da924718e0aad28bb15425ec702d4d4c43
    )
    if (NOT pivot_POPULATED)
        message(STATUS "Populating pivot")
        FetchContent_Populate(pivot)
        add_subdirectory(${pivot_SOURCE_DIR} ${pivot_BINARY_DIR})
    endif()
else()
    add_subdirectory(${LOCAL_PIVOT_DIRECTORY} ${CMAKE_BINARY_DIR}/game-engine)
endif()
target_compile_definitions(pivot-graphics PUBLIC
    PIVOT_MAX_PROJECTION_MATRIX=1000000000.0f
    CULLING_DEBUG=1
)

add_executable(${PROJECT_NAME} source/main.cpp
                               source/Application.cpp
                               source/Scene.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE include/)

target_link_libraries(${PROJECT_NAME} PRIVATE pivot::pivot)
