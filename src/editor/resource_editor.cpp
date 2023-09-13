#include "resource_editor.hpp"

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/imgui_extra.hpp"
#include "extension/icons/font_awesome4.h"
#include "general/input.hpp"
#include "editor/editor.hpp"
#include "game/process_launch.hpp"

namespace spellbook {

void ResourceEditor::setup() {
    EditorScene::setup();

    fs::path resource_editor_file = FilePath("user/").abs_path() / ("resource_editor" + string(Resource::extension()));

    json j = fs::exists(resource_editor_file) ? parse_file(resource_editor_file.string()) : json{};
    if (j.contains("tab"))
        external_tab_selection = from_jv<Tab>(*j.at("tab"));

    if (j.contains("model_path")) {
        model_cpu = load_resource<ModelCPU>(from_jv<FilePath>(*j.at("model_path")), false, true);
        switch_tab(Tab_Model);
    }

    controller.name = name + "::controller";
    controller.setup(&scene->render_scene.viewport, &scene->camera);

    scene->camera.position = v3(0.0f, -16.0f, 1.45f);

    scene->camera.fov = math::d2r(30.0f);
    scene->camera.heading = math::d2r(euler{.yaw = 90.0f, .pitch = -6.0f});
    scene->camera.view_dirty = true;
    scene->camera.proj_dirty = true;
}

void ResourceEditor::shutdown() {
    controller.cleanup();
    EditorScene::shutdown();

    fs::path resource_editor_file = FilePath("user/").abs_path() / ("resource_editor" + string(Resource::extension()));
    fs::create_directories(resource_editor_file.parent_path());
    
    auto j = json();
    TO_JSON_MEMBER(current_tab);
    j["model_path"] = make_shared<json_value>(to_jv(model_cpu.file_path));
    file_dump(j, resource_editor_file.string());
}

void ResourceEditor::update() {
    if (Input::mouse_click[GLFW_MOUSE_BUTTON_LEFT]) {
        scene->render_scene.query = v2i(Input::mouse_pos) - scene->render_scene.viewport.start;
    }

    scene->update();
    controller.update();
}

void ResourceEditor::switch_tab(Tab new_tab) {
    auto& render_scene = scene->render_scene;
    scene->registry.clear();
    
    switch (current_tab) {
        case (Tab_Model): {
            deinstance_model(render_scene, model_gpu);
        } break;
        case (Tab_Material): {
        } break;
        default: break;
    }
    
    switch (new_tab) {
        case (Tab_Model): {
            model_gpu = instance_model(render_scene, model_cpu);
        } break;
        case (Tab_Material): {
        } break;
    }
    current_tab = new_tab;
}

template<typename T>
void asset_tab(ResourceEditor& resource_editor, const string& name, ResourceEditor::Tab type, T* resource_value) {
    if (ImGui::BeginTabItem(name.c_str(), nullptr, type == resource_editor.external_tab_selection ? ImGuiTabItemFlags_SetSelected : 0)) {
        if (ImGui::Button("Save##ResourceTab")) {
            save_resource(*resource_value);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load##ResourceTab")) {
            *resource_value = load_resource<T>(resource_value->file_path, false, true);
            resource_editor.switch_tab(type);
        }
        inspect(resource_value, &resource_editor.scene->render_scene);
        ImGui::EndTabItem();
    }
}

void ResourceEditor::info_window(bool* p_open) {
    if (ImGui::Begin("Resource Editor Info", p_open)) {
//        if (ImGui::Button("Spawn Server")) {
//            launch_subprocess("academy_server.exe"_fp);
//        }
//
//        if (ImGui::Button("Spawn Client")) {
//            launch_subprocess("src/academy_client.exe"_fp);
//        }
//
//        ImGui::Separator();
//        ImGui::Separator();
//        ImGui::Separator();


        if (ImGui::BeginTabBar("Asset Types", ImGuiTabBarFlags_FittingPolicyScroll)) {
            asset_tab(*this, ICON_FA_SITEMAP "  Model", Tab_Model, &model_cpu);
            asset_tab(*this, ICON_FA_TINT "  Material", Tab_Material, &material_cpu);

            ImGui::EndTabBar();
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Scene")) {
            if (ImGui::TreeNode("Camera")) {
                inspect(&scene->camera);
                ImGui::TreePop();
            }
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            if (ImGui::TreeNode("Render Scene")) {
                scene->render_scene.settings_gui();
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

}
