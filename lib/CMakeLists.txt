add_subdirectory(glfw/ EXCLUDE_FROM_ALL)

add_subdirectory(glm/ EXCLUDE_FROM_ALL)

add_subdirectory(logger-cpp/ EXCLUDE_FROM_ALL)

add_subdirectory(tinyobjloader/ EXCLUDE_FROM_ALL)

set(VMA_USE_STL_CONTAINERS ON)
add_subdirectory(VulkanMemoryAllocator/ EXCLUDE_FROM_ALL)

add_library(stb stb/stb_image.c)
target_include_directories(stb PUBLIC stb/)

add_library(imgui imgui/imgui.cpp
                  imgui/imgui_widgets.cpp

                  imgui/imgui_demo.cpp
                  imgui/imgui_draw.cpp
                  imgui/imgui_widgets.cpp
                  imgui/imgui_tables.cpp

                  imgui/backends/imgui_impl_vulkan.cpp
                  imgui/backends/imgui_impl_glfw.cpp
)
target_include_directories(imgui PUBLIC imgui/)
target_link_libraries(imgui PRIVATE Vulkan::Vulkan glfw)
