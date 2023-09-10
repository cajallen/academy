#include "render_scene.hpp"

#include <functional>
#include <tracy/Tracy.hpp>
#include <vuk/Partials.hpp>

#include "extension/imgui_extra.hpp"
#include "extension/vuk_extra.hpp"
#include "general/logger.hpp"
#include "general/input.hpp"
#include "general/math/matrix_math.hpp"

#include "samplers.hpp"
#include "viewport.hpp"
#include "renderable.hpp"
#include "draw_functions.hpp"
#include "fmt_renderer.hpp"
#include "assets/particles.hpp"
#include "assets/material.hpp"
#include "assets/texture.hpp"

namespace spellbook {

void RenderScene::setup(vuk::Allocator& allocator) {
    scene_data.ambient               = Color(palette::white, 0.15f);
    scene_data.fog_color             = palette::black;
    scene_data.fog_depth             = -1.0f;
    scene_data.sun_direction         = quat(0.2432103f, 0.3503661f, 0.0885213f, 0.9076734f);
    scene_data.sun_intensity         = 1.0f;

    MeshCPU cube = generate_cube(v3(0.0f, 0.0f, 0.0f), v3(1.0f, 1.0f, 1.0f));
    MaterialCPU white_material{.color_tint = palette::white};
    white_material.file_path = FilePath("white_mat", true);
    uint64 mat_id = upload_material(white_material);
    quick_renderable(cube, mat_id, false);
}

void RenderScene::image(v2i size) {
    viewport.start = v2i(ImGui::GetWindowPos()) + v2i(ImGui::GetCursorPos());
    update_size(math::max(size, v2i(2, 2)));

    auto si = vuk::make_sampled_image(render_target.view.get(), Sampler().get());
    ImGui::Image(&*get_renderer().imgui_images.emplace(si), ImGui::GetContentRegionAvail());
}

void RenderScene::settings_gui() {
    ImGui::Checkbox("Pause", &user_pause);
    if (ImGui::TreeNode("Lighting")) {
        ImGui::ColorEdit4("Ambient", scene_data.ambient.data);
        uint32 id = ImGui::GetID("Sun Direction");
        ImGui::DragFloat("Sun Intensity", &scene_data.sun_intensity, 0.01f);
        ImGui::EnumCombo("Debug Mode", &post_process_data.debug_mode);
        ImGui::TreePop();
    }
    ImGui::Text("Viewport");
    inspect(&viewport);
}

void RenderScene::update_size(v2i new_size) {
    render_target = vuk::allocate_texture(*get_renderer().global_allocator, vuk::Format::eB8G8R8A8Unorm, vuk::Extent3D(new_size));
    viewport.update_size(new_size);
}

Renderable* RenderScene::add_renderable(const Renderable& renderable) {
    return &*renderables.emplace(renderable);
}

void RenderScene::delete_renderable(Renderable* renderable) {
    renderables.erase(renderables.get_iterator(renderable));
}

void RenderScene::upload_buffer_objects(vuk::Allocator& allocator) {
    ZoneScoped;
    
    struct CameraData {
        m44GPU vp;
    };
    CameraData cam_data;
    cam_data.vp     = m44GPU(viewport.camera->vp);
    CameraData sun_cam_data;
    v3 sun_vec = math::normalize(math::rotate(scene_data.sun_direction, v3::Z));
    sun_cam_data.vp     = m44GPU(math::orthographic(v3(30.0f, 30.0f, 40.0f)) * math::look(sun_vec * 25.0f, -sun_vec, v3::Z));

    auto [pubo_camera, fubo_camera] = vuk::create_buffer(allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&cam_data, 1));
    buffer_camera_data              = *pubo_camera;
    
    auto [pubo_sun_camera, fubo_sun_camera] = vuk::create_buffer(allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&sun_cam_data, 1));
    buffer_sun_camera_data              = *pubo_sun_camera;

    struct CompositeData {
        m44GPU inverse_vp;
        v4 camera_position;
        
        m44GPU light_vp;
        v4 sun_data;
        v4 ambient;
    } composite_data;
    composite_data.inverse_vp = m44GPU(math::inverse(viewport.camera->vp));
    composite_data.camera_position = v4(viewport.camera->position, 1.0f);
    composite_data.light_vp = sun_cam_data.vp;
    composite_data.sun_data = v4(sun_vec, 1.0f);
    composite_data.ambient = v4(scene_data.ambient);

    auto [pubo_composite, fubo_composite] = vuk::create_buffer(allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&composite_data, 1));
    buffer_composite_data             = *pubo_composite;
}

