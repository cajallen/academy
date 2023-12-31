#pragma once

#include "general/math/geometry.hpp"
#include "general/math/matrix.hpp"
#include "general/math/math.hpp"
#include "general/file/json.hpp"

namespace spellbook {

struct Camera {
    float fov        = math::d2r(30.0f);
    float clip_plane = 0.5f;
    float aspect_xy  = 1.0f;

    v3    position;
    euler heading; // euler_rad

    m44 view;
    m44 proj;
    m44 vp;

    bool view_dirty = true;
    bool proj_dirty = true;

    Camera() = default;
    Camera(v3 pos, euler rot);

    void update_proj();
    void update_view();

    void set_aspect_xy(float new_aspect_xy);

    void pre_render();
};

JSON_IMPL(Camera, position, heading);

void inspect(Camera* camera);

}
