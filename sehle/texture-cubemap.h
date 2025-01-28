#ifndef __SEHLE_TEXTURE_CUBEMAP_H__
#define __SEHLE_TEXTURE_CUBEMAP_H__

/*
* Libsehle
*
* Copyright (C) Lauris Kaplinski 2007-2015
*/

typedef struct _SehleTextureCubeMap SehleTextureCubeMap;
typedef struct _SehleTextureCubeMapClass SehleTextureCubeMapClass;

#define SEHLE_TYPE_TEXTURE_CUBEMAP (sehle_texture_cube_map_get_type ())
#define SEHLE_TEXTURE_CUBEMAP(p) (AZ_CHECK_INSTANCE_CAST ((p), SEHLE_TYPE_TEXTURE_CUBEMAP, SehleTextureCubeMap))
#define SEHLE_IS_TEXTURE_CUBEMAP(p) (AZ_CHECK_INSTANCE_TYPE ((p), SEHLE_TYPE_TEXTURE_CUBEMAP))

#include <sehle/texture.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleTextureCubeMap {
	SehleTexture texture;

	unsigned int width;
	unsigned int height;

	/* Filtering */
	unsigned int min_filter : 2;
	unsigned int mag_filter : 1;
	/* Mipmaps */
	unsigned int n_mipmaps : 6;
	unsigned int generate_mipmaps : 1;
};

struct _SehleTextureCubeMapClass {
	SehleTextureClass texture_class;
};

unsigned int sehle_texture_cube_map_get_type (void);

void sehle_texture_cube_map_set_size (SehleTextureCubeMap *cmap, unsigned int width, unsigned int height);
void sehle_texture_cube_map_set_format (SehleTextureCubeMap *cmap, unsigned int format, unsigned int channel_type, unsigned int color_space);
void sehle_texture_cube_map_set_filter (SehleTextureCubeMap *cmap, unsigned int min_filter, unsigned int mag_filter);
void sehle_texture_cube_map_set_mipmaps (SehleTextureCubeMap *cmap, unsigned int n_levels, unsigned int generate);

SehleTextureCubeMap *sehle_texture_cube_map_new (SehleEngine *engine, const unsigned char *id);

void sehle_texture_cube_map_set_pixels (SehleTextureCubeMap *cmap, const SehleTextureData *data, SehleTextureImage *img);
void sehle_texture_cube_map_set_pixels_from_pixblocks (SehleTextureCubeMap *cmap, const NRPixBlock *pxb);

#ifdef __cplusplus
};
#endif

#endif

