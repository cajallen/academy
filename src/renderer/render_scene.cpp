#include "render_scene.hpp"

#include <functional>
#include <tracy/Tracy.hpp>
#include <vuk/Partials.hpp>

#include "extension/imgui_extra.hpp"
#include "extension/vuk_extra.hpp"
#include "extension/fmt.hpp"
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
    scene_data.ambient               = Color(palette::white, 0.2f);
    scene_data.fog_color             = palette::black;
    scene_data.fog_depth             = -1.0f;
    scene_data.sun_direction         = quat(0.2432103f, 0.3503661f, 0.0885213f, 0.9076734f);
    scene_data.sun_intensity         = 1.0f;

    voxelization_resolution = v3i(128);
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

    constexpr float voxelization_extent = 5.0f;
    post_process_data.voxel_size = 2.0f * voxelization_extent / voxelization_resolution.x;

    struct CameraData {
        m44GPU vp;
        v4 normal;
    };
    CameraData camera_data;
    camera_data.vp = m44GPU(viewport.camera->vp);
    camera_data.normal = v4(math::euler2vector(viewport.camera->heading), 1.0);

    m44GPU voxel_cam_data[3];
    voxel_cam_data[0] = m44GPU(
        math::voxelization_mat(v3{voxelization_extent, voxelization_extent, voxelization_extent}) *
        math::look(-voxelization_extent * v3::X, v3::X, v3::Z)
    );
    voxel_cam_data[1] = m44GPU(
        math::voxelization_mat(v3{voxelization_extent, voxelization_extent, voxelization_extent}) *
        math::look(-voxelization_extent * v3::Y, v3::Y, v3::Z)
    );
    voxel_cam_data[2] = m44GPU(
        math::voxelization_mat(v3{voxelization_extent, voxelization_extent, voxelization_extent}) *
        math::look(-voxelization_extent * v3::Z, v3::Z, v3::X)
    );

    CameraData sun_camera_data;
    v3 sun_vec = math::normalize(math::rotate(scene_data.sun_direction, v3::Z));
    sun_camera_data.vp     = m44GPU(math::orthographic(v3(10.0f, 10.0f, 30.0f)) * math::look(sun_vec * 15.0f, -sun_vec, v3::Z));
    sun_camera_data.normal = v4(-sun_vec, 1.0);
    auto [pubo_camera, fubo_camera] = vuk::create_buffer(allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&camera_data, 1));
    buffer_camera_data              = *pubo_camera;

    auto [pubo_sun_camera, fubo_sun_camera] = vuk::create_buffer(allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnTransfer, std::span(&sun_camera_data, 1));
    buffer_sun_camera_data              = *pubo_sun_camera;

    auto [pubo_voxelization_camera, fubo_voxelization_camera] = vuk::create_buffer(allocator, vuk::MemoryUsage::eCPUtoGPU, vuk::DomainFlagBits::eTransferOnTransfer, std::span(voxel_cam_data, 3));
    buffer_voxelization_camera = *pubo_voxelization_camera;

    struct CompositeData {
        m44GPU inverse_vp;
        v4 camera_position;

        m44GPU voxelization_vp;
        m44GPU light_vp;
        v4 sun_data;
        v4 ambient;

        int32 voxelization_lod = 0;
    } composite_data;
    composite_data.inverse_vp = m44GPU(math::inverse(viewport.camera->vp));
    composite_data.camera_position = v4(viewport.camera->position, 1.0f);
    composite_data.voxelization_vp = voxel_cam_data[0];
    composite_data.light_vp = sun_camera_data.vp;
    composite_data.sun_data = v4(sun_vec, 1.0f);
    composite_data.ambient = v4(scene_data.ambient);
    composite_data.voxelization_lod = 0;

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

    add_sundepth_pass(rg);
    add_voxelization_pass(rg);
    add_emitter_update_pass(rg);
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

