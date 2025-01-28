#define __SEHLE_TEXTURE_2D_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2012
//

static const unsigned int debug = 0;

#include <stdlib.h>
#include <stdio.h>

#include "GL/glew.h"

#include <nr/pixblock.h>

#include <sehle/engine.h>

#include <sehle/texture-2d.h>

static void texture_2d_class_init (SehleTexture2DClass *klass);
static void texture_2d_init (SehleTexture2DClass *klass, SehleTexture2D *tex2d);
static void texture_2d_finalize (SehleTexture2DClass *klass, SehleTexture2D *tex2d);

static void texture_2d_build (SehleResource *res);
static void texture_2d_bind (SehleTexture *tex, unsigned int channel);

unsigned int
sehle_texture_2d_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const unsigned char *) "SehleTexture2D", SEHLE_TYPE_TEXTURE, sizeof (SehleTexture2DClass), sizeof (SehleTexture2D), 0,
			(void (*) (AZClass *)) texture_2d_class_init,
			(void (*) (const AZImplementation *, void *)) texture_2d_init,
			(void (*) (const AZImplementation *, void *)) texture_2d_finalize);
	}
	return type;
}

static void
texture_2d_class_init (SehleTexture2DClass *klass)
{
	((SehleTextureClass *) klass)->resource_class.build = texture_2d_build;
	((SehleTextureClass *) klass)->bind = texture_2d_bind;
}

static void
texture_2d_init (SehleTexture2DClass *klass, SehleTexture2D *tex2d)
{
	tex2d->min_filter = SEHLE_TEXTURE_FILTER_MIPMAP;
	tex2d->mag_filter = SEHLE_TEXTURE_FILTER_LINEAR;
	tex2d->gen_mipmaps = 1;
}

static void
texture_2d_finalize (SehleTexture2DClass *klass, SehleTexture2D *tex2d)
{
	if (tex2d->pixels) {
		free (tex2d->pixels);
	}
}

static void
texture_2d_build (SehleResource *res)
{
	SehleTexture2D *tex2d = (SehleTexture2D *) res;
	if (!res->gl_handle) {
		SEHLE_CHECK_ERRORS (0);
		glGenTextures (1, &res->gl_handle);
		SEHLE_CHECK_ERRORS (0);
		glBindTexture (GL_TEXTURE_2D, res->gl_handle);
		SEHLE_CHECK_ERRORS (0);
		unsigned int internalformat = sehle_texture_get_gl_internal_format (&tex2d->texture);
		if (!tex2d->n_mipmaps) {
			tex2d->n_mipmaps = (tex2d->min_filter == SEHLE_TEXTURE_FILTER_MIPMAP) ? sehle_texture_get_mipmap_levels (tex2d->width, tex2d->height) : 1;
		}
		glTexStorage2D (GL_TEXTURE_2D, tex2d->n_mipmaps, internalformat, tex2d->width, tex2d->height);
		SEHLE_CHECK_ERRORS (0);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex2d->n_mipmaps - 1);
		SEHLE_CHECK_ERRORS (0);
	} else {
		SEHLE_CHECK_ERRORS (0);
		glBindTexture (GL_TEXTURE_2D, res->gl_handle);
		SEHLE_CHECK_ERRORS (0);
	}
	if (tex2d->pixels) {
		unsigned int format = sehle_texture_get_gl_data_format (&tex2d->texture);
		glPixelStoref (GL_UNPACK_ALIGNMENT, 4);
		SEHLE_CHECK_ERRORS (0);
		glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, tex2d->width, tex2d->height, format, GL_UNSIGNED_BYTE, tex2d->pixels);
		SEHLE_CHECK_ERRORS (0);
		if (tex2d->gen_mipmaps) {
			glGenerateMipmap (GL_TEXTURE_2D);
			SEHLE_CHECK_ERRORS (0);
		}
	}
	if (tex2d->clamp) {
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		static const float border = 0;
		glTexParameterfv (GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &border);
	} else {
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	SEHLE_CHECK_ERRORS (0);

	static const unsigned int filters[] = { GL_NEAREST, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR };
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filters[tex2d->min_filter]);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filters[tex2d->mag_filter]);
	SEHLE_CHECK_ERRORS (0);
	sehle_resource_set_sate (res, SEHLE_RESOURCE_STATE_READY);
}

