#include "asset_editor.hpp"

#include "extension/icons/font_awesome4.h"
#include "editor/editor.hpp"
#include "game/input.hpp"
#include "game/game_file.hpp"

namespace spellbook {

void GameDisplay::setup() {
    EditorScene::setup();
}

void GameDisplay::shutdown() {
    EditorScene::shutdown();
}

void GameDisplay::update() {
    EditorScene::update();
}

void GameDisplay::info_window(bool* p_open) {
    if (ImGui::Begin("Game Display Info", p_open)) {
    }
    ImGui::End();
}

}
