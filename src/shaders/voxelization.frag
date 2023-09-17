#version 450
#pragma shader_stage(fragment)

#include "include.glsli"

layout (location = 0) in VS_OUT {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
} fin;

layout(binding = MATERIAL_BINDING) uniform MaterialData {
	vec4 base_color_tint; 
	vec4 emissive_tint;
    vec4 roughness_metallic_normals_scale;
};

layout (binding = ID_BINDING) uniform CameraData {
	mat4 light_vp;
    vec4 light_normal;
};

layout(binding = BASE_COLOR_BINDING) uniform sampler2D s_base_color; 
layout(binding = ORM_BINDING) uniform sampler2D s_metallic_roughness; 
layout(binding = NORMAL_BINDING) uniform sampler2D s_normal;
layout(binding = EMISSIVE_BINDING) uniform sampler2D s_emissive;
layout(binding = 9) uniform sampler2D s_sun_depth;
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
float shaded() {
    vec4 h_position_lightspace = light_vp * vec4(fin.position, 1.0);
    vec3 position_lightspace = h_position_lightspace.xyz / h_position_lightspace.w;
    vec2 uv = position_lightspace.xy * 0.5 + vec2(0.5);

    const float maxBias = 0.02;
    float b = length(1.0 / textureSize(s_sun_depth, 0));
    float bias = maxBias;

    if (uv.x < 0.0 || uv.x >= 1.0 || uv.y < 0.0 || uv.y >= 1.0 || position_lightspace.z < 0.01)
    return 1.0;

    float world_read_depth = texture(s_sun_depth, uv).r;
    bias = min(bias, maxBias) + 0.001f * (1.0f - world_read_depth);
    float world_position_depth = position_lightspace.z + bias;

    float shaded_amount = world_position_depth < world_read_depth ? 0.0 : 1.0;
    return shaded_amount;
}

void main() {
    vec2 uv = calculate_uv();

    ivec3 coord = get_write_coord();
    if (all(greaterThan(coord, ivec3(-1))) && all(lessThan(coord, pc.resolution.xyz)))
        imageStore(u_target, coord, shaded() * abs(dot(fin.normal, light_normal.xyz)) * texture(s_base_color, uv) * base_color_tint + texture(s_emissive, uv) * emissive_tint);

    fout_color = vec4(1.0);
}