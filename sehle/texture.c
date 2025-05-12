#define __SEHLE_TEXTURE_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2011
//

#ifdef DEBUG_OPENGL
static const int debug = 1;
#else
static const int debug = 0;
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "GL/glew.h"

#include "engine.h"

#include "texture.h"

static void texture_class_init (SehleTextureClass *klass);
static void texture_init (SehleTextureClass *klass, SehleTexture *tex);
static void texture_finalize (SehleTextureClass *klass, SehleTexture *tex);
/* AZObject implementation */
static void texture_shutdown (AZObject *obj);

static unsigned int texture_type = 0;
static SehleTextureClass *texture_class = NULL;
static SehleResourceClass *parent_class = NULL;

unsigned int
sehle_texture_get_type (void)
{
	if (!texture_type) {
		texture_class = (SehleTextureClass *) az_register_type (&texture_type, (const unsigned char *) "SehleTexture", SEHLE_TYPE_RESOURCE, sizeof (SehleTextureClass), sizeof (SehleTexture), AZ_FLAG_ABSTRACT,
			(void (*) (AZClass *)) texture_class_init,
			(void (*) (const AZImplementation *, void *)) texture_init,
			NULL);
		parent_class = (SehleResourceClass *) az_class_parent((AZClass *) texture_class);
	}
	return texture_type;
}

static void
texture_class_init (SehleTextureClass *klass)
{
	klass->resource_class.active_object_class.object_class.shutdown = texture_shutdown;
}

static void
texture_init (SehleTextureClass *klass, SehleTexture *tex)
{
	tex->format = SEHLE_TEXTURE_RGBA;
	tex->chtype = SEHLE_TEXTURE_U8;
}

static void
texture_shutdown (AZObject *obj)
{
	SehleTexture *tex = (SehleTexture *) obj;
	if (tex->resource.gl_handle) {
		glDeleteTextures (1, &tex->resource.gl_handle);
		tex->resource.gl_handle = 0;
	}
	((AZObjectClass *) parent_class)->shutdown (obj);
}

void
sehle_texture_setup (SehleTexture *tex, SehleEngine *engine, const unsigned char *id)
{
	arikkei_return_if_fail (!sehle_resource_is_initialized (&tex->resource));
	sehle_resource_setup (&tex->resource, engine, id);
}

void
sehle_texture_bind (SehleTexture *tex, unsigned int channel)
{
	arikkei_return_if_fail (tex != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE (tex));
	arikkei_return_if_fail (sehle_resource_is_initialized (&tex->resource));
	arikkei_return_if_fail (!sehle_resource_is_invalid (&tex->resource));
	arikkei_return_if_fail (tex->resource.engine->running);
	((SehleTextureClass *) ((AZObject *) tex)->klass)->bind (tex, channel);
}

unsigned int
sehle_texture_get_gl_data_format_from_format (unsigned int format)
{
	static const int formats[] = { GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_RG, GL_RGB, GL_RGBA, GL_RGBA, GL_RGB, GL_RGBA, GL_RGBA };
	return formats[format];
}

unsigned int
sehle_texture_get_gl_data_format (SehleTexture *tex)
{
	return sehle_texture_get_gl_data_format_from_format (tex->format);
}

unsigned int
sehle_texture_get_gl_data_type_from_type (unsigned int ch_type)
{
	static const int types[] = { GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT };
	return types[ch_type];
}

unsigned int
sehle_texture_get_gl_data_type (SehleTexture *tex)
{
	return sehle_texture_get_gl_data_type_from_type (tex->chtype);
}

