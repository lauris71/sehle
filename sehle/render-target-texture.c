#define __SEHLE_RENDER_TARGET_TEXTURE_C__

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

#include "render-target-texture.h"

static void render_target_texture_class_init (SehleRenderTargetTextureClass *klass);
static void render_target_texture_finalize (SehleRenderTargetTextureClass *klass, SehleRenderTargetTexture *tgt);

/* SehleRenderTarget implementation */
static void render_target_texture_build (SehleResource *res);

static SehleRenderTargetClass *parent_class;

unsigned int
sehle_render_target_texture_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const unsigned char *) "SehleRenderTargetTexture", SEHLE_TYPE_RENDER_TARGET, sizeof (SehleRenderTargetTextureClass), sizeof (SehleRenderTargetTexture), 0,
						(void (*) (AZClass *)) render_target_texture_class_init,
						NULL,
						(void (*) (const AZImplementation *, void *)) render_target_texture_finalize);
	}
	return type;
}

static void
render_target_texture_class_init (SehleRenderTargetTextureClass *klass)
{
	parent_class = (SehleRenderTargetClass *) ((AZClass *) klass)->parent;
	klass->render_target_klass.resource_klass.build = render_target_texture_build;
}

static void
render_target_texture_finalize (SehleRenderTargetTextureClass *klass, SehleRenderTargetTexture *tgt)
{
	if (tgt->texdepth) {
		az_object_unref (AZ_OBJECT(tgt->texdepth));
	}
	if (tgt->texcolor) {
		az_object_unref (AZ_OBJECT(tgt->texcolor));
	}

	if (tgt->gl_rbuf_depth) glDeleteRenderbuffers (1, &tgt->gl_rbuf_depth);
}

static void
render_target_texture_build (SehleResource *res)
{
	SehleRenderTargetTexture *ttex = (SehleRenderTargetTexture *) res;

	if ((ttex->texture_format != SEHLE_TEXTURE_RGB) && (ttex->texture_format != SEHLE_TEXTURE_RGBA)) {
		fprintf (stderr, "render_target_texture_build: Invalid texture format (%u)\n", ttex->texture_format);
		sehle_resource_set_sate (res, SEHLE_RESOURCE_STATE_INVALID);
		return;
	}
	if ((ttex->channel_format != SEHLE_TEXTURE_U8) && (ttex->channel_format != SEHLE_TEXTURE_F16)) {
		fprintf (stderr, "render_target_texture_build: Invalid channel format (%u)\n", ttex->channel_format);
		sehle_resource_set_sate (res, SEHLE_RESOURCE_STATE_INVALID);
		return;
	}

	if (!res->gl_handle) glGenFramebuffers (1, &res->gl_handle);
	/*
	* GL_DRAW_FRAMEBUFFER
	* GL_READ_FRAMEBUFFER
	* GL_FRAMEBUFFER binds to both draw and read targets
	*/
	glBindFramebuffer (GL_FRAMEBUFFER, res->gl_handle);

	if (ttex->texcolor) {
		/* Color buffer is present */
		sehle_render_target_attach_color_texture (&ttex->render_target, ttex->texcolor, 0);
	} else {
		/* NULL color buffer */
		glDrawBuffer (GL_NONE);
		glReadBuffer (GL_NONE);
	}

	if (ttex->has_depth) {
		/* Depth buffer is present */
		if (ttex->texdepth) {
			/* Depth is texture */
			sehle_render_target_attach_depth_texture (&ttex->render_target, ttex->texdepth);
			if (ttex->gl_rbuf_depth) {
				glDeleteRenderbuffers (1, &ttex->gl_rbuf_depth);
				ttex->gl_rbuf_depth = 0;
			}
		} else {
			int depthtype = GL_DEPTH_STENCIL;
			if (ttex->depth_format == SEHLE_TEXTURE_DEPTH) {
				if (ttex->depth_channel == SEHLE_TEXTURE_D16) {
					depthtype = GL_DEPTH_COMPONENT16;
				} else if (ttex->depth_channel == SEHLE_TEXTURE_D24) {
					depthtype = GL_DEPTH_COMPONENT24;
				} else {
					depthtype = GL_DEPTH_COMPONENT32;
				}
			}
			if (!ttex->gl_rbuf_depth) glGenRenderbuffers (1, &ttex->gl_rbuf_depth);
			glBindRenderbuffer (GL_RENDERBUFFER, ttex->gl_rbuf_depth);
			glRenderbufferStorage (GL_RENDERBUFFER, depthtype, ttex->render_target.width, ttex->render_target.height);
			// glFramebufferRenderbuffer (GL_FRAMEBUFFER , GL_DEPTH_ATTACHMENT , GL_RENDERBUFFER , glrbufdepth);
			glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, ttex->gl_rbuf_depth);
		}
	}

	GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		fprintf (stderr, "Framebuffer error: %s\n", sehle_describe_framebuffer_status (status));
	}

	glBindFramebuffer (GL_FRAMEBUFFER, 0);
}

