#pragma once

#include "editor/editor_scene.hpp"
#include "editor/camera_controller.hpp"
#include "renderer/assets/model.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/mesh.hpp"


namespace spellbook {

struct ResourceEditor : EditorScene {
    enum Tab {
        Tab_None,
        Tab_Model,
        Tab_Material
    };

    Tab current_tab;
    Tab external_tab_selection;
    
    ModelCPU model_cpu;
    ModelGPU model_gpu;

    MaterialCPU material_cpu;

    string current_exe;

    CameraController controller;

    void setup() override;
    void update() override;
    void info_window(bool* p_open) override;
    void shutdown() override;

    void switch_tab(Tab tab);
};

}
