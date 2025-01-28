#define __SEHLE_SKY_RENDERER_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2011-2021
 */

#include <stdio.h>

#include <az/class.h>
#include <elea/geometry.h>

#include <GL/glew.h>

#include "engine.h"
#include "program.h"
#include "texture-cubemap.h"
#include <sehle/program.h>
#include <sehle/render-context.h>
#include <sehle/renderable.h>
#include <sehle/shader.h>
#include <sehle/vertex-array.h>
#include <sehle/vertex-buffer.h>
#include <sehle/index-buffer.h>

#include <sehle/sky-renderer.h>

enum SehleSkyUniforms {
	SKY_SUN_DIRECTION = SEHLE_NUM_UNIFORMS,
	SKY_VP2T, SKY_MAP_RPROJECTION, SKY_DEPTH_SAMPLER,
	SKY_K_R, SKY_K_M, SKY_G_M, SKY_G_M2,
	SKY_SUN_RADIOSITY, SKY_SAMPLER, SKY_BRIGHTNESS,
	SKY_C_A, SKY_C_B,
	SKY_W2S,
	SEHLE_SKY_NUM_UNIFORMS
};

static const char *uniform_names[] = {
	"sun_direction",
	"vp2t", "map_rprojection", "depthSampler",
	"k_r", "k_m", "g_m", "g_m2", "sun_radiosity", "sky_sampler", "sky_brightness",
	"c_a", "c_b",
	"w2s"
};

static SehleShader *
sky_get_shader (SehleEngine *engine, unsigned int shader_type)
{
	char c[256];
	sprintf (c, "Sehle::Sky::%s",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment");
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const char *sources[1];
		sprintf (c, "#version 140\n#define %s\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS");
		sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "sky-vertex.txt" : "sky-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

SehleProgram *
sehle_program_sky_get_reference (SehleEngine *engine)
{
	char c[256];
	sprintf (c, "Sehle::Sky");
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 1, SEHLE_SKY_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, sky_get_shader (engine, SEHLE_SHADER_VERTEX));
		sehle_program_add_shader (prog, sky_get_shader (engine, SEHLE_SHADER_FRAGMENT));
		sehle_program_set_uniform_names (prog, 0, SEHLE_NUM_UNIFORMS, ( const unsigned char **) sehle_uniforms);
		sehle_program_set_uniform_names (prog, SEHLE_NUM_UNIFORMS, SEHLE_SKY_NUM_UNIFORMS - SEHLE_NUM_UNIFORMS, (const unsigned char **) uniform_names);
	}
	return prog;
}

static void sky_class_init (SehleSkyRendererClass *klass);
static void sky_init (SehleSkyRendererClass *klass, SehleSkyRenderer *sky);
/* SehleRenderable implementation */
static void sky_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx);
static void sky_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data);
/* SehleMaterial implementation */
static void sky_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);

unsigned int sehle_sky_renderer_type = 0;
SehleSkyRendererClass *sehle_sky_renderer_class;

unsigned int
sehle_sky_renderer_get_type (void)
{
	if (!sehle_sky_renderer_type) {
		az_register_type (&sehle_sky_renderer_type, (const unsigned char *) "SehleSkyRenderer", AZ_TYPE_BLOCK, sizeof (SehleSkyRendererClass), sizeof (SehleSkyRenderer), AZ_CLASS_ZERO_MEMORY,
			(void (*) (AZClass *)) sky_class_init,
			(void (*) (const AZImplementation *, void *)) sky_init,
			NULL);
		sehle_sky_renderer_class = (SehleSkyRendererClass *) az_type_get_class (sehle_sky_renderer_type);
	}
	return sehle_sky_renderer_type;
}

static void
sky_class_init (SehleSkyRendererClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 2);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_RENDERABLE,
		ARIKKEI_OFFSET(SehleSkyRendererClass, renderable_impl),
		ARIKKEI_OFFSET(SehleSkyRenderer, renderable_inst));
	az_class_declare_interface ((AZClass *) klass, 1, SEHLE_TYPE_MATERIAL,
		ARIKKEI_OFFSET (SehleSkyRendererClass, material_impl),
		ARIKKEI_OFFSET (SehleSkyRenderer, material_inst));
	klass->renderable_impl.display = sky_display;
	klass->renderable_impl.render = sky_render;
	klass->material_impl.bind = sky_bind;
}

