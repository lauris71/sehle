#ifndef __SEHLE_RENDER_TARGET_DEFERRED_H__
#define __SEHLE_RENDER_TARGET_DEFERRED_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2014
 *
 */

#define SEHLE_TYPE_RENDER_TARGET_DEFERRED (sehle_render_target_deferred_get_type ())
#define SEHLE_RENDER_TARGET_DEFERRED(r) (AZ_CHECK_INSTANCE_CAST ((r), SEHLE_TYPE_RENDER_TARGET_DEFERRED, SehleRenderTargetDeferred))
#define SEHLE_IS_RENDER_TARGET_DEFERRED(r) (AZ_CHECK_INSTANCE_TYPE ((r), SEHLE_TYPE_RENDER_TARGET_DEFERRED))

typedef struct _SehleRenderTargetDeferred SehleRenderTargetDeferred;
typedef struct _SehleRenderTargetDeferredClass SehleRenderTargetDeferredClass;

#include <sehle/render-target.h>

/*
 * RGBA buffer rendering
 *
 * fixme: RGBA does not work with 16-bit buffers - transparency blending messes up final alpha value
 *
 */

#define SEHLE_RENDER_TARGET_TEXTURE_DEPTH 0
#define SEHLE_RENDER_TARGET_TEXTURE_NORMAL 1
#define SEHLE_RENDER_TARGET_TEXTURE_ALBEDO_AMBIENT 2
#define SEHLE_RENDER_TARGET_TEXTURE_SPECULAR_SHININESS 3
#define SEHLE_RENDER_TARGET_NUM_TEXTURE_TYPES 4

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleRenderTargetDeferred {
	SehleRenderTarget render_target;

	SehleTexture2D *tex[SEHLE_RENDER_TARGET_NUM_TEXTURE_TYPES];
	unsigned int handle[SEHLE_RENDER_TARGET_NUM_TEXTURE_TYPES];
};

struct _SehleRenderTargetDeferredClass {
	SehleRenderTargetClass render_target_klass;
};

unsigned int sehle_render_target_deferred_get_type (void);

SehleRenderTargetDeferred *sehle_render_target_deferred_new (SehleEngine *engine, unsigned int width, unsigned int height);

#ifdef __cplusplus
};
#endif

#endif