void RenderScene::setup_renderables_for_passes(vuk::Allocator& allocator) {
    ZoneScoped;
    renderables_built.clear();

    uint32 count = 0;
    
    for (auto& renderable : renderables) {
        if (!get_gpu_asset_cache().materials.contains(renderable.material_id))
            continue;
        if (!get_gpu_asset_cache().meshes.contains(renderable.mesh_id))
            continue;

        auto& mat_map = renderables_built.try_emplace(renderable.material_id).first->second;
        auto& mesh_list = mat_map.try_emplace(renderable.mesh_id).first->second;
        mesh_list.emplace_back(renderable.selection_id, &renderable.transform);
        count++;
    }

    uint32 model_buffer_size = sizeof(m44GPU) * (count + widget_renderables.size());
    uint32 id_buffer_size = sizeof(uint32) * count;
    
    buffer_model_mats = **vuk::allocate_buffer(allocator, {vuk::MemoryUsage::eCPUtoGPU, model_buffer_size, 1});
    buffer_ids = **vuk::allocate_buffer(allocator, {vuk::MemoryUsage::eCPUtoGPU, id_buffer_size, 1});
    int i = 0;
    for (const auto& [mat_hash, mat_map] : renderables_built) {
        for (const auto& [mesh_hash, mesh_list] : mat_map) {
            for (auto& [id, transform] : mesh_list) {
                memcpy((m44GPU*) buffer_model_mats.mapped_ptr + i, transform, sizeof(m44GPU));
                *((uint32*) buffer_ids.mapped_ptr + i) = id;
                i++;
            }
        }
    }
    
    for (const auto& renderable : widget_renderables) {
        memcpy((m44GPU*) buffer_model_mats.mapped_ptr + i++, &renderable.transform, sizeof(m44GPU));
    }

}


void RenderScene::clear_frame_allocated_renderables() {
    for (auto it = renderables.begin(); it != renderables.end();) {
        if (it->frame_allocated)
            it = renderables.erase(it);
        else
            ++it;
    }
    for (auto it = widget_renderables.begin(); it != widget_renderables.end();) {
        if (it->frame_allocated)
            it = widget_renderables.erase(it);
        else
            ++it;
    }
}

void RenderScene::pre_render() {
    if (viewport.size.x < 2 || viewport.size.y < 2) {
        update_size(v2i(2, 2));
        log_error("RenderScene::pre_render without RenderScene::image call", "renderer");
    }
    viewport.pre_render();
}


void RenderScene::update() {
}

vuk::Future RenderScene::render(vuk::Allocator& frame_allocator, vuk::Future target) {
    ZoneScoped;
    
    if (cull_pause || user_pause) {
        auto rg = make_shared<vuk::RenderGraph>("graph");
        rg->attach_image("target_output", vuk::ImageAttachment::from_texture(render_target));
        return vuk::Future {rg, "target_output"};
    }

    prune_emitters();
    
    for (Renderable& renderable : renderables) {
        upload_dependencies(renderable);
    }
    for (Renderable& renderable : widget_renderables) {
        upload_dependencies(renderable);
    }
    for (EmitterGPU& emitter : emitters) {
        upload_dependencies(emitter);
    }

    get_renderer().wait_for_futures();
    
    log(BasicMessage{.str = "render_scene render", .group = "render_scene", .frame_tags = {"render_scene"}});

    upload_buffer_objects(frame_allocator);
    setup_renderables_for_passes(frame_allocator);
    
    auto rg = make_shared<vuk::RenderGraph>("graph");
    rg->attach_in("target_input", std::move(target));
    
    post_process_data.time = Input::time;

    add_emitter_update_pass(rg);
    add_sundepth_pass(rg);
    add_forward_pass(rg);
    add_widget_pass(rg);
    add_postprocess_pass(rg);
    add_info_read_pass(rg);
    
    return vuk::Future {rg, "target_output"};
}

void RenderScene::prune_emitters() {
    for (auto it = emitters.begin(); it != emitters.end();) {
        if (it->deinstance_at <= (Input::time - Input::delta_time - 0.1f))
            it = emitters.erase(it);
        else
            it++;
    }
    std::erase_if(emitters, [](const EmitterGPU& emitter) {
        return emitter.deinstance_at <= (Input::time - Input::delta_time - 0.1f);
    });
}

