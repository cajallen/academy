#version 460
#pragma shader_stage(vertex)

#include "include.glsli"

layout (location = 0) in vec3 vin_position;
layout (location = 1) in vec3 vin_normal;
layout (location = 2) in vec3 vin_tangent;
layout (location = 3) in vec3 vin_color;
layout (location = 4) in vec2 vin_uv;

layout (binding = CAMERA_BINDING) uniform CameraData {
	mat4 vp;
};

layout (binding = MODEL_BINDING) buffer readonly Model {
	mat4 model[];
};

layout (binding = ID_BINDING) buffer readonly SelectionIds {
	int selection_id[];
};

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out VS_OUT {
    vec3 position;
    vec3 color;
    vec2 uv;
    mat3 TBN;
	flat uint id;
} vout;


void main() {
    vec4 h_position = model[gl_InstanceIndex] * vec4(vin_position, 1.0);
	vout.position = h_position.xyz / h_position.w;
	
    mat3 N = transpose(inverse(mat3(model[gl_InstanceIndex])));
	vec3 n = normalize(N * vin_normal);
	vec3 t = normalize(N * vin_tangent);
	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);
	vout.TBN      = mat3(t, b, n);
    
	vout.uv = vin_uv;
	vout.color = vin_color;
	vout.id = selection_id[gl_InstanceIndex];
    gl_Position = vp * h_position;
}
