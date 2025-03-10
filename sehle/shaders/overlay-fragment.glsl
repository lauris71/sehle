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
#ifndef HAS_ALPHA
#define HAS_ALPHA 0
#endif

#if HAS_TEXTURE
in vec2 interpolatedTexCoord;
uniform sampler2D tex;
#endif

#if HAS_COLORS
in vec4 interpolatedColor;
#else
uniform vec4 primary = vec4(1.0, 1.0, 1.0, 1.0);
#endif

#if HAS_MASK
uniform vec4 secondary[3];
#endif

#if HAS_EXPOSURE
uniform float lwmax, gamma;
#endif

out vec4 color_fragment;

#if HAS_EXPOSURE
vec3 exposure (vec3 color, float Lwmax);
#endif

void main()
{
#if HAS_TEXTURE
	vec4 mask = texture (tex, interpolatedTexCoord);
#else
	vec4 mask = vec4(1.0, 1.0, 1.0, 1.0);
#endif

#if HAS_COLORS
	vec4 primary = interpolatedColor;
#endif

#if HAS_MASK
	vec4 color = mask.r * primary + mask.g * secondary[0] + mask.b * secondary[1] + mask.a * secondary[2];
#else
	vec4 color = mask * primary;
#endif

#if HAS_ALPHA
	if (color.a < MIN_ALPHA) {
		discard;
	}
#endif

#if HAS_EXPOSURE
	//color_fragment = vec4(pow (color.rgb, vec3(gamma, gamma, gamma)), color.a);
	color_fragment = vec4(pow (exposure (color.rgb, lwmax), vec3(gamma, gamma, gamma)), color.a);
#else
	//color_fragment = vec4(color.r, color.r, color.r, 1.0);
	color_fragment = color;
#endif
}

