#pragma once

#include "renderer/render_scene.hpp"

namespace spellbook {

struct GameScene {
    RenderScene render_scene;

    void setup() {}
    void update() {}
    void shutdown() {}
};

}