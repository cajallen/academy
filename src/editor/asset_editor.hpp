#pragma once

#include "editor/editor_scene.hpp"
#include "renderer/assets/model.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/mesh.hpp"


namespace spellbook {

struct AssetEditor : EditorScene {
    enum Tab {
        Tab_None,
        Tab_Model,
        Tab_Material,
        Tab_Mesh
    };

    Tab current_tab;
    Tab external_tab_selection;
    
    MeshCPU mesh_cpu;
    ModelCPU model_cpu;
    MaterialCPU material_cpu;

    string current_exe;

    void setup() override;
    void update() override;
    void info_window(bool* p_open) override;
    void shutdown() override;

    void switch_tab(Tab tab);
};

}
