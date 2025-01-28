#ifndef __SEHLE_UI_RENDERER_H__
#define __SEHLE_UI_RENDERER_H__

/*
* Libsehle
*
* Copyright (C) Lauris Kaplinski 2018
*
*/

#define SEHLE_TYPE_UI_RENDERER (sehle_ui_renderer_get_type ())
#define SEHLE_UI_RENDERER(o) (AZ_CHECK_INSTANCE_CAST ((o), SEHLE_TYPE_UI_RENDERER, SehleUIRenderer))
#define SEHLE_IS_UI_RENDERER(o) (AZ_CHECK_INSTANCE_TYPE ((o), SEHLE_TYPE_UI_RENDERER))

typedef struct _SehleUIRenderer SehleUIRenderer;
typedef struct _SehleUIRendererClass SehleUIRendererClass;

#include <az/object.h>
#include <elea/color.h>
#include <nr/types.h>

#include <sehle/sehle.h>
#include <sehle/render-state.h>

#include <az/object.h>

#define SEHLE_UI_RENDERER_DEFAULT_FLAGS ((SEHLE_RENDER_STATE_DEFAULT | SEHLE_BLEND) & ~(SEHLE_DEPTH_TEST | SEHLE_DEPTH_WRITE))

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleUIRenderer {
	AZObject object;
	SehleEngine *engine;

	SehleRenderTarget *target;
	NRRectl vport;

	unsigned int render_flags;
	SehleProgram *prog;
	SehleTexture2D *tex;
	SehleVertexArray *va[2];

	/* Current stream */
	float *attribs;
	unsigned int n_rects;
};

struct _SehleUIRendererClass {
	AZObjectClass object_class;
};

unsigned int sehle_ui_renderer_get_type (void);

SehleUIRenderer *sehle_ui_renderer_new (SehleRenderTarget *target);

void sehle_ui_renderer_set_viewport (SehleUIRenderer *rend, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1);

void sehle_ui_renderer_begin (SehleUIRenderer *rend, SehleProgram *prog, unsigned int render_flags);
void sehle_ui_renderer_set_texture (SehleUIRenderer *rend, SehleTexture2D *tex);
void sehle_ui_renderer_finish (SehleUIRenderer *rend);
void sehle_ui_renderer_add_rect (SehleUIRenderer *rend, const NRRectf *dst, const NRRectf *src, unsigned int src_width, unsigned int src_height, const EleaColor4f *colors);

#ifdef __cplusplus
};
#endif

#endif