static void
sky_init (SehleSkyRendererClass *klass, SehleSkyRenderer *sky)
{
	sky->renderable_inst.render_stages = SEHLE_STAGE_TRANSPARENT;
	sky->renderable_inst.bbox = EleaAABox3fInfinite;
	sehle_render_flags_clear (&sky->material_inst.state_flags, SEHLE_CULL | SEHLE_DEPTH_WRITE | SEHLE_DEPTH_TEST);
	sky->material_inst.render_stages = SEHLE_STAGE_TRANSPARENT;
	sky->material_inst.render_types = SEHLE_RENDER_TRANSPARENT;

	sky->sun_dir = EleaVec3fZ;
	elea_vec3fp_set_xyz (&sky->Kr, 1e-8f, 1e-8f, 1e-8f);
	elea_vec3fp_set_xyz (&sky->Km, 1e-8f, 1e-8f, 1e-8f);
	sky->gm = -0.9f;
	elea_vec3fp_set_xyz (&sky->Rs, 10, 10, 10);
	sky->Ca = 0.5f;
	sky->Cb = 1;
}

static void
sky_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx)
{
	SehleSkyRenderer *sky = SEHLE_SKY_RENDERER_FROM_RENDERABLE_INSTANCE (inst);
	sehle_render_context_schedule_render (ctx, impl, inst, SEHLE_SKY_RENDERER_MATERIAL_IMPLEMENTATION, &sky->material_inst, NULL);
}

static void
sky_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data)
{
	SehleSkyRenderer *sky = SEHLE_SKY_RENDERER_FROM_RENDERABLE_INSTANCE (inst);

	sehle_vertex_array_render_triangles (sky->va, 1, 0, sky->va->ibuf->buffer.n_elements);
}

static void
sky_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleSkyRenderer *sky = SEHLE_SKY_RENDERER_FROM_MATERIAL_INSTANCE (inst);
	SehleProgram *prog = inst->programs[0];
	sehle_render_context_set_program (ctx, prog);
	sehle_render_context_set_gbuffer_uniforms (ctx, prog, SKY_VP2T, SKY_MAP_RPROJECTION);

	EleaMat4x4f o2v_proj;
	elea_mat4x4f_multiply_mat3x4 (&o2v_proj, &ctx->proj, &ctx->w2v);
	sehle_program_setUniformMatrix4fv (prog, SEHLE_UNIFORM_O2V_PROJECTION, 1, o2v_proj.c);

	// Depth sampler
	if (ctx->depthchannel >= 0) {
		sehle_program_setUniform1i (prog, SKY_DEPTH_SAMPLER, ctx->depthchannel);
	}

	sehle_material_instance_bind_texture (inst, ctx, 0, 0, SKY_SAMPLER);
	sehle_program_setUniform1f (prog, SKY_BRIGHTNESS, sky->star_brightness);
	sehle_program_setUniform3fv (prog, SKY_SUN_DIRECTION, 1, sky->sun_dir.c);
	sehle_program_setUniform3fv (prog, SKY_K_R, 1, sky->Kr.c);
	sehle_program_setUniform3fv (prog, SKY_K_M, 1, sky->Km.c);
	sehle_program_setUniform1f (prog, SKY_G_M, sky->gm);
	sehle_program_setUniform1f (prog, SKY_G_M2, sky->gm * sky->gm);
	sehle_program_setUniform3fv (prog, SKY_SUN_RADIOSITY, 1, sky->Rs.c);
	sehle_program_setUniform1f (prog, SKY_C_A, sky->Ca);
	sehle_program_setUniform1f (prog, SKY_C_B, sky->Cb);
	float w2s[9];
	for (int c = 0; c < 3; c++) {
		for (int r = 0; r < 3; r++) {
			w2s[3 * c + r] = sky->w2s.c[4 * r + c];
		}
	}
	sehle_program_setUniformMatrix3fv (inst->programs[0], SKY_W2S, 1, w2s);
}

#define RADIUS 0.5f
#define NCOORDS 4