static void
texture_2d_bind (SehleTexture *tex, unsigned int channel)
{
	if (!sehle_resource_is_initialized (&tex->resource)) return;
	if (sehle_resource_is_modified (&tex->resource)) {
		sehle_resource_build (&tex->resource);
	}
	if (sehle_resource_is_invalid (&tex->resource)) return;
	glActiveTexture (GL_TEXTURE0 + channel);
	glBindTexture (GL_TEXTURE_2D, tex->resource.gl_handle);
	SEHLE_CHECK_ERRORS (0);
#ifdef SEHLE_PERFORMANCE_MONITOR
	tex->resource.engine->counter.texture_binds += 1;
#endif
}

void
sehle_texture_2d_bind_shadow (SehleTexture2D *tex2d, unsigned int channel)
{
	SehleTexture *tex = SEHLE_TEXTURE (tex2d);
	arikkei_return_if_fail (tex2d != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_2D (tex2d));
	arikkei_return_if_fail (sehle_resource_is_initialized (&tex2d->texture.resource));
	arikkei_return_if_fail (!sehle_resource_is_invalid (&tex2d->texture.resource));
	arikkei_return_if_fail (tex2d->texture.resource.engine->running);
	sehle_texture_bind (tex, channel);
	if (sehle_resource_is_invalid (&tex->resource)) return;
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
}

