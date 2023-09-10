#include "frame_timer.hpp"

#include <imgui.h>

#include "extension/fmt.hpp"
#include "general/math/math.hpp"
#include "general/input.hpp"

namespace spellbook {

void FrameTimer::update() {
    int last_index   = ptr;
    float last_time    = frame_times[ptr];
    ptr              = (ptr + 1) % 200;
    frame_times[ptr] = Input::time;

    if (filled > 1)
        delta_times[last_index] = frame_times[ptr] - last_time;
    filled = math::min(filled + 1, 200);
}

void FrameTimer::inspect() {
    float average = 0.0f;
    for (int n = 0; n < filled; n++)
        average += delta_times[n];
    average /= (float) filled;
    string overlay = fmt_("FPS: {:.1f}", 1.0f / average);
    ImGui::PlotLines("DT", delta_times.data(), filled, ptr, overlay.c_str(), 0.0f, 0.1f, ImVec2(0, 80.0f));
}

}