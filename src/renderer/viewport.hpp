#pragma once

#include "general/math/geometry.hpp"

#include "camera.hpp"

namespace spellbook {

struct Viewport {
    string  name;
    Camera* camera;
    v2i     start;
    v2i     size;
    bool    size_dirty = true;
    void    update_size(v2i new_size);
    m44     proj_2d = {};

    bool hovered        = false;
    bool window_hovered = false;
    bool focused        = false;

    void pre_render();
    float  aspect_xy();

    void setup();
    ray3   ray(v2i screen_pos);

    v2 mouse_uv();
};

void inspect(Viewport* viewport);

}