void
sehle_texture_2d_set_size (SehleTexture2D *tex2d, unsigned int width, unsigned int height)
{
	arikkei_return_if_fail (tex2d != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_2D (tex2d));
	arikkei_return_if_fail (!SEHLE_TEXTURE_IS_MAPPED (tex2d));
	if ((width != tex2d->width) || (height != tex2d->height)) {
		tex2d->width = width;
		tex2d->height = height;
		tex2d->n_mipmaps = 0;
		if (tex2d->pixels) {
			free (tex2d->pixels);
			tex2d->pixels = NULL;
			tex2d->rowstride = 0;
		}
		if (tex2d->texture.resource.gl_handle) {
			glDeleteTextures (1, &tex2d->texture.resource.gl_handle);
			tex2d->texture.resource.gl_handle = 0;
		}
		sehle_resource_set_sate (&tex2d->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	}
}

void
sehle_texture_2d_set_format (SehleTexture2D *tex2d, unsigned int format, unsigned int channel_type, unsigned int color_space)
{
	arikkei_return_if_fail (tex2d != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_2D (tex2d));
	arikkei_return_if_fail (!SEHLE_TEXTURE_IS_MAPPED (tex2d));
	if ((format != tex2d->texture.format) || (channel_type != tex2d->texture.chtype) || (color_space != tex2d->texture.color_space)) {
		tex2d->texture.format = format;
		tex2d->texture.chtype = channel_type;
		tex2d->texture.color_space = color_space;
		if (tex2d->pixels) {
			free (tex2d->pixels);
			tex2d->pixels = NULL;
			tex2d->rowstride = 0;
		}
		if (tex2d->texture.resource.gl_handle) {
			glDeleteTextures (1, &tex2d->texture.resource.gl_handle);
			tex2d->texture.resource.gl_handle = 0;
		}
		sehle_resource_set_sate (&tex2d->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	}
}

void
sehle_texture_2d_set_filter (SehleTexture2D *tex2d, unsigned int min_filter, unsigned int mag_filter)
{
	arikkei_return_if_fail (tex2d != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_2D (tex2d));
	arikkei_return_if_fail (!SEHLE_TEXTURE_IS_MAPPED (tex2d));
	if ((min_filter != tex2d->min_filter) || (mag_filter != tex2d->mag_filter)) {
		if ((min_filter != tex2d->min_filter) && ((min_filter == SEHLE_TEXTURE_FILTER_MIPMAP) || (tex2d->min_filter == SEHLE_TEXTURE_FILTER_MIPMAP))) {
			/* Mipmapping changes, need to refresh storage */
			if (tex2d->pixels) {
				free (tex2d->pixels);
				tex2d->pixels = NULL;
				tex2d->rowstride = 0;
			}
			if (tex2d->texture.resource.gl_handle) {
				glDeleteTextures (1, &tex2d->texture.resource.gl_handle);
				tex2d->texture.resource.gl_handle = 0;
			}
		}
		tex2d->min_filter = min_filter;
		tex2d->mag_filter = mag_filter;
		sehle_resource_set_sate (&tex2d->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	}
}

void
sehle_texture_2d_set_mipmaps (SehleTexture2D *tex2d, unsigned int n_levels, unsigned int generate)
{
	arikkei_return_if_fail (tex2d != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_2D (tex2d));
	arikkei_return_if_fail (!SEHLE_TEXTURE_IS_MAPPED (tex2d));
	if ((n_levels != tex2d->n_mipmaps) || (generate != tex2d->gen_mipmaps)) {
		tex2d->n_mipmaps = n_levels;
		tex2d->gen_mipmaps = generate;
		if (tex2d->texture.resource.gl_handle) {
			glDeleteTextures (1, &tex2d->texture.resource.gl_handle);
			tex2d->texture.resource.gl_handle = 0;
		}
		sehle_resource_set_sate (&tex2d->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	}
}

void
sehle_texture_2d_set_mapping (SehleTexture2D *tex2d, unsigned int clamp)
{
	tex2d->clamp = clamp;
	sehle_resource_set_sate (&tex2d->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
}

/* Get backing store bpp */

static unsigned int
texture_2d_get_bpp (SehleTexture2D *tex2d)
{
	static const unsigned int widths[] = { 1, 2, 2, 4, 4, 4, 4 };
	if (tex2d->texture.format <= SEHLE_TEXTURE_RGBA) return (tex2d->texture.format + 1) * widths[tex2d->texture.chtype];
	return 4;
}

static void
texture_2d_ensure_backing_store (SehleTexture2D *tex2d)
{
	if (!tex2d->pixels) {
		unsigned int bpp = texture_2d_get_bpp (tex2d);
		tex2d->rowstride = (tex2d->width * bpp + 3) & 0xfffffffc;
		tex2d->pixels = (unsigned char *) realloc (tex2d->pixels, tex2d->height * tex2d->rowstride);
	}
}

unsigned int
sehle_texture_2d_map (SehleTexture2D *tex2d, NRPixBlock *pxb, unsigned int mapping_mode)
{
	arikkei_return_val_if_fail (tex2d != NULL, 0);
	arikkei_return_val_if_fail (SEHLE_IS_TEXTURE_2D (tex2d), 0);
	arikkei_return_val_if_fail (pxb != NULL, 0);
	arikkei_return_val_if_fail (!SEHLE_TEXTURE_IS_MAPPED(tex2d), 0);
	arikkei_return_val_if_fail (tex2d->texture.resource.engine->running, 0);
	if (mapping_mode & SEHLE_TEXTURE_READ) {
		if (sehle_resource_is_modified (&tex2d->texture.resource)) sehle_resource_build (&tex2d->texture.resource);
		arikkei_return_val_if_fail (sehle_resource_is_ready (&tex2d->texture.resource), 0);
		if (debug) fprintf (stderr, "Texture::map: Mapping READ %s...", tex2d->texture.resource.id);
		glBindTexture (GL_TEXTURE_2D, tex2d->texture.resource.gl_handle);
		tex2d->texture.resource.state |= mapping_mode;
		texture_2d_ensure_backing_store (tex2d);
		glGetTexImage (GL_TEXTURE_2D, 0, sehle_texture_get_gl_data_format (&tex2d->texture), GL_UNSIGNED_BYTE, tex2d->pixels);
		SEHLE_CHECK_ERRORS (0);
		unsigned int nrmode = sehle_texture_format_to_nr (tex2d->texture.format);
		nr_pixblock_setup_extern (pxb, nrmode, 0, 0, tex2d->width, tex2d->height, tex2d->pixels, tex2d->rowstride, 0, 0);
		if (debug) fprintf (stderr, " finished\n");
		return 1;
	} else if (mapping_mode & SEHLE_TEXTURE_WRITE) {
		tex2d->texture.resource.state |= mapping_mode;
		texture_2d_ensure_backing_store (tex2d);
		unsigned int nrmode = sehle_texture_format_to_nr (tex2d->texture.format);
		nr_pixblock_setup_extern (pxb, nrmode, 0, 0, tex2d->width, tex2d->height, tex2d->pixels, tex2d->rowstride, 1, 0);
		sehle_resource_set_sate (&tex2d->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
		return 1;
	}
	return 1;
}

unsigned int
sehle_texture_2d_unmap (SehleTexture2D *tex2d)
{
	arikkei_return_val_if_fail (tex2d != NULL, 0);
	arikkei_return_val_if_fail (SEHLE_IS_TEXTURE_2D (tex2d), 0);
	arikkei_return_val_if_fail (SEHLE_TEXTURE_IS_MAPPED (tex2d), 0);
	if (tex2d->texture.resource.state & SEHLE_TEXTURE_WRITE) {
		sehle_resource_build (&tex2d->texture.resource);
		if (tex2d->gen_mipmaps) {
			glGenerateMipmap (GL_TEXTURE_2D);
			SEHLE_CHECK_ERRORS (0);
		}
	}
	tex2d->texture.resource.state &= ~SEHLE_TEXTURE_READ_WRITE;
	return 1;
}

void
sehle_texture_2d_set_pixels_from_pixblock (SehleTexture2D *tex2d, const NRPixBlock *pxb)
{
	arikkei_return_if_fail (tex2d != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_2D (tex2d));
	arikkei_return_if_fail (pxb != NULL);
	arikkei_return_if_fail (!SEHLE_TEXTURE_IS_MAPPED (tex2d));
	arikkei_return_if_fail (tex2d->texture.resource.engine->running);
	sehle_texture_2d_set_size (tex2d, pxb->area.x1 - pxb->area.x0, pxb->area.y1 - pxb->area.y0);
	unsigned int new_format = sehle_texture_get_format_from_pixblock (pxb);
	unsigned int new_channel_type = sehle_texture_get_channel_type_from_pixblock (pxb);
	unsigned int new_color_space = sehle_texture_get_color_space_from_pixblock (pxb);
	sehle_texture_2d_set_format (tex2d, new_format, new_channel_type, new_color_space);
	if (sehle_resource_is_modified (&tex2d->texture.resource)) {
		sehle_resource_build (&tex2d->texture.resource);
		arikkei_return_if_fail (sehle_resource_is_ready (&tex2d->texture.resource));
	} else {
		arikkei_return_if_fail (sehle_resource_is_ready (&tex2d->texture.resource));
		glBindTexture (GL_TEXTURE_2D, tex2d->texture.resource.gl_handle);
		SEHLE_CHECK_ERRORS (0);
	}
	NRPixBlock cpx;
	nr_pixblock_clone_packed (&cpx, pxb);
	unsigned int format = sehle_texture_get_gl_source_format_from_pixblock (&cpx);
	unsigned int type = sehle_texture_get_gl_source_type_from_pixblock (&cpx);
	glPixelStoref (GL_UNPACK_ALIGNMENT, 4);
	SEHLE_CHECK_ERRORS (0);
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, tex2d->width, tex2d->height, format, type, cpx.px);
	SEHLE_CHECK_ERRORS (0);
	nr_pixblock_release (&cpx);
	if (tex2d->gen_mipmaps) {
		glGenerateMipmap (GL_TEXTURE_2D);
		SEHLE_CHECK_ERRORS (0);
	}
}

//static int
//get_gl_compressed_format_from_format (unsigned int format)
//{
//
//}

void
sehle_texture_2d_set_pixels_from_image (SehleTexture2D *tex2d, const SehleTextureData *data, SehleTextureImage *img)
{
	arikkei_return_if_fail (tex2d != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_2D (tex2d));
	arikkei_return_if_fail (!SEHLE_TEXTURE_IS_MAPPED (tex2d));
	arikkei_return_if_fail (tex2d->texture.resource.engine->running);
	sehle_texture_2d_set_format (tex2d, data->format, data->ch_type, data->color_space);
	sehle_texture_2d_set_filter (tex2d, data->min_filter, data->mag_filter);
	sehle_texture_2d_set_size (tex2d, img->width, img->height);
	unsigned int n_mipmap_images = 1;
	if (data->min_filter == SEHLE_TEXTURE_FILTER_MIPMAP) {
		if (data->generate_mipmaps) {
			sehle_texture_2d_set_mipmaps (tex2d, 0, 1);
		} else {
			sehle_texture_2d_set_mipmaps (tex2d, img->n_mipmaps, 0);
			n_mipmap_images = img->n_mipmaps;
		}
	} else {
		sehle_texture_2d_set_mipmaps (tex2d, 1, 0);
	}
	sehle_resource_set_sate (&tex2d->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	sehle_resource_build (&tex2d->texture.resource);
	unsigned int mwidth = img->width;
	unsigned int mheight = img->height;
	for (unsigned int i = 0; i < n_mipmap_images; i++) {
		if (img->compressed) {
			int internalformat = 0;
			unsigned int blockSize = 16;
			switch (data->format) {
			case SEHLE_TEXTURE_FORMAT_BC1:
				blockSize = 8;
				internalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				break;
			case SEHLE_TEXTURE_FORMAT_BC2:
				internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				break;
			case SEHLE_TEXTURE_FORMAT_BC3:
				internalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;
			default:
				/* Not supported at moment */
				break;
			}
			unsigned int size = ((mwidth + 3) / 4) * ((mheight + 3) / 4) * blockSize;
			glCompressedTexSubImage2D (GL_TEXTURE_2D, i, 0, 0, mwidth, mheight, internalformat, size, img->images[i]);
			GLint result = 0;
			glGetTexLevelParameteriv (GL_TEXTURE_2D, i, GL_TEXTURE_COMPRESSED_ARB, &result);
			if (!result) {
				fprintf (stderr, "Texture2D::setPixels: Mipmap level %d failed\n", i);
			}
		} else {
			unsigned int format = sehle_texture_get_gl_data_format_from_format (data->format);
			unsigned int type = sehle_texture_get_gl_data_type_from_type (data->ch_type);
			glTexSubImage2D (GL_TEXTURE_2D, i, 0, 0, mwidth, mheight, format, type, img->images[i]);
		}
		if (mwidth > 1) mwidth /= 2;
		if (mheight > 1) mheight /= 2;
	}
	if (tex2d->gen_mipmaps) {
		glGenerateMipmap (GL_TEXTURE_2D);
		SEHLE_CHECK_ERRORS (0);
	}
}

SehleTexture2D *
sehle_texture_2d_new (SehleEngine *engine, const unsigned char *id)
{
	if (id) {
		SehleTexture2D *tex2d = (SehleTexture2D *) sehle_engine_lookup_resource (engine, SEHLE_TYPE_TEXTURE_2D, id);
		if (tex2d) {
			if (debug > 1) fprintf (stderr, "Texture2D::getTexture2D: Got existing 2D texture %s\n", tex2d->texture.resource.id);
			az_object_ref (AZ_OBJECT(tex2d));
			return tex2d;
		}
	}
	SehleTexture2D *tex2d = (SehleTexture2D *) az_object_new (SEHLE_TYPE_TEXTURE_2D);
	sehle_texture_setup (&tex2d->texture, engine, id);
	return tex2d;
}


