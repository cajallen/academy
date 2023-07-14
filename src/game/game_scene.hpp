#pragma once

#include <entt/entity/registry.hpp>

#include "renderer/render_scene.hpp"

namespace spellbook {

struct GameScene {
    string name;

    Camera camera;
    RenderScene render_scene;
    entt::registry registry;

    void setup();
    void update();
    void shutdown();
};

}