void RenderScene::add_sundepth_pass(shared_ptr<vuk::RenderGraph> rg) {
    rg->add_pass({
        .name = "sun_depth",
        .resources = {
             "sun_depth_input"_image   >> vuk::eDepthStencilRW >> "sun_depth_output"
        },
        .execute = [this](vuk::CommandBuffer& command_buffer) {
            ZoneScoped;
            // Prepare render
            command_buffer.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
                    .set_viewport(0, vuk::Rect2D{vuk::Sizing::eAbsolute, {}, {2048, 2048}})
                    .set_scissor(0, vuk::Rect2D{vuk::Sizing::eAbsolute, {}, {2048, 2048}})
                    .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo{
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
            for (const auto &[mat_hash, mat_map]: renderables_built) {
                for (const auto &[mesh_hash, mesh_list]: mat_map) {
                    MeshGPU *mesh = get_gpu_asset_cache().meshes.contains(mesh_hash) ? &get_gpu_asset_cache().meshes[mesh_hash] : nullptr;
                    command_buffer
                            .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
                            .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);
                    command_buffer.draw_indexed(mesh->index_count, mesh_list.size(), 0, 0, item_index);
                    item_index += mesh_list.size();
                }
            }
        }
    });
    rg->attach_and_clear_image("sun_depth_input", {.extent = {.extent = {2048, 2048, 1}}, .format = vuk::Format::eD16Unorm, .sample_count = vuk::Samples::e1}, vuk::ClearDepthStencil{0.0f, 0});
}

void RenderScene::add_voxelization_pass(shared_ptr<vuk::RenderGraph> rg) {
    ZoneScoped;

    rg->add_pass({
        .name = "voxelization",
        .resources = {
            "sun_depth_output"_image >> vuk::eFragmentSampled,
            "voxelization_input"_image >> vuk::eFragmentWrite >> "voxelization",
            "fake_input"_image >> vuk::eColorWrite >> "fake_output"
        },
        .execute = [this](vuk::CommandBuffer& command_buffer) {
            command_buffer.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
                    .set_viewport(0, vuk::Rect2D{.extent={uint32(voxelization_resolution.x), uint32(voxelization_resolution.y)}})
                    .set_scissor(0, vuk::Rect2D{.extent={uint32(voxelization_resolution.x), uint32(voxelization_resolution.y)}})
                    .set_depth_stencil(vuk::PipelineDepthStencilStateCreateInfo {
                            .depthTestEnable  = false,
                            .depthWriteEnable = false
                    })
                    .broadcast_color_blend({vuk::BlendPreset::eOff})
                    .set_rasterization({.cullMode = vuk::CullModeFlagBits::eNone});

            command_buffer
                    .bind_buffer(0, CAMERA_BINDING, buffer_voxelization_camera)
                    .bind_buffer(0, MODEL_BINDING, buffer_model_mats)
                    .bind_buffer(0, ID_BINDING, buffer_sun_camera_data);

            command_buffer.bind_image(0, 8, "voxelization_input");
            command_buffer.bind_image(0, 9, "sun_depth_output").bind_sampler(0, 9, Sampler().filter(Filter_Nearest).get());

            int item_index = 0;
            for (const auto& [mat_hash, mat_map] : renderables_built) {
                MaterialGPU* material = get_gpu_asset_cache().materials.contains(mat_hash) ? &get_gpu_asset_cache().materials[mat_hash] : nullptr;
                command_buffer.bind_graphics_pipeline(get_renderer().context->get_named_pipeline("voxelization"));
                material->bind_parameters(command_buffer);
                material->bind_textures(command_buffer);
                for (const auto& [mesh_hash, mesh_list] : mat_map) {
                    MeshGPU* mesh = get_gpu_asset_cache().meshes.contains(mesh_hash) ? &get_gpu_asset_cache().meshes[mesh_hash] : nullptr;
                    command_buffer
                            .bind_vertex_buffer(0, mesh->vertex_buffer.get(), 0, Vertex::get_format())
                            .bind_index_buffer(mesh->index_buffer.get(), vuk::IndexType::eUint32);


                    for (uint32 i = 0; i < 3; i++) {
                        struct PC { v4i res; uint32 pass; };
                        PC pc {.res = v4i(voxelization_resolution, 0), .pass = i};
                        command_buffer.push_constants(vuk::ShaderStageFlagBits::eVertex | vuk::ShaderStageFlagBits::eFragment, 0, pc);
                        command_buffer.draw_indexed(mesh->index_count, mesh_list.size(), 0, 0, item_index);

                    }
                    item_index += mesh_list.size();
                }
            }
        }
    });

    vuk::ImageAttachment voxelization_format = {
        .image_type = vuk::ImageType::e3D,
        .extent = {.extent = {uint32(voxelization_resolution.x), uint32(voxelization_resolution.y), uint32(voxelization_resolution.z)}},
        .format = vuk::Format::eR16G16B16A16Sfloat,
        .sample_count = vuk::Samples::e1,
        .view_type = vuk::ImageViewType::e3D,
        .level_count = 6,
        .layer_count = 1
    };
    vuk::ImageAttachment fake_format = {
        .extent = {.extent = {uint32(voxelization_resolution.x), uint32(voxelization_resolution.y), 1}},
        .format = vuk::Format::eR8G8B8A8Srgb,
        .sample_count = vuk::Samples::e1
    };
    rg->attach_and_clear_image("voxelization_input", voxelization_format, vuk::ClearColor(0.0f, 0.0f, 0.0f, 0.0f));
    rg->attach_and_clear_image("fake_input", fake_format, vuk::ClearColor(0.0f, 0.0f, 0.0f, 0.0f));

    generate_mips(rg, "voxelization", "voxelization_mipped", 6);
}

