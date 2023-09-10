#pragma once

#include "general/vector.hpp"
#include "general/navigation_path.hpp"

#include "renderer.hpp"
#include "camera.hpp"

namespace spellbook {

    struct Bitmask3D;

    struct FormattedVertex {
        v3 position;
        Color color;
        float width;

        static constexpr FormattedVertex separate() {
            return {v3(0.0f), Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f};
        }
    };

    MeshCPU generate_cube(v3 center, v3 extents, Color vertex_color = palette::black);
    MeshCPU generate_cylinder(v3 center, uint8 rotations, Color vertex_color = palette::black, v3 cap_axis = v3::Z, v3 axis_1 = v3::X, v3 axis_2 = v3::Y);
    MeshCPU generate_icosphere(int subdivisions);
    MeshCPU generate_formatted_line(Camera* camera, vector<FormattedVertex> vertices);
    MeshCPU generate_formatted_dot(Camera* camera, FormattedVertex vertex);
    MeshCPU generate_formatted_3d_bitmask(Camera* camera, const Bitmask3D& bitmask);
    MeshCPU generate_outline(Camera* camera, const Bitmask3D& bitmask, const vector<v3i>& places, const Color& color, float thickness);

    void add_formatted_square(vector<FormattedVertex>& vertices, v3 center, v3 axis_1, v3 axis_2, Color color, float width);

    void draw_path(RenderScene& render_scene, Path& path, const v3& pos);

}

