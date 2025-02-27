#ifndef DIRECTIONAL
#define DIRECTIONAL 1
#endif

// Use point attenuation and light position
#ifndef POINT
#define POINT 0
#endif

// Use spot attenuation and light direction
// Every spotlight should have point also enabled
#ifndef SPOT
#define SPOT 0
#endif

#ifndef HAS_SHADOW
#define HAS_SHADOW 0
#endif

#ifndef HAS_DENSITY
#define HAS_DENSITY 0
#endif

#ifndef NUM_SPLITS
#define NUM_SPLITS 1
#endif

// GBuffer parameters
uniform vec4 viewportinv;
uniform mat4 map_rprojection;
uniform sampler2D depthSampler, normalSampler, albedoSampler, specularSampler;

uniform vec4 light_pos;

#if SPOT || DIRECTIONAL
uniform vec3 lightdir;
#endif

uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 direct;

#if POINT
/* Min distance, radius, falloff */
uniform vec3 point_attn;
#endif

#if SPOT
/* Inner cos, outer cos, falloff */
uniform vec3 spot_attn;
#endif

#if HAS_SHADOW
uniform float splits[NUM_SPLITS];
uniform mat4 eye2shadow[NUM_SPLITS];
uniform sampler2DShadow shadowSampler;
#endif

#if HAS_DENSITY
uniform sampler2D densitySampler;
#endif

out vec4 color_fragment;

in vec4 interpolatedColor;

void decodeGBuffer (in vec4 albedoData, in vec4 normalData, in vec4 specualrData, out vec3 albedo, out vec3 normal, out vec3 specular, out float ambient, out float shininess);

#if POINT
float
point_attenuation(float distance, float radius, float falloff)
{
	if (radius <= distance) return 0.0;
    float s = distance / radius;
    return (1.0 - s * s) * (1.0 - s * s) / (1.0 + falloff * s);
}

float
point_intensity (vec3 v2l)
{
	float dist = length (v2l);
	float radius = point_attn[1];
	float falloff = point_attn[2];
	if (falloff <= 0.0) return 1.0;
	if (dist < point_attn[0]) return 0.0;
	return point_attenuation(dist, radius, falloff);
}
#endif

#if SPOT
float
spot_attenuation(float cosa, float inner, float outer, float falloff)
{
	if (cosa <= outer) return 0.0;
	if (cosa >= inner) return 1.0;
	float s = (inner - cosa) / (inner - outer);
    return (1.0 - s * s) * (1.0 - s * s) / (1.0 + falloff * s);
}

float
spot_intensity (vec3 v2l_norm, vec3 l_dir)
{
	float inner = spot_attn[0];
	float outer = spot_attn[1];
	float falloff = spot_attn[2];
	if (falloff <= 0.0) return 1.0;
	float c_angle = -dot(v2l_norm, l_dir);
	return spot_attenuation(c_angle, inner, outer, falloff);
}
#endif

vec3
ambient_light (float light_intensity, vec3 material_diffuse)
{
	return light_intensity * ambient * material_diffuse;
}

vec3
diffuse_light (float light_intensity, vec3 material_diffuse, vec3 normal, vec3 v2l_n)
{
	float diffuse_light_intensity = light_intensity * max (dot (v2l_n, normal), 0.0);
	vec3 diffuse_light_color = diffuse_light_intensity * diffuse;
	vec3 diffuse_color = diffuse_light_color * material_diffuse;
	return diffuse_color;
}

vec3
direct_light (vec3 light, vec3 material_diffuse, vec3 material_specular, float material_shininess, vec3 normal, vec3 v2e_n, vec3 v2l_n)
{
	// Diffuse
	float direct_light_intensity = max (dot (v2l_n, normal), 0.0);
	vec3 direct_light_color = direct_light_intensity * light * direct;
	vec3 direct_color = direct_light_color * material_diffuse;

	// Specular
	float specularIntensity = clamp (dot (reflect (v2l_n, normal), -v2e_n), 0.0, 1.0);
	float specularShineIntensity = pow (specularIntensity, material_shininess);
	vec3 specularLightColor = specularShineIntensity * light * direct;
	vec3 specularColor = specularLightColor * material_specular;

	return direct_color + specularColor;
}

#if HAS_SHADOW || HAS_DENSITY
vec3
shadow_density (vec3 vertexEye, vec3 normalEye)
{
	vec3 lightIntensity = vec3(1.0, 1.0, 1.0);
	for (int i = 0; i < NUM_SPLITS; i++) {
		if (vertexEye.z > splits[i]) {
			// We move shadow reciever a bit along normal to eliminate nasty pixel noise
			vec4 shadowCoordsD = eye2shadow[i] * vec4(vertexEye + 0.002 * -splits[i] * normalEye, 1.0);
			vec3 shadowCoords = shadowCoordsD.stp / shadowCoordsD.q;
#if HAS_SHADOW
			lightIntensity = lightIntensity * texture(shadowSampler, shadowCoords);
#endif
#if HAS_DENSITY
			lightIntensity = lightIntensity * texture(densitySampler, shadowCoords.st).rgb;
#endif
			break;
		}
	}
	return lightIntensity;
}
#endif

vec3
lighting (vec3 vertexEye, vec3 normalEye, vec3 materialDiffuse, vec3 materialSpecular, float materialShininess, float ambientFactor)
{
	vec3 v2e = -vertexEye;
	vec3 v2e_norm = normalize (v2e);
	vec3 v2l = light_pos.xyz - light_pos.w * vertexEye;
	vec3 v2l_norm = normalize (v2l);

#if POINT
	float source = point_intensity (v2l);
	if (source < 0.001) discard;
#else
	float source = 1.0;
#endif

#if SPOT
	source *= spot_intensity (v2l_norm, lightdir);
	if (source < 0.001) discard;
#endif

	vec3 eye2Vertex = -vertexEye;
	vec3 eye2VertexNormalized = normalize (eye2Vertex);

	// Ambient
	vec3 light = ambient_light (source * ambientFactor, materialDiffuse);

	// Diffuse
	light += diffuse_light (source, materialDiffuse, normalEye, v2l_norm);

	// Direct
	vec3 lightIntensity = vec3(source, source, source);
#if HAS_SHADOW || HAS_DENSITY
	lightIntensity *= shadow_density (vertexEye, normalEye);
#endif
	// Lighted
	light += direct_light (lightIntensity, materialDiffuse, materialSpecular, materialShininess, normalEye, eye2VertexNormalized, v2l_norm);

	return light;
}

void main()
{
	// Normalized screen / texture lookup coordinates
	vec2 tlc = gl_FragCoord.xy * viewportinv.xy + viewportinv.zw;
	float Zw =  texture(depthSampler, tlc).x;
	// Denormalized eye coordinates
	vec4 vertexEyeDN = map_rprojection * vec4(tlc, Zw, 1.0);
	vec4 vertexEye = vertexEyeDN / vertexEyeDN.w;

	vec4 albedoData = texture (albedoSampler, tlc);
	vec4 normalData = texture (normalSampler, tlc);
	vec4 specularData = texture (specularSampler, tlc);

	vec3 albedo, normal, specular;
	float ambientFactor, shininess;
	decodeGBuffer (albedoData, normalData, specularData, albedo, normal, specular, ambientFactor, shininess);

	vec3 color = lighting (vertexEye.xyz, normal, albedo, specular, shininess, ambientFactor);

	//color_fragment = vec4(1.0, 1.0, 0.0, 1.0);
	color_fragment = vec4(color, 1.0);
}
