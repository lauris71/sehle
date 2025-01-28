#ifndef __SEHLE_RENDERABLE_H__
#define __SEHLE_RENDERABLE_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

/*
 * SehleRenderable
 *
 * Abstract base interface of all renderable entities
 *
 * Renderables have specified bounds in world space for culling
 * Implementations are responsible for updating bounds and in submitting geometry into rendering pipeline
 */

#define SEHLE_TYPE_RENDERABLE (sehle_renderable_get_type ())
#define SEHLE_RENDERABLE(r) ((SehleRenderableInstance *) (r))
#define SEHLE_IS_RENDERABLE_IMPLEMENTATION(i) arikkei_type_is_a (((ArikkeiImplementation *) (i))->type, SEHLE_TYPE_RENDERABLE)

typedef struct _SehleRenderableClass SehleRenderableClass;
typedef struct _SehleRenderableImplementation SehleRenderableImplementation;
typedef struct _SehleRenderableInstance SehleRenderableInstance;
typedef struct _SehleRenderableInstance SehleRenderable;

#include <az/interface.h>

#include <elea/aabox.h>
#include <elea/polyhedron.h>

#include <sehle/render-context.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleRenderableInstance {
	/* Mask of render stages for quick culling, default is SEHLE_RENDER_STAGES_ALL */
	unsigned int render_stages;
	/* Visibility mask, semantics are determined by application, default is all layers */
	unsigned int layer_mask;
	/* Visual data */
	EleaAABox3f bbox;
};

struct _SehleRenderableImplementation {
	AZImplementation interface_impl;
	/* Ask renderable to schedule itself for rendering at given stages */
	void (*display) (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx);
	/* Perform renderable-specific parts of rendering (transforms and submitting vertices) */
	/* Program has to be bound to rendercontext already */
	void (*render) (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data);
};

struct _SehleRenderableClass {
	AZInterfaceClass klass;
};

/* Type system */
unsigned int sehle_renderable_get_type (void);

/* For display system implementation */
/* Test visibility and invoke display if visible */
ARIKKEI_INLINE void
sehle_renderable_display (SehleRenderableImplementation* impl, SehleRenderableInstance* inst, SehleRenderContext* ctx, SehleDisplayContext* displayctx)
{
	if (!(inst->render_stages & displayctx->stages)) return;
	if (!(inst->layer_mask & displayctx->mask)) return;
	if (elea_polyhedron3f_test_aabox (&ctx->vspace, &inst->bbox) == ELEA_POSITION_OUT) return;
	impl->display (impl, inst, ctx, displayctx);
}

ARIKKEI_INLINE void
sehle_renderable_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data)
{
	impl->render (impl, inst, ctx, prog, render_type, data);
}

#ifdef __cplusplus
};
#endif

#endif

