#ifndef HAS_COLORS
#define HAS_COLORS 0
#endif
#ifndef HAS_DIFFUSE_TEXTURE
#define HAS_DIFFUSE_TEXTURE 0
#endif
#ifndef HAS_NORMAL_TEXTURE
#define HAS_NORMAL_TEXTURE 0
#endif
#ifndef HAS_SPECULAR_TEXTURE
#define HAS_SPECULAR_TEXTURE 0
#endif
#ifndef HAS_AMBIENT_TEXTURE
#define HAS_AMBIENT_TEXTURE 0
#endif

#define HAS_TEXTURE (HAS_DIFFUSE_TEXTURE || HAS_NORMAL_TEXTURE || HAS_SPECULAR_TEXTURE || HAS_AMBIENT_TEXTURE)

uniform vec4 diffuse;
uniform float shininess;

in vec3 interpolatedVertexEye;
in vec3 interpolatedNormalEye;

#if HAS_COLORS
in vec4 interpolatedColor;
#endif

#if HAS_TEXTURE
in vec2 interpolatedTexCoord;
#endif

#if HAS_DIFFUSE_TEXTURE
uniform sampler2D colorSampler;
#endif

#if HAS_NORMAL_TEXTURE
uniform sampler2D normalSampler;
#endif

#if HAS_SPECULAR_TEXTURE
uniform sampler2D specularSampler;
#else
uniform vec4 specular;
#endif

#if HAS_AMBIENT_TEXTURE
uniform sampler2D ambientSampler;
out vec2 interpolatedTexCoordAmbient;
#else
uniform float ambient;
#endif

#if HAS_NORMAL_TEXTURE
mat3 findTangentSpace (vec3 vEye, vec3 nEyeNormalized, vec2 tCoord);
#endif

#ifdef FORWARD
#define NUM_LIGHTS 4
uniform vec3 global_ambient;
uniform vec3 light_ambient[NUM_LIGHTS];
uniform vec3 light_diffuse[NUM_LIGHTS];
uniform vec3 light_direct[NUM_LIGHTS];
/* W = 0 for directional light */
uniform vec4 light_pos[NUM_LIGHTS];
/* Negative Z of light matrix */
uniform vec3 light_dir[NUM_LIGHTS];
/* Radius, falloff */
uniform vec2 point_attn[NUM_LIGHTS];
/* Inner cos, outer cos, falloff */
uniform vec3 spot_attn[NUM_LIGHTS];
#endif

/*
 * Output
 */

#ifdef GBUFFER
void encodeGBuffer (vec3 normal, vec3 diffuse, float ambientfactor, vec3 specular, float shininess);
#endif
#ifdef FORWARD
out vec4 color_fragment;
void lighting (vec3 normal, vec4 materialDiffuse, float ambientfactor, vec4 specular, float shininess);
#endif
#ifdef DENSITY
out vec4 color_fragment;
#endif

void clip (vec3 Ce, vec4 plane)
{
	float pos = dot (Ce, plane.xyz) + plane.w;
	if (pos < 0.0) {
		discard;
	}
}

uniform vec4 clip_plane;

void main()
{
	// Manual clipping for reflections
	clip (interpolatedVertexEye, clip_plane);

	// Diffuse
	vec4 materialDiffuse = diffuse;
#if HAS_COLORS
	materialDiffuse *= interpolatedColor;
#endif
#if HAS_DIFFUSE_TEXTURE
	materialDiffuse *= texture (colorSampler, interpolatedTexCoord);
#endif
	if (materialDiffuse.a < 0.03125) {
		discard;
	}

	// Specular
#if HAS_SPECULAR_TEXTURE
	vec4 specular = texture (specularSampler, interpolatedTexCoord);
#endif

	// Normal
	vec3 normal = normalize (interpolatedNormalEye);
#if HAS_NORMAL_TEXTURE
	vec3 n = normalize (texture (normalSampler, interpolatedTexCoord).xyz * 2.0 - 1.0);
	mat3 tSpace = findTangentSpace (interpolatedVertexEye, normal, interpolatedTexCoord);
	normal = tSpace * n;
#endif

#if HAS_AMBIENT_TEXTURE
	float ambient = texture(ambientSampler, interpolatedTexCoordAmbient).r;
#endif

#ifdef GBUFFER
	encodeGBuffer (normal, materialDiffuse.rgb, ambient, specular.rgb, shininess);
#endif
#ifdef FORWARD
	lighting (normal, materialDiffuse, ambient, specular, shininess);
#endif
#ifdef DENSITY
	color_fragment = materialDiffuse;
#endif
}

#ifdef FORWARD
float
diffuse_intensity(vec3 normal, vec3 light_dir) {
	return max(dot(normal, light_dir), 0.0);
}

float
point_attenuation(float distance, float radius, float falloff)
{
	if (radius <= distance) return 0.0;
    float s = distance / radius;
    return (1.0 - s * s) * (1.0 - s * s) / (1.0 + falloff * s);
}

float
point_intensity (int light, vec3 v2l)
{
	float dist = length (v2l);
	float radius = point_attn[light][0];
	float falloff = point_attn[light][1];
	if (falloff <= 0.0) return 1.0;
	return point_attenuation(dist, radius, falloff);
}

float
spot_attenuation(float cosa, float inner, float outer, float falloff)
{
	if (cosa <= outer) return 0.0;
	if (cosa >= inner) return 1.0;
	float s = (inner - cosa) / (inner - outer);
    return (1.0 - s * s) * (1.0 - s * s) / (1.0 + falloff * s);
}

float
spot_intensity (int light, vec3 v2l_norm)
{
	float inner = spot_attn[light][0];
	float outer = spot_attn[light][1];
	float falloff = spot_attn[light][2];
	if (falloff <= 0.0) return 1.0;
	float c_angle = -dot(v2l_norm, light_dir[light]);
	return spot_attenuation(c_angle, inner, outer, falloff);
}

void
lighting (vec3 normal, vec4 materialDiffuse, float ambientfactor, vec4 specular, float shininess)
{
	vec3 v2e = -interpolatedVertexEye;
	vec3 v2e_norm = normalize (v2e);

	vec3 diffuse_light = global_ambient;
	vec3 specular_light = vec3(0, 0, 0);
	for (int light = 0; light < 3; light++) {
		vec3 v2l = light_pos[light].xyz - light_pos[light].w * interpolatedVertexEye;
		vec3 v2l_norm = normalize (v2l);

		float intensity = point_intensity (light, v2l);
		intensity *= spot_intensity (light, v2l_norm);

		diffuse_light += intensity * diffuse_intensity(normal, v2l_norm) * light_diffuse[light];
		diffuse_light += intensity * light_ambient[light];

		vec3 halfv = normalize(v2e_norm + v2l_norm);
		float spec_intensity = dot(halfv, normal);
		spec_intensity = pow(spec_intensity, 16);
		specular_light += spec_intensity * light_direct[light];
	}
	vec4 color = vec4(materialDiffuse.rgb * diffuse_light + specular.rgb * specular_light, materialDiffuse.a);
	//vec4 color = vec4(specular_light, materialDiffuse.a);
	color_fragment = color;
}
#endif
