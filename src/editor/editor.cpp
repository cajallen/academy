#include "editor.hpp"

#include <filesystem>
#include <tracy/Tracy.hpp>

#include "editor/console.hpp"
#include "editor/editor_scene.hpp"
#include "game/input.hpp"
#include "game/game_scene.hpp"

#include "editor/asset_editor.hpp"


namespace fs = std::filesystem;

namespace spellbook {

Editor editor;

ADD_EDITOR_SCENE(AssetEditor);

void Editor::startup() {
    external_resource_folder = (fs::current_path() / "external_resources").string();
    resource_folder = (fs::current_path() / "resources").string();
    user_folder = (fs::current_path() / "user").string();

    Console::setup();
    renderer.setup();
    Input::setup();

    for (auto& editor_scene : EditorScenes::values()) {
        editor_scene->setup();
    }
    
    gui.setup();
}

void Editor::run() {
    while (!Input::exit_accepted) {
        editor.step(false);
    }
}

void Editor::step(bool skip_input) {
    if (!skip_input)
        Input::update();
    renderer.update();
    gui.update();
    for (auto& editor_scene : EditorScenes::values()) {
        editor_scene->update();
    }
    renderer.render();
    FrameMark;
}

void Editor::shutdown() {
    gui.shutdown();
    for (auto& editor_scene : EditorScenes::values()) {
        editor_scene->shutdown();
    }
    renderer.cleanup();
}

}