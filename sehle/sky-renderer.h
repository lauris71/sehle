#ifndef __SEHLE_SKY_RENDERER_H__
#define __SEHLE_SKY_RENDERER_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2011-2021
 */

typedef struct _SehleSkyRenderer SehleSkyRenderer;
typedef struct _SehleSkyRendererClass SehleSkyRendererClass;

#define SEHLE_TYPE_SKY_RENDERER sehle_sky_renderer_get_type ()

#define SEHLE_SKY_RENDERER_FROM_RENDERABLE_INSTANCE(i) (SehleSkyRenderer *) ARIKKEI_BASE_ADDRESS (SehleSkyRenderer, renderable_inst, i)
#define SEHLE_SKY_RENDERER_RENDERABLE_IMPLEMENTATION (&sehle_sky_renderer_class->renderable_impl)
#define SEHLE_SKY_RENDERER_FROM_MATERIAL_INSTANCE(i) (SehleSkyRenderer *) ARIKKEI_BASE_ADDRESS (SehleSkyRenderer, material_inst, i)
#define SEHLE_SKY_RENDERER_MATERIAL_IMPLEMENTATION (&sehle_sky_renderer_class->material_impl)

#include <elea/vector3.h>
#include <elea/matrix3x4.h>

#include <sehle/material.h>
#include <sehle/renderable.h>

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_sky_get_reference (SehleEngine *engine);

struct _SehleSkyRenderer {
	SehleRenderableInstance renderable_inst;
	SehleMaterialInstance material_inst;
	SehleVertexArray *va;
	EleaVec3f sun_dir;
	/* Rayleigh scattering constant */
	EleaVec3f Kr;
	/* Mie scattering constant */
	EleaVec3f Km;
	// Mie symmetry constant
	float gm;
	/* Sun radiosity */
	EleaVec3f Rs;
	/* Intensity mapping coefficients */
	float Ca, Cb;
	/* Starfield transformation */
	EleaMat3x4f w2s;
	float star_brightness;
};

struct _SehleSkyRendererClass {
	AZClass klass;
	SehleRenderableImplementation renderable_impl;
	SehleMaterialImplementation material_impl;
};

#ifndef __SEHLE_SKY_RENDERER_C__
extern unsigned int sehle_sky_renderer_type;
extern SehleSkyRendererClass *sehle_sky_renderer_class;
#endif

unsigned int sehle_sky_renderer_get_type (void);

void sehle_sky_renderer_setup (SehleSkyRenderer *sky, SehleEngine *engine);
void sehle_sky_renderer_release (SehleSkyRenderer *sky);

void sehle_sky_renderer_set_star_texture (SehleSkyRenderer *sky, SehleTextureCubeMap *stars);

void sehle_sky_renderer_calculate_sky_position (unsigned int day_of_year, float time_of_day, float latitude, float *sun_azimuth, float *sun_altitude, float *sidereal_time);
float sehle_sky_renderer_ray_length_atmosphere (const EleaVec3f *p, const EleaVec3f *d);
void sehle_sky_renderer_atmospheric_attenuation (float dst_intensity[], const float src_intensity[], const float Kr[], const float Km[], float ray_length);
void sehle_sky_renderer_get_sky_color (EleaColor4f *dst,  const EleaVec3f *view_dir, const EleaVec3f *sun_dir, const EleaVec3f *Kr, const EleaVec3f *Km, float gm, const EleaColor4f *Rsun);

#ifdef __cplusplus
};
#endif

#endif