void RenderScene::add_forward_pass(shared_ptr<vuk::RenderGraph> rg) {
    ZoneScoped;
    rg->attach_and_clear_image("base_color_input", {.format = vuk::Format::eR16G16B16A16Sfloat, .sample_count = vuk::Samples::e1}, vuk::ClearColor(scene_data.fog_color));
    rg->attach_and_clear_image("emissive_input", {.format = vuk::Format::eR16G16B16A16Sfloat}, vuk::ClearColor {0.0f, 0.0f, 0.0f, 0.0f});
    rg->attach_and_clear_image("normal_input", {.format = vuk::Format::eR16G16B16A16Sfloat}, vuk::ClearColor {0.0f, 0.0f, 0.0f, 0.0f});
    rg->attach_and_clear_image("depth_input", {.format = vuk::Format::eD32Sfloat}, vuk::ClearDepthStencil{0.0f, 0});
    rg->attach_and_clear_image("info_input", {.format = vuk::Format::eR32Uint}, vuk::ClearColor {-1u, -1u, -1u, -1u});
    rg->inference_rule("base_color_input", vuk::same_extent_as("target_input"));
    rg->add_pass({
        .name = "forward",
        .resources = {
            "base_color_input"_image >> vuk::eColorWrite  >> "base_color_output",
            "emissive_input"_image >> vuk::eColorWrite    >> "emissive_output",
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
}

void RenderScene::add_widget_pass(shared_ptr<vuk::RenderGraph> rg) {
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


void RenderScene::add_postprocess_pass(shared_ptr<vuk::RenderGraph> rg) {
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
        "voxelization_mipped"_image >> vuk::eComputeSampled,
        "sun_depth_output"_image >> vuk::eComputeSampled,
        "target_input"_image   >> vuk::eComputeWrite >> "target_output",
    },
    .execute =
        [this](vuk::CommandBuffer& cmd) {
            ZoneScoped;
            cmd.bind_compute_pipeline("postprocess");

            vuk::SamplerCreateInfo sampler = Sampler().filter(Filter_Linear).get();
            vuk::SamplerCreateInfo voxel_sampler = Sampler().address(Address_Clamp).filter(Filter_Linear).get();
            vuk::SamplerCreateInfo sun_sampler = Sampler().address(Address_Clamp).filter(Filter_Nearest).get();
            cmd.bind_image(0, 0, "base_color_output").bind_sampler(0, 0, sampler);
            cmd.bind_image(0, 1, "emissive_output").bind_sampler(0, 1, sampler);
            cmd.bind_image(0, 2, "normal_output").bind_sampler(0, 2, sampler);
            cmd.bind_image(0, 3, "depth_output").bind_sampler(0, 3, sampler);
            cmd.bind_image(0, 4, "widget_output").bind_sampler(0, 4, sampler);
            cmd.bind_image(0, 5, "widget_depth_output").bind_sampler(0, 5, sampler);
            cmd.bind_image(0, 6, "voxelization_mipped").bind_sampler(0, 6, voxel_sampler);
            cmd.bind_image(0, 9, "sun_depth_output").bind_sampler(0, 9, sun_sampler);
            cmd.bind_image(0, 7, "target_input");

            cmd.bind_buffer(0, 8, buffer_composite_data);
            
            vuk::ImageAttachment target = *cmd.get_resource_image_attachment("target_input");
            const vuk::Extent3D& target_size = target.extent.extent;
            cmd.specialize_constants(0, target_size.width);
            cmd.specialize_constants(1, target_size.height);

            cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, post_process_data);

            cmd.dispatch_invocations(target_size.width, target_size.height);
        },
    });
}

