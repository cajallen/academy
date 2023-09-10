find_package(Vulkan REQUIRED)

add_library(renderer
    assets/material.cpp
    assets/mesh.cpp
    assets/model.cpp
    assets/particles.cpp
    assets/texture.cpp
    camera.cpp
    draw_functions.cpp
    light.cpp
    render_scene.cpp
    renderable.cpp
    renderer.cpp
    samplers.cpp
    vertex.cpp
    viewport.cpp
    gpu_asset_cache.cpp
    frame_timer.cpp
)

target_include_directories(renderer PRIVATE ..)
target_link_libraries(renderer PUBLIC archive libs)
target_precompile_headers(renderer PUBLIC ../archive/general/global.hpp)
