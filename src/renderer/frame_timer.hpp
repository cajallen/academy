#pragma once

#include <array>

namespace spellbook {

struct FrameTimer {
    int               ptr         = 0;
    int               filled      = 0;
    std::array<float, 200> frame_times = {};
    std::array<float, 200> delta_times = {};

    void update();
    void inspect();
};

}