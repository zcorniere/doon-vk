add_subdirectory(glfw/ EXCLUDE_FROM_ALL)

add_subdirectory(glm/ EXCLUDE_FROM_ALL)

add_subdirectory(logger-cpp/ EXCLUDE_FROM_ALL)

add_subdirectory(tinyobjloader/ EXCLUDE_FROM_ALL)

# set(VMA_USE_STL_CONTAINERS ON)
# add_subdirectory(VulkanMemoryAllocator/ EXCLUDE_FROM_ALL)

add_library(VulkanMemoryAllocator STATIC VulkanMemoryAllocator-Hpp/vk_mem_alloc.h)
set_source_files_properties(
    VulkanMemoryAllocator-Hpp/vk_mem_alloc.h PROPERTIES
    LANGUAGE CXX
)
target_include_directories(VulkanMemoryAllocator PUBLIC VulkanMemoryAllocator-Hpp/)
target_compile_definitions(VulkanMemoryAllocator PRIVATE VMA_IMPLEMENTATION)
set_target_properties(VulkanMemoryAllocator PROPERTIES LINKER_LANGUAGE CXX)

add_library(stb STATIC stb/stb_image.h)
set_source_files_properties(
    stb/stb_image.h PROPERTIES
    LANGUAGE C
)
target_compile_definitions(stb PRIVATE STB_IMAGE_IMPLEMENTATION)
target_include_directories(stb PUBLIC stb/)
set_target_properties(stb PROPERTIES LINKER_LANGUAGE C)


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
