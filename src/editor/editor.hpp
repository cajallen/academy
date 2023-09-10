#pragma once

#include "renderer/renderer.hpp"
#include "editor/gui.hpp"

namespace spellbook {

struct Editor {
    GUI      gui;

    void startup();
    void run();
    void step(bool skip_input);
    void shutdown();
};

inline Editor& get_editor() {
    static Editor editor;
    return editor;
}

}