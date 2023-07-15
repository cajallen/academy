#pragma once

namespace spellbook {

struct GameDisplay : EditorScene {
    void setup() override;
    void update() override;
    void window(bool* p_open) override;
    void shutdown() override;
};

}
