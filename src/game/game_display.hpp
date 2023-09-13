#pragma once

#include "general/string.hpp"
#include "general/memory.hpp"
#include "game/game_scene.hpp"
#include "extension/fmt.hpp"

namespace spellbook {

struct GameDisplay {
    string name;
    unique_ptr<GameScene> scene = {};

    GameDisplay() { scene = std::make_unique<GameScene>(); }

    virtual void setup() { scene->name = fmt_("{}::GameScene", name); scene->setup(); }
    virtual void update() { scene->update(); }
    virtual void shutdown() { scene->shutdown(); scene.reset(); }

    virtual void info_window(bool* open);
    virtual void output_window(bool* open);
};

}
