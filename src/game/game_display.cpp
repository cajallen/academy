#include "game_display.hpp"

#include <imgui.h>

namespace spellbook {

void GameDisplay::info_window(bool* p_open) {
    if (ImGui::Begin("Game Display Info", p_open)) {
    }
    ImGui::End();
}

void GameDisplay::output_window(bool* p_open) {
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
