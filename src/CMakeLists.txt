
add_subdirectory(archive)
add_subdirectory(renderer)

add_library(academy_src
    editor/asset_browser.cpp
    editor/console.cpp
    editor/editor.cpp
    editor/editor_scene.cpp
	editor/gui.cpp
    editor/pose_widget.cpp
    editor/widget_system.cpp
    editor/asset_editor.cpp
    game/input.cpp
	game/file_cache.cpp
	game/game_file.cpp
 "game/game_scene.cpp")

target_include_directories(academy_src PUBLIC .)
target_link_libraries(academy_src PUBLIC 
	archive
	academy_libs
	renderer
)
target_precompile_headers(academy_src PUBLIC archive/general/global.hpp)
target_compile_definitions(academy_src PUBLIC IMGUI_USER_CONFIG=\"imgui_config.h\")