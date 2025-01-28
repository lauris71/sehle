#define __SEHLE_RENDERTARGET_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2014
 *
 */

static const int debug = 0;

#include <arikkei/arikkei-utils.h>

#include "GL/glew.h"

#include <sehle/engine.h>
#include <sehle/texture-2d.h>

#include <sehle/render-target.h>

static void render_target_class_init (SehleRenderTargetClass *klass);

/* ArikkeiObject implementation */
static void render_target_shutdown (AZObject *object);

static SehleResourceClass *parent_class;

unsigned int
sehle_render_target_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const unsigned char *) "SehleRenderTarget", SEHLE_TYPE_RESOURCE, sizeof (SehleRenderTargetClass), sizeof (SehleRenderTarget), 0,
			(void (*) (AZClass *)) render_target_class_init,
			NULL, NULL);
	}
	return type;
}

static void
render_target_class_init (SehleRenderTargetClass *klass)
{
	parent_class = (SehleResourceClass *) ((AZClass *) klass)->parent;
	((AZObjectClass *) klass)->shutdown = render_target_shutdown;
}

static void
render_target_shutdown (AZObject *object)
{
	SehleRenderTarget *tgt = SEHLE_RENDER_TARGET(object);
	tgt->resource.engine = NULL;
	if (tgt->resource.gl_handle) {
		glDeleteFramebuffers (1, &tgt->resource.gl_handle);
		tgt->resource.gl_handle = 0;
	}
	if (((AZObjectClass *) parent_class)->shutdown) {
		((AZObjectClass *) parent_class)->shutdown (object);
	}
}

void
sehle_render_target_setup (SehleRenderTarget *tgt, SehleEngine *engine, unsigned int width, unsigned int height)
{
	arikkei_return_if_fail (tgt != NULL);
	arikkei_return_if_fail (SEHLE_IS_RENDER_TARGET(tgt));
	arikkei_return_if_fail (engine != NULL);
	sehle_resource_setup (&tgt->resource, engine, NULL);
	tgt->width = width;
	tgt->height = height;
}

void
sehle_render_target_attach_color_texture (SehleRenderTarget *tgt, SehleTexture2D *tex2d, unsigned int attachment)
{
	sehle_texture_2d_set_size (tex2d, tgt->width, tgt->height);
	unsigned int gl_handle = sehle_resource_get_handle (&tex2d->texture.resource);
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D, tex2d->texture.resource.gl_handle, 0);
	SEHLE_CHECK_ERRORS (0);
}

void
sehle_render_target_attach_depth_texture (SehleRenderTarget *tgt, SehleTexture2D *tex2d)
{
	sehle_texture_2d_set_size (tex2d, tgt->width, tgt->height);
	unsigned int gl_handle = sehle_resource_get_handle (&tex2d->texture.resource);
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex2d->texture.resource.gl_handle, 0);
	SEHLE_CHECK_ERRORS (0);
}

void
sehle_render_target_resize (SehleRenderTarget *tgt, unsigned int width, unsigned int height)
{
	arikkei_return_if_fail (tgt != NULL);
	arikkei_return_if_fail (SEHLE_IS_RENDER_TARGET(tgt));
	if ((width == tgt->width) && (height == tgt->height)) return;
	if (((SehleRenderTargetClass *) ((AZObject *) tgt)->klass)->resize) {
		((SehleRenderTargetClass *) ((AZObject *) tgt)->klass)->resize (tgt, width, height);
	}
	tgt->width = width;
	tgt->height = height;
	sehle_resource_set_sate (&tgt->resource, SEHLE_RESOURCE_STATE_MODIFIED);
}

SehleRenderTarget *
sehle_render_target_new_viewport (SehleEngine *engine, unsigned int width, unsigned int height)
{
	arikkei_return_val_if_fail (engine != NULL, NULL);
	SehleRenderTarget *tgt = (SehleRenderTarget *) az_object_new (SEHLE_TYPE_RENDER_TARGET);
	sehle_render_target_setup (tgt, engine, width, height);
	sehle_resource_set_sate (&tgt->resource, SEHLE_RESOURCE_STATE_MODIFIED);
	return tgt;
}

