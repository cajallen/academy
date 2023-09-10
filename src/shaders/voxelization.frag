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

vec2 calculate_uv() {
    return fin.uv * roughness_metallic_normals_scale.w;
}

ivec3 get_write_coord() {
    return ivec3(
        round(gl_FragCoord.x),
        round(gl_FragCoord.y),
        round(gl_FragCoord.z * 64)
    );
}

void main() {
    vec2 uv = calculate_uv();

    ivec3 coord = get_write_coord();
    if (all(greaterThan(coord, ivec3(-1))) && all(lessThan(coord, ivec3(64))))
        imageStore(u_target, get_write_coord(), texture(s_base_color, uv) * base_color_tint);
}