SehleRenderTargetTexture *sehle_render_target_texture_new_full (SehleEngine *engine, unsigned int width, unsigned int height,
	unsigned int has_color, SehleTexture2D *color_texture, unsigned int texture_format, unsigned int channel_format,
	unsigned int has_depth, unsigned int depth_is_texture, SehleTexture2D *depth_texture, unsigned int depth_format, unsigned int depth_channel)
{
	arikkei_return_val_if_fail (engine != NULL, NULL);
	arikkei_return_val_if_fail (SEHLE_IS_ENGINE (engine), NULL);
	arikkei_return_val_if_fail (has_color || !color_texture, NULL);
	arikkei_return_val_if_fail (!has_color || (texture_format == SEHLE_TEXTURE_RGB) || (texture_format == SEHLE_TEXTURE_RGBA), NULL);
	arikkei_return_val_if_fail (!has_color || (channel_format == SEHLE_TEXTURE_U8) || (channel_format == SEHLE_TEXTURE_F16), NULL);
	arikkei_return_val_if_fail (has_depth || !depth_texture, NULL);
	arikkei_return_val_if_fail (depth_is_texture || !depth_texture, NULL);
	arikkei_return_val_if_fail (!has_depth || (depth_format == SEHLE_TEXTURE_DEPTH) || (depth_format == SEHLE_TEXTURE_DEPTH_STENCIL), NULL);
	arikkei_return_val_if_fail (!has_depth || (depth_channel == SEHLE_TEXTURE_D16) || (depth_channel == SEHLE_TEXTURE_D24) || (depth_channel == SEHLE_TEXTURE_D32), NULL);
	arikkei_return_val_if_fail (!has_depth || (depth_format != SEHLE_TEXTURE_DEPTH_STENCIL) || (depth_channel == SEHLE_TEXTURE_D24), NULL);

	SehleRenderTargetTexture *ttex = (SehleRenderTargetTexture *) az_object_new (SEHLE_TYPE_RENDER_TARGET_TEXTURE);
	sehle_render_target_setup (&ttex->render_target, engine, width, height);

	ttex->texture_format = texture_format;
	ttex->channel_format = channel_format;
	if (has_color) {
		if (color_texture) {
			az_object_ref (AZ_OBJECT (color_texture));
			ttex->texcolor = color_texture;
		} else {
			ttex->texcolor = sehle_texture_2d_new (engine, NULL);
			sehle_texture_2d_set_format (ttex->texcolor, ttex->texture_format, ttex->channel_format, SEHLE_TEXTURE_COLOR_SPACE_LINEAR);
			sehle_texture_2d_set_filter (ttex->texcolor, SEHLE_TEXTURE_FILTER_LINEAR, SEHLE_TEXTURE_FILTER_LINEAR);
		}
		sehle_texture_2d_set_size (ttex->texcolor, width, height);
	}

	ttex->has_depth = has_depth;
	ttex->depth_format = depth_format;
	ttex->depth_channel = depth_channel;
	if (has_depth) {
		if (depth_is_texture) {
			if (depth_texture) {
				az_object_ref (AZ_OBJECT (depth_texture));
				ttex->texdepth = depth_texture;
			} else {
				ttex->texdepth = sehle_texture_2d_new (engine, NULL);
				sehle_texture_2d_set_format (ttex->texdepth, SEHLE_TEXTURE_DEPTH, SEHLE_TEXTURE_D24, SEHLE_TEXTURE_COLOR_SPACE_LINEAR);
				sehle_texture_2d_set_filter (ttex->texdepth, SEHLE_TEXTURE_FILTER_LINEAR, SEHLE_TEXTURE_FILTER_LINEAR);
			}
			sehle_texture_2d_set_size (ttex->texdepth, width, height);
		}
	}

	sehle_resource_set_sate (&ttex->render_target.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	return ttex;
}

SehleRenderTargetTexture *
sehle_render_target_texture_new (SehleEngine *engine, unsigned int width, unsigned int height, unsigned int texture_format, unsigned int channel_format, unsigned int has_depth, unsigned int has_depth_texture)
{
	return sehle_render_target_texture_new_full (engine, width, height, 1, NULL, texture_format, channel_format, has_depth, has_depth_texture, NULL, SEHLE_TEXTURE_DEPTH_STENCIL, SEHLE_TEXTURE_D24);
}

SehleRenderTargetTexture *
sehle_render_target_texture_new_tex (SehleEngine *engine, SehleTexture2D *texture, unsigned int has_depth, unsigned int has_depth_texture)
{
	return sehle_render_target_texture_new_full (engine, texture->width, texture->height, 1, texture, texture->texture.format, texture->texture.chtype, has_depth, has_depth_texture, NULL, SEHLE_TEXTURE_DEPTH_STENCIL, SEHLE_TEXTURE_D24);
}

SehleTexture2D *
sehle_render_target_texture_get_color (SehleRenderTargetTexture *ttex)
{
	if (ttex->texcolor) az_object_ref (AZ_OBJECT (ttex->texcolor));
	return ttex->texcolor;
}

SehleTexture2D *
sehle_render_target_texture_get_depth (SehleRenderTargetTexture *ttex)
{
	if (ttex->texdepth) az_object_ref (AZ_OBJECT (ttex->texdepth));
	return ttex->texdepth;
}

void
sehle_render_target_texture_set_color (SehleRenderTargetTexture *ttex, SehleTexture2D *texture)
{
	if (texture == ttex->texcolor) return;
	if (ttex->texcolor) az_object_unref (AZ_OBJECT (ttex->texcolor));
	ttex->texcolor = texture;
	if (ttex->texcolor) az_object_ref (AZ_OBJECT (ttex->texcolor));
	sehle_resource_set_sate (&ttex->render_target.resource, SEHLE_RESOURCE_STATE_MODIFIED);
}

void
sehle_render_target_texture_set_depth (SehleRenderTargetTexture *ttex, unsigned int has_depth, SehleTexture2D *texture)
{
	arikkei_return_if_fail (!has_depth || texture);
	if ((has_depth == ttex->has_depth) && (texture == ttex->texdepth)) return;
	ttex->has_depth = has_depth;
	if (ttex->texdepth) az_object_unref (AZ_OBJECT (ttex->texdepth));
	ttex->texdepth = texture;
	if (ttex->texdepth) az_object_ref (AZ_OBJECT (ttex->texdepth));
	sehle_resource_set_sate (&ttex->render_target.resource, SEHLE_RESOURCE_STATE_MODIFIED);
}

SehleTexture2D *
sehle_render_target_texture_replace_color (SehleRenderTargetTexture *ttex, SehleTexture2D *new_texture)
{
	arikkei_return_val_if_fail (ttex->texcolor, NULL);
	if (new_texture == ttex->texcolor) return new_texture;
	if (!new_texture) {
		new_texture = sehle_texture_2d_new (ttex->render_target.resource.engine, NULL);
		sehle_texture_2d_set_filter (new_texture, SEHLE_TEXTURE_FILTER_LINEAR, SEHLE_TEXTURE_FILTER_LINEAR);
	}
	SehleTexture2D *old_texture = sehle_render_target_texture_get_color (ttex);
	sehle_render_target_texture_set_color (ttex, new_texture);
	return old_texture;
}

SehleTexture2D *
sehle_render_target_texture_replace_depth (SehleRenderTargetTexture *ttex, SehleTexture2D *new_texture)
{
	arikkei_return_val_if_fail (ttex->has_depth && ttex->texdepth, NULL);
	if (new_texture == ttex->texdepth) return new_texture;
	if (!new_texture) {
		new_texture = sehle_texture_2d_new (ttex->render_target.resource.engine, NULL);
		sehle_texture_2d_set_format (new_texture, SEHLE_TEXTURE_DEPTH, SEHLE_TEXTURE_D24, SEHLE_TEXTURE_COLOR_SPACE_LINEAR);
		sehle_texture_2d_set_filter (new_texture, SEHLE_TEXTURE_FILTER_LINEAR, SEHLE_TEXTURE_FILTER_LINEAR);
	}
	SehleTexture2D *old_texture = sehle_render_target_texture_get_depth (ttex);
	sehle_render_target_texture_set_depth (ttex, 1, new_texture);
	return old_texture;
}