void
sehle_sky_renderer_setup (SehleSkyRenderer *sky, SehleEngine *engine)
{
	unsigned int nvsphere;
	unsigned int nisphere;
	elea_generate_sphere (NULL, 0, NULL, 0, NULL, 0, NULL, RADIUS, 4, 1, &nvsphere, &nisphere);
	sky->va = sehle_vertex_array_new_from_attrs (engine, NULL, nvsphere, nisphere, SEHLE_ATTRIBUTE_VERTEX, 4, -1);
	float *attributes = sehle_vertex_buffer_map (sky->va->vbufs[SEHLE_ATTRIBUTE_VERTEX], SEHLE_BUFFER_WRITE);
	uint32_t *indices = sehle_index_buffer_map (sky->va->ibuf, SEHLE_BUFFER_WRITE);
	elea_generate_sphere (attributes, NCOORDS * 4, NULL, 0, NULL, 0, indices, RADIUS, 4, -1, NULL, NULL);
	/* Flatten in Z direction */
	for (unsigned int i = 0; i < nvsphere; i++) {
		attributes[i * NCOORDS + ELEA_Z] *= 0.1f;
	}
	sehle_vertex_buffer_unmap (sky->va->vbufs[SEHLE_ATTRIBUTE_VERTEX]);
	sehle_index_buffer_unmap (sky->va->ibuf);

	sehle_material_setup (&sky->material_inst, 1, 1);
	sky->material_inst.programs[0] = sehle_program_sky_get_reference (engine);
}

void
sehle_sky_renderer_release (SehleSkyRenderer *sky)
{
	az_object_unref ((AZObject *) sky->va);
	sehle_material_release (&sky->material_inst);
}

void
sehle_sky_renderer_set_star_texture (SehleSkyRenderer *sky, SehleTextureCubeMap *stars)
{
	sehle_material_set_texture (&sky->material_inst, 0, (SehleTexture *) stars);
}

void
sehle_sky_renderer_calculate_sky_position (unsigned int day_of_year, float time_of_day, float latitude, float *sun_azimuth, float *sun_altitude, float *sidereal_time)
{
	// Calculate the coordinates of Sun
	// http://www.astro.uio.no/~bgranslo/aares/calculate.html
	// Number of days since 01/01/2000
	// d0 = 367Y - INT{(7/4)[Y+INT((M+9)/12)]} + INT(275M/9) + D -730531.5  [1]
	float d0 = (float) day_of_year;
	// Number of Julian centuries since 01/01/2000
	float T0 = d0 / 36525;
	// Sidereal time at Greenwich at 00/00 midnight
	float S0_hours = 6.6974f + 2400.0513f * T0;
	S0_hours = fmodf (S0_hours, 24);
	// Sidereal time at Greenwich at given universal time
	float SG_hours = S0_hours + (366.2422f / 365.2422f) * (time_of_day / 3600);
	SG_hours = fmodf (SG_hours, 24);
	// Sidereal time at longitude
	float S_hours = SG_hours + 0;
	float S = S_hours * 360 / 24;
	// Number of days since 01/01/2000
	float d = d0 + time_of_day / 86400;
	// Number of centuries from 01/01/2000
	float T = d / 36525;
	// Sun longitude in degrees
	float L0 = 280.466f + 36000.770f * T;
	L0 = fmodf (L0, 360);
	// Sun anomaly in degrees
	float M0 = 357.529f + 35999.050f * T;
	// Sun center
	float C = (1.915f - 0.005f * T) * sinf (M0 * ELEA_M_PI_F / 180) + 0.020f * sinf (2 * M0 * ELEA_M_PI_F / 180);
	// True ecliptical longitude of Sun
	float LS = L0 + C;
	LS = fmodf (LS, 360);
	// The obliquity of ecliptic
	float K = 23.439f - 0.013f * T;
	// The Sun right ascension
	float tanRS = tanf (LS * ELEA_M_PI_F / 180) * cosf (K * ELEA_M_PI_F / 180);
	float RS = atanf (tanRS) * 180 / ELEA_M_PI_F;
	if ((LS > 90) && (LS < 270)) RS += 180;
	// Declination
	float sinDS = sinf (RS * ELEA_M_PI_F / 180) * sinf (K * ELEA_M_PI_F / 180);
	float DS = asinf (sinDS) * 180 / ELEA_M_PI_F;
	// Hour angle
	float H = S - RS;
	// fprintf (stderr, "S %g   RS %g    H %g\n", S, RS, H);
	// True altitude
	float sinh = sinf (latitude) * sinf (DS * ELEA_M_PI_F / 180) + cosf (latitude) * cosf (DS * ELEA_M_PI_F / 180) * cosf (H * ELEA_M_PI_F / 180);
	float h = asinf (sinh);
	// Azimuth (eastward from north)
	float y = -sinf (H * ELEA_M_PI_F / 180);
	float x = tanf (DS * ELEA_M_PI_F / 180) * cosf (latitude) - sinf (latitude) * cosf (H * ELEA_M_PI_F / 180);
	float A = -atan2f (y, x);
	// Air mass
	// float X = (sinh > 0) ? 1 / (sinh + 0.025f * exp (-11 * sinh)) : 40;

	if (sun_azimuth) *sun_azimuth = A;
	if (sun_altitude) *sun_altitude = h;
	if (sidereal_time) *sidereal_time = SG_hours;
}

