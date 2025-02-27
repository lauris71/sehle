
in vec3 vertex;

in vec4 color;
out vec4 interpolatedColor;

uniform mat4 l2w_w2v_projection;

void main ()
{
	vec4 vClip4 = l2w_w2v_projection * vec4(vertex, 1.0);
	// Keep light volume inside view frustum
	vClip4.z = min (vClip4.z, vClip4.w);
	interpolatedColor = color;
	gl_Position = vClip4;
}