void RenderScene::add_sundepth_pass(std::shared_ptr<vuk::RenderGraph> rg) {
    rg->add_pass({
        .name = "sun_depth",
        .resources = {
            "sun_depth_input"_image   >> vuk::eDepthStencilRW >> "sun_depth_output"
        },
        .execute = [this](vuk::CommandBuffer& command_buffer) {
            ZoneScoped;
            // Prepare render
            command_buffer.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
                .set_viewport(0, vuk::Rect2D{vuk::Sizing::eAbsolute, {}, {4096, 4096}})
                .set_scissor(0, vuk::Rect2D{vuk::Sizing::eAbsolute, {}, {4096, 4096}})
                .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                    .depthTestEnable  = true,
                    .depthWriteEnable = true,
                    .depthCompareOp   = vuk::CompareOp::eGreaterOrEqual, // EQUAL can be used for multipass things
                })
                .broadcast_color_blend({vuk::BlendPreset::eOff});
                
            command_buffer
                .bind_buffer(0, CAMERA_BINDING, buffer_sun_camera_data)
                .bind_buffer(0, MODEL_BINDING, buffer_model_mats)
                .bind_buffer(0, ID_BINDING, buffer_ids);

            command_buffer
                .set_rasterization({.cullMode = vuk::CullModeFlagBits::eNone})
                .bind_graphics_pipeline("directional_depth");
    
            int item_index = 0;
            for (const auto& [mat_hash, mat_map] : renderables_built) {
                for (const auto& [mesh_hash, mesh_list] : mat_map) {
                    MeshGPU* mesh = get_gpu_asset_cache().meshes.contains(mesh_hash) ? &get_gpu_asset_cache().meshes[mesh_hash] : nullptr;
                    command_buffer
                        .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
                        .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);
                    command_buffer.draw_indexed(mesh->index_count, mesh_list.size(), 0, 0, item_index);
                    item_index += mesh_list.size();
                }
            }
        }
    });

    rg->attach_and_clear_image("sun_depth_input", {.extent = {.extent = {4096, 4096, 1}}, .format = vuk::Format::eD16Unorm, .sample_count = vuk::Samples::e1}, vuk::ClearDepthStencil{0.0f, 0});
}

void RenderScene::add_forward_pass(std::shared_ptr<vuk::RenderGraph> rg) {
    ZoneScoped;
    rg->add_pass({
        .name = "forward",
        .resources = {
            "base_color_input"_image >> vuk::eColorWrite     >> "base_color_output",
            "emissive_input"_image >> vuk::eColorWrite     >> "emissive_output",
            "normal_input"_image  >> vuk::eColorWrite     >> "normal_output",
            "info_input"_image    >> vuk::eColorWrite     >> "info_output",
            "depth_input"_image   >> vuk::eDepthStencilRW >> "depth_output"
        },
        .execute = [this](vuk::CommandBuffer& command_buffer) {
            ZoneScoped;
            // Prepare render
            command_buffer.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
                .set_viewport(0, vuk::Rect2D::framebuffer())
                .set_scissor(0, vuk::Rect2D::framebuffer())
                .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                          .depthTestEnable  = true,
                          .depthWriteEnable = true,
                          .depthCompareOp   = vuk::CompareOp::eGreaterOrEqual, // EQUAL can be used for multipass things
                })
                .broadcast_color_blend({vuk::BlendPreset::eAlphaBlend})
                .set_color_blend("base_color_input", vuk::BlendPreset::eAlphaBlend)
                .set_color_blend("emissive_input", vuk::BlendPreset::eAlphaBlend)
                .set_color_blend("normal_input", vuk::BlendPreset::eOff)
                .set_color_blend("info_input", vuk::BlendPreset::eOff);
            
            command_buffer
                .bind_buffer(0, CAMERA_BINDING, buffer_camera_data)
                .bind_buffer(0, MODEL_BINDING, buffer_model_mats)
                .bind_buffer(0, ID_BINDING, buffer_ids);

            int item_index = 0;
            for (const auto& [mat_hash, mat_map] : renderables_built) {
                MaterialGPU* material = get_gpu_asset_cache().materials.contains(mat_hash) ?& get_gpu_asset_cache().materials[mat_hash] : nullptr;
                command_buffer
                    .set_rasterization({.cullMode = material->cull_mode})
                    .bind_graphics_pipeline(material->pipeline);
                material->bind_parameters(command_buffer);
                material->bind_textures(command_buffer);
                for (const auto& [mesh_hash, mesh_list] : mat_map) {
                    MeshGPU* mesh = get_gpu_asset_cache().meshes.contains(mesh_hash) ? &get_gpu_asset_cache().meshes[mesh_hash] : nullptr;
                    command_buffer
                        .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
                        .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);
                    command_buffer.draw_indexed(mesh->index_count, mesh_list.size(), 0, 0, item_index);
                    item_index += mesh_list.size();
                }
            }
            
            for (auto& emitter : emitters) {
                render_particles(emitter, command_buffer);
            }
        }
    });

    rg->attach_and_clear_image("base_color_input", {.format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1}, vuk::ClearColor(scene_data.fog_color));
    rg->attach_and_clear_image("emissive_input", {.format = vuk::Format::eR16G16B16A16Sfloat}, vuk::ClearColor {0.0f, 0.0f, 0.0f, 0.0f});
    rg->attach_and_clear_image("normal_input", {.format = vuk::Format::eR16G16B16A16Sfloat}, vuk::ClearColor {0.0f, 0.0f, 0.0f, 0.0f});
    rg->attach_and_clear_image("depth_input", {.format = vuk::Format::eD32Sfloat}, vuk::ClearDepthStencil{0.0f, 0});
    rg->attach_and_clear_image("info_input", {.format = vuk::Format::eR32Uint}, vuk::ClearColor {-1u, -1u, -1u, -1u});
    rg->inference_rule("base_color_input", vuk::same_extent_as("target_input"));
}

