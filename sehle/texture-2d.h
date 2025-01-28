#ifndef __SEHLE_TEXTURE_2D_H__
#define __SEHLE_TEXTURE_2D_H__

/*
* Libsehle
*
* Copyright (C) Lauris Kaplinski 2007-2015
*/

typedef struct _SehleTexture2D SehleTexture2D;
typedef struct _SehleTexture2DClass SehleTexture2DClass;

#define SEHLE_TYPE_TEXTURE_2D (sehle_texture_2d_get_type ())
#define SEHLE_TEXTURE_2D(p) (AZ_CHECK_INSTANCE_CAST ((p), SEHLE_TYPE_TEXTURE_2D, SehleTexture2D))
#define SEHLE_IS_TEXTURE_2D(p) (AZ_CHECK_INSTANCE_TYPE ((p), SEHLE_TYPE_TEXTURE_2D))

#include <sehle/texture.h>

/*
* Transfer formats
*
* U8 U8
* I8 I8
* U16 U16
* F16 F32
* F32 F32
* 
*/

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleTexture2D {
	SehleTexture texture;

	unsigned int width;
	unsigned int height;

	/* Filtering */
	unsigned int min_filter : 2;
	unsigned int mag_filter : 1;
	/* Mipmaps */
	unsigned int n_mipmaps : 6;
	unsigned int gen_mipmaps : 1;
	/* Clamp or repeat */
	unsigned int clamp : 1;

	/* Mapping data */
	unsigned char *pixels;
	unsigned int rowstride;
};

struct _SehleTexture2DClass {
	SehleTextureClass texture_class;
};

unsigned int sehle_texture_2d_get_type ();

SehleTexture2D *sehle_texture_2d_new (SehleEngine *engine, const unsigned char *id);

/* Bind texture for shadow lookups (compare R mode) */
void sehle_texture_2d_bind_shadow (SehleTexture2D *tex, unsigned int channel);

/* State management */
/* If state is changed, pixels will be invalidated */
void sehle_texture_2d_set_size (SehleTexture2D *tex2d, unsigned int width, unsigned int height);
void sehle_texture_2d_set_format (SehleTexture2D *tex2d, unsigned int format, unsigned int channel_type, unsigned int color_space);
void sehle_texture_2d_set_mipmaps (SehleTexture2D *tex2d, unsigned int n_levels, unsigned int generate);
/* If mipmapping is changed, pixels will be invalidated */
void sehle_texture_2d_set_filter (SehleTexture2D *tex2d, unsigned int min_filter, unsigned int mag_filter);
/* Keeps pixels */
void sehle_texture_2d_set_mapping (SehleTexture2D *tex2d, unsigned int clamp);

/* Update */
void sehle_texture_2d_set_pixels_from_pixblock (SehleTexture2D *tex2d, const NRPixBlock *pxb);
void sehle_texture_2d_set_pixels_from_image (SehleTexture2D *tex2d, const SehleTextureData *data, SehleTextureImage *img);

/**
 * @brief map texture image to memory
 * 
 * Map texture image into memory and set up NRPixBlock structure describing its layout. The destination pixblock has to be
 * uninitialized or there will be memory leak. The caller should to release the pixblock it after unmapping.
 * @param tex2d a texture object
 * @param pxb an uninitialized pixblock recieving pixel data
 * @param mapping_mode SEHLE_BUFFER_READ or SEHLE_BUFFER_WRITE
 * @return true if successful
 */
unsigned int sehle_texture_2d_map (SehleTexture2D *tex2d, NRPixBlock *pxb, unsigned int mapping_mode);
/* Release mapping and update texture if needed */
/**
 * @brief Release memory mapping and update texture is needed
 * 
 * The pixblock should be released after unmapping texture
 * @param tex2d a texture object
 * @return true if successful
 */
unsigned int sehle_texture_2d_unmap (SehleTexture2D *tex2d);

#ifdef __cplusplus
};
#endif

#endif

