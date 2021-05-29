cmake_minimum_required(VERSION 3.20)
project(doon)

find_package(Vulkan REQUIRED)

include(shaders/CMakeRule.txt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} source/main.cpp
                               source/Window.cpp
                               source/VulkanApplication.cpp
                               source/Application.cpp
)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE include/)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

add_subdirectory(lib)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw
                                              glm
)
