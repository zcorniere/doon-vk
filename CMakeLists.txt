cmake_minimum_required(VERSION 3.20)
project(doon)

find_package(Vulkan REQUIRED)

include(shaders/CMakeRule.txt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CLANG_TIME_TRACE "Enable clang profiling." OFF)

if(CLANG_TIME_TRACE)
    message(STATUS "Clang profiling - enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftime-trace")
endif()

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

add_subdirectory(lib)

add_executable(${PROJECT_NAME} source/main.cpp
                               source/Window.cpp
                               source/VulkanApplication.cpp
                               source/VulkanApplication_helpers.cpp
                               source/VulkanApplication_static.cpp
                               source/vk_init.cpp
                               source/vk_utils.cpp
                               source/PipelineBuilder.cpp
                               source/Camera.cpp
                               source/Player.cpp
                               source/Scene.cpp
                               source/Swapchain.cpp
                               source/Application.cpp
)

target_precompile_headers(${PROJECT_NAME} PUBLIC <Logger.hpp> <unordered_map> <vector> <DeletionQueue.hpp>
                                                 <vulkan/vulkan.hpp> <glm/glm.hpp> <VulkanApplication.hpp>
                                                 <vk_mem_alloc.hpp>
)

add_shader(${PROJECT_NAME} default_triangle.vert)
add_shader(${PROJECT_NAME} default_triangle.frag)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  GLM_FORCE_INLINE
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
  GLFW_INCLUDE_NONE
  GLFW_INCLUDE_VULKAN
  MAX_PROJECTION_LIMIT=200.0f
  LOGGER_EXTERN_DECLARATION_PTR
  VULKAN_HPP_NO_CONSTRUCTORS
)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE include/)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE stb
                                              glfw
                                              glm
                                              imgui
                                              VulkanMemoryAllocator
                                              tinyobjloader
                                              logger
)
