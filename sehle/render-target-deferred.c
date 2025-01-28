#define __SEHLE_RENDER_TARGET_DEFERRED_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2014
 *
 */

static const int debug = 0;

#include <stdio.h>

#include <arikkei/arikkei-utils.h>

#include "GL/glew.h"

#include "engine.h"
#include "texture-2d.h"

#include "render-target-deferred.h"

static void render_target_deferred_class_init (SehleRenderTargetDeferredClass *klass);
static void render_target_deferred_finalize (SehleRenderTargetDeferredClass *klass, SehleRenderTargetDeferred *tgt);

/* SehleRenderTarget implementation */
static void render_target_deferred_build (SehleResource *res);

static SehleRenderTargetClass *parent_class;

unsigned int
sehle_render_target_deferred_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const unsigned char *) "SehleRenderTargetDeferred", SEHLE_TYPE_RENDER_TARGET, sizeof (SehleRenderTargetDeferredClass), sizeof (SehleRenderTargetDeferred), 0,
						(void (*) (AZClass *)) render_target_deferred_class_init,
						NULL,
						(void (*) (const AZImplementation *, void *)) render_target_deferred_finalize);
	}
	return type;
}

static void
render_target_deferred_class_init (SehleRenderTargetDeferredClass *klass)
{
	parent_class = (SehleRenderTargetClass *) ((AZClass *) klass)->parent;
	klass->render_target_klass.resource_klass.build = render_target_deferred_build;
}

static void
render_target_deferred_finalize (SehleRenderTargetDeferredClass *klass, SehleRenderTargetDeferred *tgt)
{
	for (int i = 0; i < SEHLE_RENDER_TARGET_NUM_TEXTURE_TYPES; i++) {
		if (tgt->tex[i]) {
			az_object_unref (AZ_OBJECT(tgt->tex[i]));
		}
	}
}

static void
render_target_deferred_build (SehleResource *res)
{
	SehleRenderTargetDeferred *deftgt = (SehleRenderTargetDeferred *) res;

	if (!res->gl_handle) glGenFramebuffers (1, &res->gl_handle);

	glBindFramebuffer (GL_FRAMEBUFFER, res->gl_handle);
	sehle_render_target_attach_depth_texture (&deftgt->render_target, deftgt->tex[SEHLE_RENDER_TARGET_TEXTURE_DEPTH]);
	sehle_render_target_attach_color_texture (&deftgt->render_target, deftgt->tex[SEHLE_RENDER_TARGET_TEXTURE_NORMAL], 0);
	sehle_render_target_attach_color_texture (&deftgt->render_target, deftgt->tex[SEHLE_RENDER_TARGET_TEXTURE_ALBEDO_AMBIENT], 1);
	sehle_render_target_attach_color_texture (&deftgt->render_target, deftgt->tex[SEHLE_RENDER_TARGET_TEXTURE_SPECULAR_SHININESS], 2);

	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers (3, buffers);

	GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		fprintf (stderr, "Framebuffer error: %s\n", sehle_describe_framebuffer_status (status));
	}

	glBindFramebuffer (GL_FRAMEBUFFER, 0);
}

SehleRenderTargetDeferred *
sehle_render_target_deferred_new (SehleEngine *engine, unsigned int width, unsigned int height)
{
	static unsigned int formats[] = { SEHLE_TEXTURE_DEPTH, SEHLE_TEXTURE_RGBA, SEHLE_TEXTURE_RGBA, SEHLE_TEXTURE_RGBA };
	static unsigned int channel_types[] = { SEHLE_TEXTURE_D24, SEHLE_TEXTURE_U8, SEHLE_TEXTURE_U8, SEHLE_TEXTURE_U8 };
	arikkei_return_val_if_fail (engine != NULL, NULL);

	SehleRenderTargetDeferred *tgt = (SehleRenderTargetDeferred *) az_object_new (SEHLE_TYPE_RENDER_TARGET_DEFERRED);
	sehle_render_target_setup (SEHLE_RENDER_TARGET(tgt), engine, width, height);

	for (int i = 0; i < SEHLE_RENDER_TARGET_NUM_TEXTURE_TYPES; i++) {
		tgt->tex[i] = sehle_texture_2d_new (engine, NULL);
		if (i != 2) {
			sehle_texture_2d_set_format (tgt->tex[i], formats[i], channel_types[i], SEHLE_TEXTURE_COLOR_SPACE_LINEAR);
		} else {
			sehle_texture_2d_set_format (tgt->tex[i], formats[i], channel_types[i], SEHLE_TEXTURE_COLOR_SPACE_SRGB);
		}
		sehle_texture_2d_set_size (tgt->tex[i], width, height);
	}
	sehle_texture_2d_set_filter (tgt->tex[SEHLE_RENDER_TARGET_TEXTURE_DEPTH], SEHLE_TEXTURE_FILTER_LINEAR, SEHLE_TEXTURE_FILTER_LINEAR);
	sehle_resource_set_sate (&tgt->render_target.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	return tgt;
}