float
sehle_sky_renderer_ray_length_atmosphere (const EleaVec3f *p, const EleaVec3f *d)
{
	// d2 * t2 + 2 * (P - C) * d * t + (P - C)2 - R2 = 0;
	// lengthVertical = 8km
	// lengthHorizontal = 309.941km
	float r = 6000000.0f;
	float R = 6008000.0f;
	EleaVec3f C;
	elea_vec3fp_set_xyz (&C, 0, 0, -r);
	EleaVec3f p_C = elea_vec3f_sub (*p, C);
	float b = 2 * elea_vec3f_dot (p_C, *d);
	float c = elea_vec3f_dot (p_C, p_C) - R * R;
	return (-b + sqrtf (b * b - 4 * c)) / 2;
}

float
sehle_sky_renderer_t (float l, float K)
{
	float f = 4 * ELEA_M_PI_F / 2.718281828f;
	return f * K * l;
}

float
sehle_sky_renderer_Fr (float cosTheta)
{
	return 0.75f * (1 + cosTheta * cosTheta);
}

float
sehle_sky_renderer_Fm (float cosTheta, float g)
{
	float a = 3 * (1 - g * g);
	float b = 2 * (2 + g * g);
	float c = 1 + cosTheta * cosTheta;
	float d = 1 + g * g - 2 * g * cosTheta;
	d = sqrtf (d * d * d);
	return a * c / (b * d);
}

void
sehle_sky_renderer_atmospheric_attenuation (float dst_intensity[], const float src_intensity[], const float Kr[], const float Km[], float ray_length)
{
	for (int i = 0; i < 3; i++) {
		dst_intensity[i] = src_intensity[i] * expf (-sehle_sky_renderer_t (ray_length, Kr[i] + Km[i]));
	}
}

static EleaColor4f
sehle_sky_renderer_sky_luminosity (const EleaVec3f *view_dir, const EleaVec3f *sun_dir, const EleaVec3f *K, const EleaColor4f *Rsun)
{
	EleaColor4f isLight = EleaColor4fBlack;
	float lenAB = sehle_sky_renderer_ray_length_atmosphere (&EleaVec3f0, view_dir);
	float dl = lenAB / 8;
	for (int i = 0; i < 8; i++) {
		float s = (i + 0.5f) / 8;
		float lenAP = s * lenAB;
		EleaVec3f P = elea_vec3f_mul (*view_dir, lenAP);
		float lenPC = sehle_sky_renderer_ray_length_atmosphere (&P, sun_dir);
		for (int j = 0; j < 3; j++) {
			// Light arriving at P
			float lightP = expf (-sehle_sky_renderer_t (lenPC, K->c[j]));
			// Light scattered towards A
			float lightPscatter = lightP;
			// Light arriving at A
			isLight.c[j] += lightPscatter * expf (-sehle_sky_renderer_t (lenAP, K->c[j]));
		}
	}
	for (int i = 0; i < 3; i++) isLight.c[i] = isLight.c[i] * Rsun->c[i] * dl;
	return isLight;
}

void
sehle_sky_renderer_get_sky_color (EleaColor4f *dst, const EleaVec3f *view_dir, const EleaVec3f *sun_dir, const EleaVec3f *Kr, const EleaVec3f *Km, float gm, const EleaColor4f *Rsun)
{
	float cosTheta = -elea_vec3fp_dot (view_dir, sun_dir);
	EleaVec3f KrKm = elea_vec3f_add (*Kr, *Km);
	*dst = sehle_sky_renderer_sky_luminosity (view_dir, sun_dir, &KrKm, Rsun);
	for (int j = 0; j < 3; j++) {
		dst->c[j] *= (Kr->c[j] * sehle_sky_renderer_Fr (cosTheta) + Km->c[j] * sehle_sky_renderer_Fm (cosTheta, gm));
	}
}

