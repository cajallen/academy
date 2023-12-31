#pragma once

#include <plf_colony.h>
#include <vuk/vuk_fwd.hpp>
#include <vuk/Buffer.hpp>
#include <vuk/Image.hpp>
#include <vuk/Future.hpp>

#include "general/string.hpp"
#include "general/color.hpp"
#include "general/memory.hpp"
#include "general/math/geometry.hpp"
#include "general/math/quaternion.hpp"

#include "viewport.hpp"
#include "renderable.hpp"
#include "assets/particles.hpp"

namespace spellbook {

using mesh_id = uint64;
using mat_id = uint64;

struct MeshCPU;
struct MaterialCPU;
struct SkeletonGPU;

enum DebugDrawMode {
    DebugDrawMode_Lit,
    DebugDrawMode_BaseColor,
    DebugDrawMode_Emissive,
    DebugDrawMode_Position,
    DebugDrawMode_Normal,
    DebugDrawMode_Depth,
    DebugDrawMode_Voxelization,
    DebugDrawMode_None
};

struct SceneData {
    Color ambient;
    Color fog_color;
    float fog_depth;
    quat sun_direction;
    float sun_intensity;
};

struct PostProcessData {
    DebugDrawMode debug_mode = DebugDrawMode_Lit;
    float voxel_size;
    float time;
};

struct RenderScene {
    string name;
    
    Viewport viewport;
    plf::colony<Renderable> renderables;
    plf::colony<Renderable> widget_renderables;
    plf::colony<EmitterGPU> emitters;
    bool render_widgets = true;
    
    SceneData       scene_data;
    PostProcessData post_process_data;

    v2i         query = v2i(-1, -1);
    vuk::Future fut_query_result;

    bool user_pause = false;
    bool cull_pause = false;
    vuk::Texture render_target;

    vuk::Buffer buffer_camera_data;
    vuk::Buffer buffer_voxelization_camera;
    vuk::Buffer buffer_sun_camera_data;
    vuk::Buffer buffer_composite_data;
    vuk::Buffer buffer_model_mats;
    vuk::Buffer buffer_ids;

    struct BuiltRenderable {
        uint32 id;
        m44GPU* mat;
    };
    umap<mat_id, umap<mesh_id, vector<BuiltRenderable>>> renderables_built;

    v3i voxelization_resolution;

    void update_size(v2i new_size);
    
    void        setup(vuk::Allocator& allocator);
    void        image(v2i size);
    void        settings_gui();
    void        pre_render();
    void        update();
    vuk::Future render(vuk::Allocator& allocator, vuk::Future target);
    void        cleanup();

    Renderable* add_renderable(const Renderable& renderable);
    void        delete_renderable(Renderable* renderable);

    Renderable& quick_mesh(const MeshCPU& mesh_cpu, bool frame_allocated, bool widget);
    Renderable& quick_material(const MaterialCPU& material_cpu, bool frame_allocated);
    Renderable& quick_renderable(const MeshCPU& mesh_id, uint64 mat_id, bool frame_allocated);
    Renderable& quick_renderable(uint64 mesh_id, uint64 mat_id, bool frame_allocated);
    Renderable& quick_renderable(uint64 mesh_id, const MaterialCPU& mat_id, bool frame_allocated);

    void add_sundepth_pass(shared_ptr<vuk::RenderGraph> rg);
    void add_voxelization_pass(shared_ptr<vuk::RenderGraph> rg);
    void add_forward_pass(shared_ptr<vuk::RenderGraph> rg);
    void add_widget_pass(shared_ptr<vuk::RenderGraph> rg);
    void add_postprocess_pass(shared_ptr<vuk::RenderGraph> rg);
    void add_info_read_pass(shared_ptr<vuk::RenderGraph> rg);
    void add_emitter_update_pass(shared_ptr<vuk::RenderGraph> rg);

    void prune_emitters();

    void upload_buffer_objects(vuk::Allocator& frame_allocator);
    void setup_renderables_for_passes(vuk::Allocator& allocator);
    void clear_frame_allocated_renderables();

    void generate_mips(shared_ptr<vuk::RenderGraph> rg, string_view input_name, string_view output_name, uint32 mip_count);
};


}
