#define __SEHLE_TEXTURE_CUBEMAP_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2012
//

static const unsigned int debug = 0;

#include <stdlib.h>
#include <stdio.h>

#include "GL/glew.h"

#include <az/extend.h>

#include <sehle/engine.h>

#include <sehle/texture-cubemap.h>

static void texture_cubemap_class_init (SehleTextureCubeMapClass *klass);
static void texture_cubemap_init (SehleTextureCubeMapClass *klass, SehleTextureCubeMap *cmap);
static void texture_cubemap_finalize (SehleTextureCubeMapClass *klass, SehleTextureCubeMap *cmap);

static void texture_cubemap_build (SehleResource *res);
static void texture_cubemap_bind (SehleTexture *tex, unsigned int channel);

static const GLenum targets[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

unsigned int
sehle_texture_cube_map_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const unsigned char *) "SehleTextureCubeMap", SEHLE_TYPE_TEXTURE, sizeof (SehleTextureCubeMapClass), sizeof (SehleTextureCubeMap), 0,
			(void (*) (AZClass *)) texture_cubemap_class_init,
			(void (*) (const AZImplementation *, void *)) texture_cubemap_init,
			(void (*) (const AZImplementation *, void *)) texture_cubemap_finalize);
	}
	return type;
}

static void
texture_cubemap_class_init (SehleTextureCubeMapClass *klass)
{
	klass->texture_class.resource_class.build = texture_cubemap_build;
	((SehleTextureClass *) klass)->bind = texture_cubemap_bind;
}

static void
texture_cubemap_init (SehleTextureCubeMapClass *klass, SehleTextureCubeMap *cmap)
{
	cmap->min_filter = SEHLE_TEXTURE_FILTER_MIPMAP;
	cmap->mag_filter = SEHLE_TEXTURE_FILTER_LINEAR;
	cmap->generate_mipmaps = 1;
}

static void
texture_cubemap_finalize (SehleTextureCubeMapClass *klass, SehleTextureCubeMap *cmap)
{
}

static void
texture_cubemap_build (SehleResource *res)
{
	SehleTextureCubeMap *cmap = (SehleTextureCubeMap *) res;
	if (!res->gl_handle) {
		glGenTextures (1, &res->gl_handle);
		SEHLE_CHECK_ERRORS (0);
		glBindTexture (GL_TEXTURE_CUBE_MAP, res->gl_handle);
		SEHLE_CHECK_ERRORS (0);
		unsigned int internalformat = sehle_texture_get_gl_internal_format (&cmap->texture);
		if (!cmap->n_mipmaps) {
			cmap->n_mipmaps = (cmap->min_filter == SEHLE_TEXTURE_FILTER_MIPMAP) ? sehle_texture_get_mipmap_levels (cmap->width, cmap->height) : 1;
		}
		glTexStorage2D (GL_TEXTURE_CUBE_MAP, cmap->n_mipmaps, internalformat, cmap->width, cmap->height);
		SEHLE_CHECK_ERRORS (0);
		glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, cmap->n_mipmaps - 1);
		SEHLE_CHECK_ERRORS (0);
	} else {
		glBindTexture (GL_TEXTURE_CUBE_MAP, res->gl_handle);
		SEHLE_CHECK_ERRORS (0);
	}
	static const unsigned int filters[] = { GL_NEAREST, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR };
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, filters[cmap->min_filter]);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, filters[cmap->mag_filter]);
	SEHLE_CHECK_ERRORS (0);
	sehle_resource_set_sate (res, SEHLE_RESOURCE_STATE_READY);
#if 0
	// Get free texture handle
	if (!res->gl_handle) glGenTextures (1, &res->gl_handle);
	int format = sehle_texture_get_gl_data_format (tex);
	int internalformat = sehle_texture_get_gl_internal_format (tex);
	// Bind texture
	glBindTexture (GL_TEXTURE_CUBE_MAP, res->gl_handle);
	// Set up image
	static const GLenum targets[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };
	for (int i = 0; i < 6; i++) {
		// fixme: I do not like the copying, but what to do if rs != width?
		unsigned int width = cmap->images[i]->pixels.area.x1 - cmap->images[i]->pixels.area.x0;
		unsigned int height = cmap->images[i]->pixels.area.y1 - cmap->images[i]->pixels.area.y0;
		unsigned int bpp = cmap->images[i]->pixels.n_channels;
		unsigned int rowstride = (width * bpp + 3) & 0xfffffffc;
		unsigned char *pixels = (unsigned char *) malloc (height * rowstride);
		for (unsigned int y = 0; y < height; y++) {
			unsigned char *d = pixels + y * rowstride;
			const unsigned char *s = NR_PIXBLOCK_ROW (&cmap->images[i]->pixels, y);
			memcpy (d, s, width * bpp);
		}
		glTexImage2D (targets[i], 0, internalformat, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
		free (pixels);
	}
	glGenerateMipmap (GL_TEXTURE_CUBE_MAP);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (debug) fprintf (stderr, " finished\n");
	sehle_resource_set_sate (res, SEHLE_RESOURCE_STATE_READY);
#endif
}