unsigned int
sehle_texture_get_gl_internal_format (SehleTexture *tex)
{
	static const int formats[][8][13] = {{
		/* Linear */
		/* U8 */
		{ GL_R8, GL_RG8, GL_RGB8, GL_RGBA8, 0, 0, GL_COMPRESSED_RG_RGTC2, GL_COMPRESSED_RGB8_ETC2, GL_COMPRESSED_RGBA_BPTC_UNORM, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT },
		/* I8 */
		{ GL_R8I, GL_RG8I, GL_RGB8I, GL_RGBA8I, 0, 0, GL_COMPRESSED_SIGNED_RG_RGTC2, 0, 0, 0, 0, 0, 0 },
		/* U16 */
		{ GL_R16, GL_RG16, GL_RGB16, GL_RGBA16, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		/* F16 */
		{ GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		/* F32 */
		{ GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		/* D16 */
		{ 0, 0, 0, 0, GL_DEPTH_COMPONENT16, 0, 0, 0, 0, 0, 0, 0, 0 },
		/* D24 */
		{ 0, 0, 0, 0, GL_DEPTH_COMPONENT24, GL_DEPTH_STENCIL, 0, 0, 0, 0, 0, 0, 0 },
		/* D32 */
		{ 0, 0, 0, 0, GL_DEPTH_COMPONENT32, 0, 0, 0, 0, 0, 0, 0, 0 }
		}, {
		/* SRGB(A) */
		/* U8 */
#if __APPLE__
		{ 0, 0, GL_SRGB8, GL_SRGB8_ALPHA8, 0, 0, 0, GL_RGB8, GL_RGBA8, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, 0, 0, 0 },
#else
		{ 0, 0, GL_SRGB8, GL_SRGB8_ALPHA8, 0, 0, 0, GL_COMPRESSED_SRGB8_ETC2, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, 0, 0, 0 },
#endif
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
		} };
	return formats[tex->color_space][tex->chtype][tex->format];
}

unsigned int
sehle_texture_get_mipmap_levels (unsigned int width, unsigned int height)
{
	unsigned int size = (width >= height) ? width : height;
	unsigned int levels = 1;
	while (size > 1) {
		levels += 1;
		size /= 2;
	}
	return levels;
}

unsigned int
sehle_texture_get_format_from_pixblock (const NRPixBlock *pb)
{
	static const unsigned int formats[] = { SEHLE_TEXTURE_R, SEHLE_TEXTURE_RG, SEHLE_TEXTURE_RGB, SEHLE_TEXTURE_RGBA };
	return formats[pb->n_channels - 1];
}

unsigned int
sehle_texture_get_channel_type_from_pixblock (const NRPixBlock *pb)
{
	static const unsigned int ch_formats[] = { SEHLE_TEXTURE_U8, SEHLE_TEXTURE_U16, SEHLE_TEXTURE_F32 };
	return ch_formats[pb->ch_type];
}

unsigned int
sehle_texture_get_color_space_from_pixblock (const NRPixBlock *pb)
{
	static const unsigned int color_spaces[] = { SEHLE_TEXTURE_COLOR_SPACE_LINEAR, SEHLE_TEXTURE_COLOR_SPACE_SRGB };
	return color_spaces[pb->colorspace];
}

unsigned int
sehle_texture_get_gl_source_format_from_pixblock (const NRPixBlock *pb)
{
	static const unsigned int formats[] = { GL_RED, GL_RG, GL_RGB, GL_RGBA };
	return formats[pb->n_channels - 1];
}

unsigned int
sehle_texture_get_gl_source_type_from_pixblock (const NRPixBlock *pb)
{
	static const unsigned int ch_formats[] = { GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_FLOAT };
	return ch_formats[pb->ch_type];
}

unsigned int
sehle_texture_format_from_nr (unsigned int pixblock_mode)
{
	static const unsigned int formats[] = { SEHLE_TEXTURE_R, SEHLE_TEXTURE_RGB, SEHLE_TEXTURE_RGBA, SEHLE_TEXTURE_RGBA };
	return formats[pixblock_mode];
}

unsigned int
sehle_texture_format_to_nr (unsigned int format)
{
	static const unsigned int nrformats[13] = {
		NR_PIXBLOCK_MODE_G8, NR_PIXBLOCK_MODE_G8, NR_PIXBLOCK_MODE_R8G8B8, NR_PIXBLOCK_MODE_R8G8B8A8N,
		NR_PIXBLOCK_MODE_G8, NR_PIXBLOCK_MODE_G8,
		NR_PIXBLOCK_MODE_R8G8B8A8N, NR_PIXBLOCK_MODE_R8G8B8A8N, NR_PIXBLOCK_MODE_R8G8B8A8N, NR_PIXBLOCK_MODE_R8G8B8A8N, NR_PIXBLOCK_MODE_R8G8B8A8N, NR_PIXBLOCK_MODE_R8G8B8A8N, NR_PIXBLOCK_MODE_R8G8B8A8N };
	return nrformats[format];
}

unsigned int
sehle_get_pow2_ceil (unsigned int val)
{
	val &= 0x7fffffff;
	unsigned int s = 1;
	while (s < val) s <<= 1;
	return s;
}

unsigned int
sehle_get_pow2_floor (unsigned int val)
{
	val &= 0x7fffffff;
	unsigned int s = 1;
	while (s < val) s <<= 1;
	return (s == val) ? s : s >> 1;
}

