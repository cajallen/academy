
add_subdirectory(archive)
add_subdirectory(renderer)

add_library(academy_src
	editor/file_browser.cpp
    editor/console.cpp
    editor/editor.cpp
    editor/editor_scene.cpp
	editor/gui.cpp
    editor/pose_widget.cpp
    editor/widget_system.cpp
	editor/resource_editor.cpp
	editor/camera_controller.cpp
    game/game_scene.cpp
    game/process_launch.cpp
	game/game_display.cpp
)

target_include_directories(academy_src PUBLIC .)
target_link_libraries(academy_src PUBLIC archive renderer)
target_precompile_headers(academy_src PUBLIC archive/general/global.hpp)

add_custom_target(copy_icon ALL COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../icon.png ${CMAKE_CURRENT_BINARY_DIR}/icon.png)
add_custom_target(copy_shaders ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders)

add_executable(academy_server game/server/main.cpp game/server/server.cpp game/h_impl.cpp)
target_link_libraries(academy_server PUBLIC libs)
target_link_libraries(academy_server PUBLIC academy_src)
add_dependencies(academy_server copy_icon copy_shaders)


add_executable(academy_client game/client/main.cpp game/client/client.cpp game/h_impl.cpp)
target_link_libraries(academy_client PUBLIC libs)
target_link_libraries(academy_client PUBLIC academy_src)
add_dependencies(academy_client copy_icon copy_shaders)
