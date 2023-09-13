#version 450
#pragma shader_stage(fragment)

#include "include.glsli"

layout (location = 0) in VS_OUT {
    vec3 position;
    vec3 color;
    vec2 uv;
} fin;

layout(binding = MATERIAL_BINDING) uniform MaterialData {
	vec4 base_color_tint; 
	vec4 emissive_tint;
    vec4 roughness_metallic_normals_scale;
};
layout(binding = BASE_COLOR_BINDING) uniform sampler2D s_base_color; 
layout(binding = ORM_BINDING) uniform sampler2D s_metallic_roughness; 
layout(binding = NORMAL_BINDING) uniform sampler2D s_normal; 
layout(binding = EMISSIVE_BINDING) uniform sampler2D s_emissive;
layout(binding = 8, rgba16f) uniform writeonly image3D u_target;

layout (location = 0) out vec4 fout_color;

layout(push_constant) uniform uPushConstant {
    ivec4 resolution;
    uint pass;
} pc;

vec2 calculate_uv() {
    return fin.uv * roughness_metallic_normals_scale.w;
}

ivec3 get_write_coord() {
    ivec3 coord = ivec3(
        floor(gl_FragCoord.x),
        floor(gl_FragCoord.y),
        floor(gl_FragCoord.z * pc.resolution.z)
    );

    if (pc.pass == 1)
        coord = ivec3(coord.z, coord.y, pc.resolution.x-coord.x-1);
    if (pc.pass == 2)
        coord = ivec3(pc.resolution.x-coord.x-1, pc.resolution.z-coord.z-1, pc.resolution.y-coord.y-1);

    return coord;
}

void main() {
    vec2 uv = calculate_uv();

    ivec3 coord = get_write_coord();
    if (all(greaterThan(coord, ivec3(-1))) && all(lessThan(coord, pc.resolution.xyz)))
        imageStore(u_target, coord, texture(s_emissive, uv) * emissive_tint);

    fout_color = vec4(1.0);
}