void RenderScene::add_info_read_pass(shared_ptr<vuk::RenderGraph> rg) {
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

void RenderScene::add_emitter_update_pass(shared_ptr<vuk::RenderGraph> rg) {
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

void RenderScene::generate_mips(shared_ptr<vuk::RenderGraph> rg, string_view input_name, string_view output_name, uint32 mip_count) {
    vector<vuk::Name> diverged_names;
    for (uint32 mip_level = 0; mip_level < mip_count; mip_level++) {
        vuk::Name div_name = {string_view(fmt_("{}_mip{}", input_name, mip_level))};
        if (mip_level != 0) {
            diverged_names.push_back(div_name.append("+"));
        } else {
            diverged_names.push_back(div_name);
        }
        rg->diverge_image(input_name, { .base_level = mip_level, .level_count = 1 }, div_name);
    }

    for (uint32 mip_level = 1; mip_level < mip_count; mip_level++) {
        vuk::Name mip_src_name = {string_view(fmt_("{}_mip{}{}", input_name, mip_level - 1, mip_level != 1 ? "+" : ""))};
        vuk::Name mip_dst_name = {string_view(fmt_("{}_mip{}", input_name, mip_level))};
        vuk::Resource src_resource(mip_src_name, vuk::Resource::Type::eImage, vuk::Access::eTransferRead);
        vuk::Resource dst_resource(mip_dst_name, vuk::Resource::Type::eImage, vuk::Access::eTransferWrite, mip_dst_name.append("+"));
        rg->add_pass({
            .name = {string_view(fmt_("{}_mip{}_gen", input_name, mip_level))},
            .execute_on = vuk::DomainFlagBits::eGraphicsOnGraphics,
            .resources = { src_resource, dst_resource },
            .execute = [mip_src_name, mip_dst_name, mip_level](vuk::CommandBuffer& command_buffer) {
                vuk::ImageBlit blit;
                auto src_ia = *command_buffer.get_resource_image_attachment(mip_src_name);
                auto dim = src_ia.extent;
                assert(dim.sizing == vuk::Sizing::eAbsolute);
                auto extent = dim.extent;
                blit.srcSubresource.aspectMask = vuk::format_to_aspect(src_ia.format);
                blit.srcSubresource.baseArrayLayer = src_ia.base_layer;
                blit.srcSubresource.layerCount = src_ia.layer_count;
                blit.srcSubresource.mipLevel = mip_level - 1;
                blit.srcOffsets[0] = vuk::Offset3D{ 0 };
                blit.srcOffsets[1] = vuk::Offset3D{
                    math::max(int32(extent.width) >> (mip_level - 1), 1),
                    math::max(int32(extent.height) >> (mip_level - 1), 1),
                    math::max(int32(extent.depth) >> (mip_level - 1), 1)
                };
                blit.dstSubresource = blit.srcSubresource;
                blit.dstSubresource.mipLevel = mip_level;
                blit.dstOffsets[0] = vuk::Offset3D{ 0 };
                blit.dstOffsets[1] = vuk::Offset3D{
                    math::max(int32(extent.width) >> (mip_level), 1),
                    math::max(int32(extent.height) >> (mip_level), 1),
                    math::max(int32(extent.depth) >> (mip_level), 1)
                };
                command_buffer.blit_image(mip_src_name, mip_dst_name, blit, vuk::Filter::eLinear);
            } });
    }

    rg->converge_image_explicit(diverged_names, output_name);
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
