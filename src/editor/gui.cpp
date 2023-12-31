#include "gui.hpp"

#include <filesystem>
#include <imgui.h>
#include <tracy/Tracy.hpp>

#include "general/input.hpp"
#include "editor/editor.hpp"
#include "editor/console.hpp"
#include "editor/file_browser.hpp"
#include "editor/editor_scene.hpp"
#include "editor/widget_system.hpp"
#include "game/game_scene.hpp"
#include "renderer/draw_functions.hpp"

namespace fs = std::filesystem;

namespace spellbook {

void GUI::setup() {
    fs::path gui_file = FilePath("user/").abs_path() / ("gui" + string(Resource::extension()));

    WidgetSystem::setup();
    if (fs::exists(gui_file)) {
        json j = parse_file(gui_file.string());
        FROM_JSON_MEMBER(windows);
        FROM_JSON_MEMBER(item_state);
        FROM_JSON_MEMBER(file_browser_path);
    }
}

void GUI::shutdown() {
    fs::path gui_file = FilePath("user/").abs_path() / ("gui" + string(Resource::extension()));
    fs::create_directories(gui_file.parent_path());
    
    auto j = json();
    TO_JSON_MEMBER(windows);
    TO_JSON_MEMBER(item_state);
    TO_JSON_MEMBER(file_browser_path);
    file_dump(j, gui_file.string());
}


bool* GUI::window_open(string window_name) {
    if (windows.count(window_name) == 0) {
        windows[window_name] = {true, false};
    }
    return &windows[window_name].opened;
}

void GUI::_main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            for (auto& [k, v] : windows) {
                if (ImGui::MenuItem(k.c_str(), NULL, v.opened))
                    v.opened = !v.opened;
                v.queried = false;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void GUI::update() {
    ZoneScoped;

    WidgetSystem::update();
    
    // main
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID            dockspace_id    = ImGui::GetID("Dockspace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::End();

    _main_menu_bar();
    
    bool* p_open;

    if (*(p_open = window_open("demo")))
        ImGui::ShowDemoWindow(p_open);

    if (*(p_open = window_open("console")))
        Console::window(p_open);
    for (auto& scene : get_editor_scenes()) {
        if (*(p_open = window_open(scene->name + " info")))
            scene->info_window(p_open);
    }
    for (auto& scene : get_editor_scenes()) {
        if (*(p_open = window_open(scene->name + " output")))
            scene->output_window(p_open);
    }
    if (*(p_open = window_open("input")))
        Input::debug_window(p_open);
    if (*(p_open = window_open("renderer")))
        get_renderer().debug_window(p_open);
    if (*(p_open = window_open("colors")))
        color_window(p_open);
    if (*(p_open = window_open("file_browser"))) {
        file_browser("File Browser", p_open, &file_browser_path);
    }
}

}

