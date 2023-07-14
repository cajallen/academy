#pragma once

#include <optional>
#include <mutex>
#include <array>

#include <vuk/resources/DeviceFrameResource.hpp>
#include <vuk/Context.hpp>
#include <vuk/RenderGraph.hpp>
#include <VkBootstrap.h>

#include "extension/vuk_imgui.hpp"
#include "general/vector.hpp"
#include "general/math/geometry.hpp"
#include "renderer/gpu_asset_cache.hpp"
#include "game/frame_timer.hpp"

struct GLFWwindow;

using std::optional;
using std::array;

namespace spellbook {

#define CAMERA_BINDING 0
#define MODEL_BINDING 1
#define ID_BINDING 2
#define BONES_BINDING 3
#define MATERIAL_BINDING 4
#define BASE_COLOR_BINDING 5
#define ORM_BINDING 6
#define NORMAL_BINDING 7
#define EMISSIVE_BINDING 8
#define SPARE_BINDING_1 9
#define PARTICLES_BINDING MODEL_BINDING

struct RenderScene;

enum RenderStage { RenderStage_Inactive, RenderStage_Setup, RenderStage_BuildingRG, RenderStage_Presenting };

struct Renderer {
    constexpr static uint32 inflight_count = 3;

    RenderStage          stage = RenderStage_Setup;
    FrameTimer           frame_timer;
    vector<RenderScene*> scenes;

    v2i         window_size = {2560, 1440};
    GLFWwindow* window;
    bool        suspend = false;

    optional<vuk::Context>                  context;
    optional<vuk::DeviceSuperFrameResource> super_frame_resource;
    optional<vuk::Allocator>                global_allocator;
    optional<vuk::Allocator>                frame_allocator;
    vuk::SwapchainRef                       swapchain;
    std::vector<vuk::Future>                futures;
    std::mutex                              setup_lock;
    vuk::Compiler                           compiler;
    vuk::Unique<array<VkSemaphore, inflight_count>> present_ready;
    vuk::Unique<array<VkSemaphore, inflight_count>> render_complete;
    ImGuiData                      imgui_data;
    plf::colony<vuk::SampledImage> imgui_images;

    Renderer();
    void setup();
    void update();
    void render();
    void shutdown();
    ~Renderer();

    void add_scene(RenderScene*);
    void remove_scene(RenderScene*);
    void enqueue_setup(vuk::Future&& fupgpt);
    void wait_for_futures();
    
    void resize(v2i new_size);
    
    void debug_window(bool* p_open);

 private:
    vkb::Instance                           vkbinstance;
    vkb::Device                             vkbdevice;
    VkSurfaceKHR                            surface;
};

inline Renderer& get_renderer() {
    static Renderer renderer;
    return renderer;
}

}
