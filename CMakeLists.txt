cmake_minimum_required(VERSION 3.16)

project(academy)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(libs)
add_subdirectory(src)

add_executable(academy_editor src/main.cpp)

target_link_libraries(academy_editor PUBLIC academy_libs)
target_link_libraries(academy_editor PUBLIC academy_src)

add_custom_target(copy_icon ALL COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/icon.png ${CMAKE_CURRENT_BINARY_DIR}/icon.png)
add_custom_target(copy_shaders ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders)
add_dependencies(academy_editor copy_icon copy_shaders)

target_compile_definitions(academy_editor PUBLIC STBI_MSC_SECURE_CRT)