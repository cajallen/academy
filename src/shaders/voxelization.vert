#version 460
#pragma shader_stage(vertex)

#include "include.glsli"

layout (location = 0) in vec3 vin_position;
layout (location = 1) in vec3 vin_normal;
layout (location = 2) in vec3 vin_tangent;
layout (location = 3) in vec3 vin_color;
layout (location = 4) in vec2 vin_uv;

layout (binding = CAMERA_BINDING) uniform CameraData {
	mat4 vp[3];
};

layout (binding = MODEL_BINDING) buffer readonly Model {
	mat4 model[];
};

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out VS_OUT {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
} vout;

layout(push_constant) uniform uPushConstant {
    ivec4 resolution;
    uint pass;
} pc;

void main() {
    mat3 N = transpose(inverse(mat3(model[gl_InstanceIndex])));

    vec4 h_position = model[gl_InstanceIndex] * vec4(vin_position, 1.0);
	vout.position = h_position.xyz / h_position.w;
    vout.normal = normalize(N * vin_normal);
	vout.color = vin_color;
    vout.uv = vin_uv;
    gl_Position = vp[pc.pass] * h_position;
}
