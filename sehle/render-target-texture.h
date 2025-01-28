#ifndef __SEHLE_RENDER_TARGET_TEXTURE_H__
#define __SEHLE_RENDER_TARGET_TEXTURE_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2014
 *
 */

#define SEHLE_TYPE_RENDER_TARGET_TEXTURE (sehle_render_target_texture_get_type ())
#define SEHLE_RENDER_TARGET_TEXTURE(r) (AZ_CHECK_INSTANCE_CAST ((r), SEHLE_TYPE_RENDER_TARGET_TEXTURE, SehleRenderTargetTexture))
#define SEHLE_IS_RENDER_TARGET_TEXTURE(r) (AZ_CHECK_INSTANCE_TYPE ((r), SEHLE_TYPE_RENDER_TARGET_TEXTURE))

typedef struct _SehleRenderTargetTexture SehleRenderTargetTexture;
typedef struct _SehleRenderTargetTextureClass SehleRenderTargetTextureClass;

#include <sehle/render-target.h>

/*
 * RGBA buffer rendering
 */

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleRenderTargetTexture {
	SehleRenderTarget render_target;

	/*
	 * If color_texture == NULL render target does not have color attachment
	 */

	/* RGB or RGBA */
	unsigned int texture_format : 4;
	/* UINT8 or FLOAT16 */
	unsigned int channel_format : 4;
	SehleTexture2D *texcolor;

	/*
	 * If has_depth and depth_texture == NULL render target has depth framebuffer
	 */
	unsigned int has_depth : 1;
	/* DEPTH or DEPTH_STENCIL */
	unsigned int depth_format : 4;
	/* FLOAT16, FLOAT24 or FLOAT32, if format is DEPTH_STENCIL only FLOAT24 is allowed */
	unsigned int depth_channel : 4;
	SehleTexture2D *texdepth;
	unsigned int gl_rbuf_depth;
};

struct _SehleRenderTargetTextureClass {
	SehleRenderTargetClass render_target_klass;
};

unsigned int sehle_render_target_texture_get_type (void);

/*
 * If color or depth texture format or size differs from requested values it is reshaped silently
 */
SehleRenderTargetTexture *sehle_render_target_texture_new_full (SehleEngine *engine, unsigned int width, unsigned int height,
	unsigned int has_color, SehleTexture2D *color_texture, unsigned int texture_format, unsigned int channel_format,
	unsigned int has_depth, unsigned int depth_is_texture, SehleTexture2D *depth_texture, unsigned int depth_format, unsigned int depth_channel);

/* These create FLOAT24 DEPTH_STENCIL depth texture or renderbuffer */
SehleRenderTargetTexture *sehle_render_target_texture_new (SehleEngine *engine, unsigned int width, unsigned int height, unsigned int texture_format, unsigned int channel_format, unsigned int has_depth, unsigned int has_depth_texture);
SehleRenderTargetTexture *sehle_render_target_texture_new_tex (SehleEngine *engine, SehleTexture2D *texture, unsigned int has_depth, unsigned int has_depth_texture);

/* Get new reference to texture or NULL if not present */
SehleTexture2D *sehle_render_target_texture_get_color (SehleRenderTargetTexture *ttex);
SehleTexture2D *sehle_render_target_texture_get_depth (SehleRenderTargetTexture *ttex);
/*
 * If color texture format or size differs from rendertarget values the texture is reshaped silently
 * If new texture is NULL color buffer will be disabled
 */
void sehle_render_target_texture_set_color (SehleRenderTargetTexture *ttex, SehleTexture2D *texture);
/*
* If depth texture format or size differs from rendertarget values the texture is reshaped silently
* If has_depth is true and the new texture is NULL renderbuffer is created
*/
void sehle_render_target_texture_set_depth (SehleRenderTargetTexture *ttex, unsigned int has_depth, SehleTexture2D *texture);
/*
 * Transfers old texture reference to caller and grabs the reference to new texture
 * If texture is NULL a new one is created
 * Not allowed if render target has no color
 */
SehleTexture2D *sehle_render_target_texture_replace_color (SehleRenderTargetTexture *ttex, SehleTexture2D *new_texture);
/*
 * Transfers old texture reference to caller and grabs the reference to new texture
 * If texture is NULL a new one is created
 * Not allowed if render target has no depth of depth is framebuffer
 */
SehleTexture2D *sehle_render_target_texture_replace_depth (SehleRenderTargetTexture *ttex, SehleTexture2D *new_texture);

#ifdef __cplusplus
};
#endif

#endif

