#include "asset_editor.hpp"

#include "extension/icons/font_awesome4.h"
#include "editor/editor.hpp"
#include "game/input.hpp"
#include "game/game_file.hpp"

namespace spellbook {

void AssetEditor::setup() {
    EditorScene::setup();
    scene->render_scene.scene_data.ambient = Color(palette::white, 0.20f);
    
    fs::path asset_editor_file = fs::path(get_editor().user_folder) / ("asset_editor" + extension(FileType_General));
    
    json j = fs::exists(asset_editor_file) ? parse_file(asset_editor_file.string()) : json{};
    if (j.contains("tab"))
        external_tab_selection = from_jv<Tab>(*j.at("tab"));
}

void AssetEditor::shutdown() {
    EditorScene::shutdown();

    fs::path asset_editor_file = fs::path(get_editor().user_folder) / ("asset_editor" + extension(FileType_General));
    fs::create_directories(asset_editor_file.parent_path());
    
    auto j = json();
    TO_JSON_MEMBER(current_tab);
    file_dump(j, asset_editor_file.string());
}

void AssetEditor::update() {
    if (Input::mouse_click[GLFW_MOUSE_BUTTON_LEFT]) {
        scene->render_scene.query = v2i(Input::mouse_pos) - scene->render_scene.viewport.start;
    }

    scene->update();
}

void AssetEditor::switch_tab(Tab new_tab) {
    auto& render_scene = scene->render_scene;
    scene->registry.clear();
    
    switch (current_tab) {
        case (Tab_Model): {
        } break;
        case (Tab_Mesh): {
        } break;
        case (Tab_Material): {
        } break;
        default: break;
    }
    
    switch (new_tab) {
        case (Tab_Model): {
        } break;
        case (Tab_Mesh): {
        } break;
        case (Tab_Material): {
        } break;
    }
    current_tab = new_tab;
}

template<typename T>
void asset_tab(AssetEditor& asset_editor, const string& name, AssetEditor::Tab type, T* asset_value) {
    if (ImGui::BeginTabItem(name.c_str(), nullptr, type == asset_editor.external_tab_selection ? ImGuiTabItemFlags_SetSelected : 0)) {
        if (ImGui::Button("Save##AssetTab")) {
            save_asset(*asset_value);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load##AssetTab")) {
            *asset_value = load_asset<T>(asset_value->file_path, false, true);
            asset_editor.switch_tab(type);
        }
        ImGui::EndTabItem();
    }
}

template<>
void asset_tab(AssetEditor& asset_editor, const string& name, AssetEditor::Tab type, MeshCPU* asset_value) {
    if (ImGui::BeginTabItem(name.c_str(), nullptr, type == asset_editor.external_tab_selection ? ImGuiTabItemFlags_SetSelected : 0)) {
        ImGui::EndTabItem();
    }
}

void AssetEditor::window(bool* p_open) {
    if (ImGui::Begin("Asset Editor", p_open)) {
        if (ImGui::BeginTabBar("Asset Types", ImGuiTabBarFlags_FittingPolicyScroll)) {
            asset_tab(*this, ICON_FA_CODEPEN " Mesh", Tab_Mesh, &mesh_cpu);
            asset_tab(*this, ICON_FA_TINT " Material", Tab_Material, &material_cpu);
            asset_tab(*this, ICON_FA_SITEMAP " Model", Tab_Model, &model_cpu);

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

}
