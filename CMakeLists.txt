cmake_minimum_required(VERSION 3.16)

project(academy)

set(CMAKE_CXX_STANDARD 20)

set(RESOURCE_PARENT_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Root directory for resources" FORCE)
set(FA_PATH "${CMAKE_CURRENT_BINARY_DIR}/fa.ttf" CACHE PATH "Font Awesome path" FORCE)

add_subdirectory(libs)
add_subdirectory(src)

add_executable(academy_editor src/main.cpp)

target_compile_definitions(academy_editor PUBLIC 
	STBI_MSC_SECURE_CRT
	RESOURCE_PARENT_DIR=\"${RESOURCE_PARENT_DIR}\"
)

target_link_libraries(academy_editor PUBLIC libs)
target_link_libraries(academy_editor PUBLIC academy_src)

add_custom_target(editor_copy_icon ALL COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/icon.png ${CMAKE_CURRENT_BINARY_DIR}/icon.png)
add_custom_target(editor_copy_font ALL COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/fa.ttf ${CMAKE_CURRENT_BINARY_DIR}/fa.ttf)
add_custom_target(editor_copy_shaders ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders)
add_custom_target(editor_copy_internal ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources)
add_custom_target(editor_copy_external ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/external_resources ${CMAKE_CURRENT_BINARY_DIR}/external_resources)
add_dependencies(academy_editor editor_copy_icon editor_copy_shaders editor_copy_font editor_copy_internal editor_copy_external)

add_dependencies(academy_editor academy_server academy_client)