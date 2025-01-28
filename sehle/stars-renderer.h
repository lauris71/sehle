#ifndef __SEHLE_STARS_RENDERER_H__
#define __SEHLE_STARS_RENDERER_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2016-2021
 */

typedef struct _SehleStarsRenderer SehleStarsRenderer;
typedef struct _SehleStarsRendererClass SehleStarsRendererClass;

#define SEHLE_TYPE_STARS_RENDERER sehle_stars_renderer_get_type ()

#define SEHLE_STARS_RENDERER_FROM_RENDERABLE_INSTANCE(i) (SehleStarsRenderer *) ARIKKEI_BASE_ADDRESS (SehleStarsRenderer, renderable_inst, i)
#define SEHLE_STARS_RENDERER_RENDERABLE_IMPLEMENTATION (&sehle_stars_renderer_class->renderable_impl)
#define SEHLE_STARS_RENDERER_FROM_MATERIAL_INSTANCE(i) (SehleStarsRenderer *) ARIKKEI_BASE_ADDRESS (SehleStarsRenderer, material_inst, i)
#define SEHLE_STARS_RENDERER_MATERIAL_IMPLEMENTATION (&sehle_stars_renderer_class->material_impl)

#include <sehle/material.h>
#include <sehle/renderable.h>

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_stars_get_reference (SehleEngine *engine);

struct _SehleStarsRenderer {
	SehleRenderableInstance renderable_inst;
	SehleMaterialInstance material_inst;
	SehleVertexArray *va;
	/* Starfield transformation */
	EleaMat3x4f w2s;
	/* Star properties */
	float starBrightness;
	float starGamma;
};

struct _SehleStarsRendererClass {
	AZClass klass;
	SehleRenderableImplementation renderable_impl;
	SehleMaterialImplementation material_impl;
};

#ifndef __SEHLE_STARS_RENDERER_C__
extern unsigned int sehle_stars_renderer_type;
extern SehleStarsRendererClass *sehle_stars_renderer_class;
#endif

unsigned int sehle_stars_renderer_get_type (void);

void sehle_stars_renderer_setup (SehleStarsRenderer *stars, SehleEngine *engine, unsigned int n_stars, const float *star_data);
void sehle_stars_renderer_release (SehleStarsRenderer *stars);

#ifdef __cplusplus
};
#endif

#endif
