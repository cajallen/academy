#pragma once

#include <imgui.h>

#include "editor_scene.hpp"

namespace spellbook {

void EditorScene::info_window(bool* p_open) {
}

void EditorScene::output_window(bool* p_open) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    if (ImGui::Begin((name + " Output").c_str(), p_open)) {
        scene->render_scene.viewport.window_hovered = ImGui::IsWindowHovered();
        scene->render_scene.image((v2i) ImGui::GetContentRegionAvail());
        scene->render_scene.cull_pause = false;
    } else {
        scene->render_scene.viewport.window_hovered = false;
        scene->render_scene.cull_pause = true;
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

}
