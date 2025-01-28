#ifndef __SEHLE_SMAA_RENDERER_H__
#define __SEHLE_SMAA_RENDERER_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2014
 *
 */

#define SEHLE_TYPE_SMAA_RENDERER (sehle_smaa_renderer_get_type ())
#define SEHLE_SMAA_RENDERER(o) (AZ_CHECK_INSTANCE_CAST ((o), SEHLE_TYPE_SMAA_RENDERER, SehleSMAARenderer))
#define SEHLE_IS_SMAA_RENDERER(o) (AZ_CHECK_INSTANCE_TYPE ((o), SEHLE_TYPE_SMAA_RENDERER))

typedef struct _SehleSMAARenderer SehleSMAARenderer;
typedef struct _SehleSMAARendererClass SehleSMAARendererClass;

#include <az/object.h>
#include <sehle/render-context.h>

typedef struct _SehleRenderTargetTexture SehleRenderTargetTexture;

/*
 * An encapsulated postprocessing object
 *
 */

#define SEHLE_SMAA_EDGES_TARGET 0
#define SEHLE_SMAA_WEIGHTS_TARGET 1
#define SEHLE_SMAA_NUM_TARGETS 2

#define SEHLE_SMAA_AREA_TEXTURE 0
#define SEHLE_SMAA_SEARCH_TEXTURE 1
#define SEHLE_SMAA_EDGES_TEXTURE 2
#define SEHLE_SMAA_WEIGHTS_TEXTURE 3
#define SEHLE_SMAA_NUM_TEXTURES 4

#define SEHLE_SMAA_EDGES_PROGRAM 0
#define SEHLE_SMAA_WEIGHTS_PROGRAM 1
#define SEHLE_SMAA_BLEND_PROGRAM 2
#define SEHLE_SMAA_NUM_PROGRAMS 3

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleSMAARenderer {
	AZObject object;

	SehleEngine *engine;
	unsigned int target_width;
	unsigned int target_height;

	SehleRenderContext ctx;
	SehleRenderTargetTexture *targets[SEHLE_SMAA_NUM_TARGETS];
	SehleTexture2D *textures[SEHLE_SMAA_NUM_TEXTURES];
	SehleProgram *programs[SEHLE_SMAA_NUM_PROGRAMS];
};

struct _SehleSMAARendererClass {
	AZObjectClass object_class;
};

unsigned int sehle_smaa_renderer_get_type (void);

/* If targets or textures are present they are recycled, otherwise new ones are created */
SehleSMAARenderer *sehle_smaa_renderer_new (SehleEngine *engine, unsigned int width, unsigned int height,
	SehleRenderTargetTexture *edges_tgt, SehleRenderTargetTexture *blend_tgt, SehleTexture2D *edges_tex, SehleTexture2D *blend_tex);

void sehle_smaa_renderer_resize (SehleSMAARenderer *rend, unsigned int width, unsigned int height);

void sehle_smaa_renderer_render (SehleSMAARenderer *rend, SehleRenderTarget *tgt, SehleTexture2D *source);
/* Render area of initial texture to full target */
/*void sehle_smaa_renderer_render_oversized (SehleSMAARenderer *rend, SehleRenderTarget *tgt, SehleTexture2D *source);*/

#ifdef __cplusplus
};
#endif

#endif