void RenderScene::add_widget_pass(std::shared_ptr<vuk::RenderGraph> rg) {
    ZoneScoped;
    rg->add_pass({
    .name = "widget",
    .resources = {
        "widget_input"_image >> vuk::eColorWrite     >> "widget_output",
        "widget_depth_input"_image   >> vuk::eDepthStencilRW >> "widget_depth_output"
    },
    .execute = [this](vuk::CommandBuffer& command_buffer) {
        ZoneScoped;
        if (render_widgets) {
            // Prepare render
            command_buffer.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
                .set_viewport(0, vuk::Rect2D::framebuffer())
                .set_scissor(0, vuk::Rect2D::framebuffer())
                .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                          .depthTestEnable  = true,
                          .depthWriteEnable = true,
                          .depthCompareOp   = vuk::CompareOp::eGreaterOrEqual, // EQUAL can be used for multipass things
                })
                .broadcast_color_blend({vuk::BlendPreset::eAlphaBlend});
                
            command_buffer
                .bind_buffer(0, CAMERA_BINDING, buffer_camera_data)
                .bind_buffer(0, MODEL_BINDING, buffer_model_mats);
            // Render items
            int item_index = 0 + renderables.size();
            for (Renderable& renderable : widget_renderables) {
                render_widget(renderable, command_buffer, &item_index);
            }
        }
    }});

    rg->attach_and_clear_image("widget_input", {.format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1}, vuk::ClearColor{0.0f, 0.0f, 0.0f, 0.0f});
    rg->attach_and_clear_image("widget_depth_input", {.format = vuk::Format::eD32Sfloat}, vuk::ClearDepthStencil{0.0f, 0});
    rg->inference_rule("widget_input", vuk::same_extent_as("target_input"));
}


void RenderScene::add_postprocess_pass(std::shared_ptr<vuk::RenderGraph> rg) {
    ZoneScoped;
    rg->add_pass(vuk::Pass {
    .name = "postprocess_apply",
    .resources = {
        "base_color_output"_image >> vuk::eComputeSampled,
        "emissive_output"_image >> vuk::eComputeSampled,
        "normal_output"_image  >> vuk::eComputeSampled,
        "depth_output"_image   >> vuk::eComputeSampled,
        "widget_output"_image >> vuk::eComputeSampled,
        "widget_depth_output"_image >> vuk::eComputeSampled,
        "sun_depth_output"_image >> vuk::eComputeSampled,
        "target_input"_image   >> vuk::eComputeWrite >> "target_output",
    },
    .execute =
        [this](vuk::CommandBuffer& cmd) {
            ZoneScoped;
            cmd.bind_compute_pipeline("postprocess");

            auto sampler = Sampler().filter(Filter_Linear).get();
            auto sun_sampler = Sampler().filter(Filter_Nearest).address(Address_Border).get();
            sun_sampler.borderColor = vuk::BorderColor::eFloatTransparentBlack;
            cmd.bind_image(0, 0, "base_color_output").bind_sampler(0, 0, sampler);
            cmd.bind_image(0, 1, "emissive_output").bind_sampler(0, 1, sampler);
            cmd.bind_image(0, 2, "normal_output").bind_sampler(0, 2, sampler);
            cmd.bind_image(0, 3, "depth_output").bind_sampler(0, 3, sampler);
            cmd.bind_image(0, 4, "widget_output").bind_sampler(0, 4, sampler);
            cmd.bind_image(0, 5, "widget_depth_output").bind_sampler(0, 5, sampler);
            cmd.bind_image(0, 6, "sun_depth_output").bind_sampler(0, 6, sun_sampler);
            cmd.bind_image(0, 7, "target_input");

            cmd.bind_buffer(0, 8, buffer_composite_data);
            
            auto target      = *cmd.get_resource_image_attachment("target_input");
            auto target_size = target.extent.extent;
            cmd.specialize_constants(0, target_size.width);
            cmd.specialize_constants(1, target_size.height);

            cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, post_process_data);

            cmd.dispatch_invocations(target_size.width, target_size.height);
        },
    });
}

