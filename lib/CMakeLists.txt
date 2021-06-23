add_subdirectory(glfw/ EXCLUDE_FROM_ALL)

add_subdirectory(glm/ EXCLUDE_FROM_ALL)

add_library(stb stb/stb_image.c)
target_include_directories(stb PUBLIC stb/)

