#ifndef __SEHLE_RENDER_TARGET_H__
#define __SEHLE_RENDER_TARGET_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2016
 *
 */

#define SEHLE_TYPE_RENDER_TARGET (sehle_render_target_get_type ())
#define SEHLE_RENDER_TARGET(r) (AZ_CHECK_INSTANCE_CAST ((r), SEHLE_TYPE_RENDER_TARGET, SehleRenderTarget))
#define SEHLE_IS_RENDER_TARGET(r) (AZ_CHECK_INSTANCE_TYPE ((r), SEHLE_TYPE_RENDER_TARGET))

typedef struct _SehleRenderTarget SehleRenderTarget;
typedef struct _SehleRenderTargetClass SehleRenderTargetClass;

#include <sehle/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleRenderTarget {
	SehleResource resource;

	unsigned int width;
	unsigned int height;
};

struct _SehleRenderTargetClass {
	SehleResourceClass resource_klass;
	/* Resizes associated resources */
	void (* resize) (SehleRenderTarget *tgt, unsigned int width, unsigned int height);
};

unsigned int sehle_render_target_get_type (void);

void sehle_render_target_resize (SehleRenderTarget *tgt, unsigned int width, unsigned int height);

/* Create new default rendertarget */
SehleRenderTarget *sehle_render_target_new_viewport (SehleEngine *engine, unsigned int width, unsigned int height);

/* For subclass implementations */
void sehle_render_target_setup (SehleRenderTarget *tgt, SehleEngine *engine, unsigned int width, unsigned int height);
void sehle_render_target_attach_color_texture (SehleRenderTarget *tgt, SehleTexture2D *tex2d, unsigned int attachment);
void sehle_render_target_attach_depth_texture (SehleRenderTarget *tgt, SehleTexture2D *tex2d);

#ifdef __cplusplus
};
#endif

#endif

