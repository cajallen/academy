#pragma once

#include "renderer/renderer.hpp"
#include "editor/gui.hpp"

namespace spellbook {

struct GameScene;

struct Editor {
    Renderer renderer;
    GUI      gui;
    
    string external_resource_folder;
    string resource_folder;
    string user_folder;

    void startup();
    void run();
    void step(bool skip_input);
    void shutdown();
};

extern Editor editor;

}