static void
texture_cubemap_bind (SehleTexture *tex, unsigned int channel)
{
	if (!sehle_resource_is_initialized (&tex->resource)) return;
	if (sehle_resource_is_modified (&tex->resource)) {
		sehle_resource_build (&tex->resource);
	}
	if (sehle_resource_is_invalid (&tex->resource)) return;
	glActiveTexture (GL_TEXTURE0 + channel);
	glBindTexture (GL_TEXTURE_CUBE_MAP, tex->resource.gl_handle);
	SEHLE_CHECK_ERRORS (0);
#ifdef SEHLE_PERFORMANCE_MONITOR
	tex->resource.engine->counter.texture_binds += 1;
#endif
}

void
sehle_texture_cube_map_set_size (SehleTextureCubeMap *cmap, unsigned int width, unsigned int height)
{
	arikkei_return_if_fail (cmap != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_CUBEMAP (cmap));
	if ((width != cmap->width) || (height != cmap->height)) {
		cmap->width = width;
		cmap->height = height;
		cmap->n_mipmaps = 0;
		if (cmap->texture.resource.gl_handle) {
			glDeleteTextures (1, &cmap->texture.resource.gl_handle);
			cmap->texture.resource.gl_handle = 0;
		}
		sehle_resource_set_sate (&cmap->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	}
}

void
sehle_texture_cube_map_set_format (SehleTextureCubeMap *cmap, unsigned int format, unsigned int channel_type, unsigned int color_space)
{
	arikkei_return_if_fail (cmap != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_CUBEMAP (cmap));
	if ((format != cmap->texture.format) || (channel_type != cmap->texture.chtype) || (color_space != cmap->texture.color_space)) {
		cmap->texture.format = format;
		cmap->texture.chtype = channel_type;
		cmap->texture.color_space = color_space;
		if (cmap->texture.resource.gl_handle) {
			glDeleteTextures (1, &cmap->texture.resource.gl_handle);
			cmap->texture.resource.gl_handle = 0;
		}
		sehle_resource_set_sate (&cmap->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	}
}

void
sehle_texture_cube_map_set_filter (SehleTextureCubeMap *cmap, unsigned int min_filter, unsigned int mag_filter)
{
	arikkei_return_if_fail (cmap != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_CUBEMAP (cmap));
	if ((min_filter != cmap->min_filter) || (mag_filter != cmap->mag_filter)) {
		if ((min_filter != cmap->min_filter) && ((min_filter == SEHLE_TEXTURE_FILTER_MIPMAP) || (cmap->min_filter == SEHLE_TEXTURE_FILTER_MIPMAP))) {
			/* Mipmapping chges, need to refresh storage */
			if (cmap->texture.resource.gl_handle) {
				glDeleteTextures (1, &cmap->texture.resource.gl_handle);
				cmap->texture.resource.gl_handle = 0;
			}
		}
		cmap->min_filter = min_filter;
		cmap->mag_filter = mag_filter;
		sehle_resource_set_sate (&cmap->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	}
}

void
sehle_texture_cube_map_set_mipmaps (SehleTextureCubeMap *cmap, unsigned int n_levels, unsigned int generate)
{
	arikkei_return_if_fail (cmap != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_CUBEMAP (cmap));
	if ((n_levels != cmap->n_mipmaps) || (generate != cmap->generate_mipmaps)) {
		cmap->n_mipmaps = n_levels;
		cmap->generate_mipmaps = generate;
		if (cmap->texture.resource.gl_handle) {
			glDeleteTextures (1, &cmap->texture.resource.gl_handle);
			cmap->texture.resource.gl_handle = 0;
		}
		sehle_resource_set_sate (&cmap->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	}
}


void
sehle_texture_cube_map_set_pixels (SehleTextureCubeMap *cmap, const SehleTextureData *data, SehleTextureImage *img)
{
	unsigned int i;
	arikkei_return_if_fail (cmap != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_CUBEMAP (cmap));
	if (!cmap->texture.resource.engine->running) return;
	sehle_texture_cube_map_set_size (cmap, img->width, img->height);
	sehle_texture_cube_map_set_format (cmap, data->format, data->ch_type, data->color_space);
	sehle_texture_cube_map_set_filter (cmap, data->min_filter, data->mag_filter);
	unsigned int n_mipmap_images = 1;
	if (data->min_filter == SEHLE_TEXTURE_FILTER_MIPMAP) {
		if (data->generate_mipmaps) {
			sehle_texture_cube_map_set_mipmaps (cmap, 0, 1);
		} else {
			sehle_texture_cube_map_set_mipmaps (cmap, img->n_mipmaps, 0);
			n_mipmap_images = img->n_mipmaps;
		}
	} else {
		sehle_texture_cube_map_set_mipmaps (cmap, 1, 0);
	}
	sehle_resource_set_sate (&cmap->texture.resource, SEHLE_RESOURCE_STATE_MODIFIED);
	sehle_resource_build (&cmap->texture.resource);
	for (i = 0; i < 6; i++) {
		unsigned int mwidth = img->width;
		unsigned int mheight = img->height;
		for (unsigned int j = 0; j < n_mipmap_images; j++) {
			unsigned int format = sehle_texture_get_gl_data_format_from_format (data->format);
			unsigned int type = sehle_texture_get_gl_data_type_from_type (data->ch_type);
			glTexSubImage2D (targets[i], j, 0, 0, mwidth, mheight, format, type, img->images[i * img->n_mipmaps + j]);
			SEHLE_CHECK_ERRORS (0);
			if (mwidth > 1) mwidth /= 2;
			if (mheight > 1) mheight /= 2;
		}
	}
	if (cmap->generate_mipmaps) {
		glGenerateMipmap (GL_TEXTURE_CUBE_MAP);
		SEHLE_CHECK_ERRORS (0);
	}
}

void
sehle_texture_cube_map_set_pixels_from_pixblocks (SehleTextureCubeMap *cmap, const NRPixBlock *pxb)
{
	NRPixBlock cpx;
	unsigned int i;
	arikkei_return_if_fail (cmap != NULL);
	arikkei_return_if_fail (SEHLE_IS_TEXTURE_CUBEMAP (cmap));
	arikkei_return_if_fail (pxb != NULL);
	arikkei_return_if_fail (cmap->texture.resource.engine->running);
	sehle_texture_cube_map_set_size (cmap, pxb->area.x1 - pxb->area.x0, pxb->area.y1 - pxb->area.y0);
	unsigned int new_format = sehle_texture_get_format_from_pixblock (pxb);
	unsigned int new_channel_type = sehle_texture_get_channel_type_from_pixblock (pxb);
	unsigned int new_color_space = sehle_texture_get_color_space_from_pixblock (pxb);
	sehle_texture_cube_map_set_format (cmap, new_format, new_channel_type, new_color_space);
	if (sehle_resource_is_modified (&cmap->texture.resource)) {
		sehle_resource_build (&cmap->texture.resource);
		arikkei_return_if_fail (sehle_resource_is_ready (&cmap->texture.resource));
	} else {
		arikkei_return_if_fail (sehle_resource_is_ready (&cmap->texture.resource));
		glBindTexture (GL_TEXTURE_CUBE_MAP, cmap->texture.resource.gl_handle);
		SEHLE_CHECK_ERRORS (0);
	}
	for (i = 0; i < 6; i++) {
		nr_pixblock_clone_packed (&cpx, pxb);
		unsigned int format = sehle_texture_get_gl_source_format_from_pixblock (&cpx);
		unsigned int type = sehle_texture_get_gl_source_type_from_pixblock (&cpx);
		glPixelStoref (GL_UNPACK_ALIGNMENT, 4);
		SEHLE_CHECK_ERRORS (0);
		glTexSubImage2D (targets[i], 0, 0, 0, cmap->width, cmap->height, format, type, cpx.px);
		SEHLE_CHECK_ERRORS (0);
		nr_pixblock_release (&cpx);
	}
	if (cmap->generate_mipmaps) {
		glGenerateMipmap (GL_TEXTURE_CUBE_MAP);
		SEHLE_CHECK_ERRORS (0);
	}
}

SehleTextureCubeMap *
sehle_texture_cube_map_new (SehleEngine *engine, const unsigned char *id)
{
	if (id) {
		SehleTextureCubeMap *cmap = (SehleTextureCubeMap *) sehle_engine_lookup_resource (engine, SEHLE_TYPE_TEXTURE_CUBEMAP, id);
		if (cmap) {
			if (debug > 1) fprintf (stderr, "TextureCubemap::getTextureCubemap: Got existing 2D texture %s\n", cmap->texture.resource.id);
			az_object_ref (AZ_OBJECT (cmap));
			return cmap;
		}
	}
	SehleTextureCubeMap *cmap = (SehleTextureCubeMap *) az_object_new (SEHLE_TYPE_TEXTURE_CUBEMAP);
	sehle_texture_setup (&cmap->texture, engine, id);
	return cmap;
}

