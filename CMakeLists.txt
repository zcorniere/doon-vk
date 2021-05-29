cmake_minimum_required(VERSION 3.20)
project(doon)

find_package(Vulkan REQUIRED)

include(shaders/CMakeRule.txt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_C_LINKER_FLAGS "-fuse-ld=lld")
set(CMAKE_CXX_LINKER_FLAGS "-fuse-ld=lld")
set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")

add_subdirectory(lib)

add_executable(${PROJECT_NAME} source/main.cpp
                               source/Window.cpp
                               source/VulkanApplication.cpp
                               source/VulkanApplication_static.cpp
                               source/vk_init.cpp
                               source/Logger.cpp
                               source/Application.cpp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE 
            -DVULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE include/)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw
                                              glm
)
