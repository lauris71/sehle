#ifndef HAS_TEXTURE
#define HAS_TEXTURE 0
#endif
#ifndef HAS_COLORS
#define HAS_COLORS 0
#endif
#ifndef HAS_MASK
#define HAS_MASK 0
#endif
#ifndef HAS_DEPTH
#define HAS_DEPTH 0
#endif
#ifndef HAS_EXPOSURE
#define HAS_EXPOSURE 0
#endif

in vec2 vertex;
uniform vec4 vertex_transform = vec4(0.0, 0.0, 1.0, 1.0);

#if HAS_TEXTURE
in vec2 texcoord;
out vec2 interpolatedTexCoord;
uniform vec4 tex_transform = vec4(0.0, 0.0, 1.0, 1.0);
#endif

#if HAS_COLORS
in vec4 color;
out vec4 interpolatedColor;
#endif

#if HAS_DEPTH
uniform float depth;
#else
#define depth 0
#endif

// Mask is either texture or white
// Primary color is either color or color[0]

void main()
{
	vec2 v = vertex_transform.xy + vertex_transform.zw * vertex;
	gl_Position = vec4(v, depth, 1.0);
#if HAS_TEXTURE
	vec2 t = tex_transform.xy + tex_transform.zw * texcoord;
	interpolatedTexCoord = t;
#endif
#if HAS_COLORS
	interpolatedColor = color;
#endif
}

