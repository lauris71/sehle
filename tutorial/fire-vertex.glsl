#version 140

in vec3 vertex;
in vec3 normal;

out vec3 vertex_w;
out vec3 normal_w;

uniform mat4 o2v_projection;
uniform mat4x3 o2w;

void main() {
    gl_Position = o2v_projection * vec4(vertex, 1.0);
    vertex_w = o2w * vec4(vertex, 1.0);
    normal_w = (o2v_projection * vec4(normal, 0.0)).xyz;
}
