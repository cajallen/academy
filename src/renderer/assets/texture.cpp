#include "texture.hpp"

#include <vuk/Partials.hpp>

#include "general/logger.hpp"
#include "renderer/renderer.hpp"
#include "renderer/gpu_asset_cache.hpp"

namespace spellbook {

string upload_texture(const TextureCPU& tex_cpu, bool frame_allocation) {
    assert_else(!tex_cpu.file_path.empty());
    uint64 tex_cpu_hash = hash_view(tex_cpu.file_path);
    vuk::Allocator& alloc = frame_allocation ? *get_renderer().frame_allocator : *get_renderer().global_allocator;
    auto [tex, tex_fut] = vuk::create_texture(alloc, tex_cpu.format, vuk::Extent3D(tex_cpu.size), (void*) tex_cpu.pixels.data(), true);
    get_renderer().context->set_name(tex, vuk::Name(tex_cpu.file_path));
    get_renderer().enqueue_setup(std::move(tex_fut));
    
    get_gpu_asset_cache().textures[tex_cpu_hash] = {std::move(tex), frame_allocation};
    return tex_cpu.file_path;
}

}