void RenderScene::add_info_read_pass(std::shared_ptr<vuk::RenderGraph> rg) {
    if (math::contains(range2i(v2i(0), v2i(viewport.size)), query)) {
        auto info_storage_buffer = **vuk::allocate_buffer(*get_renderer().global_allocator, { vuk::MemoryUsage::eGPUtoCPU, sizeof(uint32), 1});
        rg->attach_buffer("info_storage", info_storage_buffer);
        rg->add_pass({
            .name  = "read",
            .resources = { 
                "info_output"_image   >> vuk::eTransferRead,
                "info_storage"_buffer >> vuk::eMemoryWrite >> "info_readable"},
            .execute = [this](vuk::CommandBuffer& command_buffer) {
                command_buffer.copy_image_to_buffer("info_output", "info_storage", {
                    .imageSubresource = {.aspectMask = vuk::ImageAspectFlagBits::eColor},
                    .imageOffset = {query.x, query.y, 0},
                    .imageExtent = {1,1,1}
                });
                query = v2i(-1, -1);
            }
        });
        fut_query_result = vuk::Future{rg, "info_readable"};
    }
}

void RenderScene::add_emitter_update_pass(std::shared_ptr<vuk::RenderGraph> rg) {
    rg->add_pass({
        .name = "emitter_update",
        .resources = {},
        .execute = [this](vuk::CommandBuffer& command_buffer) {
            for (auto& emitter : emitters) {
                update_emitter(emitter, command_buffer);
            }
        }
    });
}

void RenderScene::cleanup() {
}

void widget_setup() {
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    vuk::PipelineBaseCreateInfo pci;
    pci.add_glsl(get_contents(shader_path("widget.vert")), shader_path("widget.vert").abs_string());
    pci.add_glsl(get_contents(shader_path("widget.frag")), shader_path("widget.frag").abs_string());
    get_renderer().context->create_named_pipeline("widget", pci);
    
    MaterialCPU widget_mat = {.shader_name = "widget"};
    widget_mat.file_path = FilePath("widget", true);
    upload_material(widget_mat);
}

Renderable& RenderScene::quick_mesh(const MeshCPU& mesh_cpu, bool frame_allocated, bool widget) {
    if (widget)
        widget_setup();
    
    Renderable r;
    r.mesh_id = upload_mesh(mesh_cpu, frame_allocated);
    r.material_id = hash_view(widget ? "widget" : "default");
    r.frame_allocated = frame_allocated;

    return *(widget ? widget_renderables : renderables).emplace(r);
}

void material_setup() {
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    upload_mesh(generate_icosphere(3));
}

Renderable& RenderScene::quick_material(const MaterialCPU& material_cpu, bool frame_allocated) {
    material_setup();
    
    Renderable r;
    r.mesh_id = hash_view("icosphere_subdivisions:3");
    r.material_id = upload_material(material_cpu, frame_allocated);
    r.frame_allocated = frame_allocated;
    
    return *renderables.emplace(r);
}

Renderable& RenderScene::quick_renderable(uint64 mesh_id, uint64 mat_id, bool frame_allocated) {
    material_setup();
    
    Renderable r;
    r.mesh_id = mesh_id;
    r.material_id = mat_id;
    r.frame_allocated = frame_allocated;
    
    return *renderables.emplace(r);
}

Renderable& RenderScene::quick_renderable(const MeshCPU& mesh, uint64 mat_id, bool frame_allocated) {
    material_setup();
    
    Renderable r;
    r.mesh_id = upload_mesh(mesh);
    r.material_id = mat_id;
    r.frame_allocated = frame_allocated;
    
    return *renderables.emplace(r);
}

Renderable& RenderScene::quick_renderable(uint64 mesh_id, const MaterialCPU& mat, bool frame_allocated) {
    material_setup();
    
    Renderable r;
    r.mesh_id = mesh_id;
    r.material_id = upload_material(mat);
    r.frame_allocated = frame_allocated;
    
    return *renderables.emplace(r);
}

}
