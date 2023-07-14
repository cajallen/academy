find_package(Vulkan REQUIRED)

add_library(renderer
    assets/asset_loader.cpp
    assets/material.cpp
    assets/mesh_asset.cpp
    assets/mesh.cpp
    assets/model.cpp
    assets/particles.cpp
    assets/skeleton.cpp
    assets/texture_asset.cpp
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
)

target_link_libraries(renderer PUBLIC archive academy_libs academy_src)
target_precompile_headers(renderer PUBLIC ../archive/general/global.hpp)
target_compile_definitions(renderer PUBLIC IMGUI_USER_CONFIG=\"imgui_config.h\")
