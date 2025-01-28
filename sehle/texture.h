#ifndef __SEHLE_TEXTURE_H__
#define __SEHLE_TEXTURE_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2015
 */

typedef struct _SehleTexture SehleTexture;
typedef struct _SehleTextureClass SehleTextureClass;
typedef struct _SehleTextureData SehleTextureData;
typedef struct _SehleTextureImage SehleTextureImage;

#define SEHLE_TYPE_TEXTURE (sehle_texture_get_type ())
#define SEHLE_TEXTURE(p) (AZ_CHECK_INSTANCE_CAST ((p), SEHLE_TYPE_TEXTURE, SehleTexture))
#define SEHLE_IS_TEXTURE(p) (AZ_CHECK_INSTANCE_TYPE ((p), SEHLE_TYPE_TEXTURE))

#include <nr/pixblock.h>

#include <sehle/resource.h>

/* Pixels are saved as private array */
#define SEHLE_TEXTURE_PRIVATE_STORAGE 4
/* Mapping modes */
#define SEHLE_TEXTURE_READ 8
#define SEHLE_TEXTURE_WRITE 16
#define SEHLE_TEXTURE_READ_WRITE (SEHLE_TEXTURE_READ | SEHLE_TEXTURE_WRITE)
#define SEHLE_TEXTURE_IS_MAPPED(t) ((((SehleTexture *) (t))->resource.state & SEHLE_TEXTURE_READ_WRITE) != 0)

/* Formats */
#define SEHLE_TEXTURE_R 0
#define SEHLE_TEXTURE_RG 1
#define SEHLE_TEXTURE_RGB 2
#define SEHLE_TEXTURE_RGBA 3
#define SEHLE_TEXTURE_DEPTH 4
#define SEHLE_TEXTURE_DEPTH_STENCIL 5
#define SEHLE_TEXTURE_RG_COMPRESSED 6
#define SEHLE_TEXTURE_RGB_COMPRESSED 7
#define SEHLE_TEXTURE_RGBA_COMPRESSED 8
#define SEHLE_TEXTURE_RGBA1_COMPRESSED 9
#define SEHLE_TEXTURE_FORMAT_BC1 10
#define SEHLE_TEXTURE_FORMAT_BC2 11
#define SEHLE_TEXTURE_FORMAT_BC3 12

/* Channel types */
#define SEHLE_TEXTURE_U8 0
#define SEHLE_TEXTURE_I8 1
#define SEHLE_TEXTURE_U16 2
#define SEHLE_TEXTURE_F16 3
#define SEHLE_TEXTURE_F32 4
#define SEHLE_TEXTURE_D16 5
#define SEHLE_TEXTURE_D24 6
#define SEHLE_TEXTURE_D32 7

/* Color space */
#define SEHLE_TEXTURE_COLOR_SPACE_LINEAR 0
#define SEHLE_TEXTURE_COLOR_SPACE_SRGB 1

/*
 * Allowed combinations
 *
 * Linear R    U8, I8, U16, F16, F32
 * Linear RG   U8, I8, U16, F16, F32
 * Linear RGB  U8, I8, U16, F16, F32
 * Linear RGBA U8, I8, U16, F16, F32
 * Linear DEPTH D16, D24, D32
 * Linear DEPTH_STENCIL D24
 * Linear RG_COMPRESSED U8, I8
 * Linear RGB_COMPRESSED U8
 * Linear RGBA_COMPRESSED U8
 * Linear RGBA1_COMPRESSED U8
 * SRGB   RGB U8
 * SRGB   RGBA U8
 * SRGB   RGB_COMPRESSED U8
 * SRGB   RGBA_COMPRESSED U8
 * SRGB   RGBA1_COMPRESSED U8
 */

/* Filters */
#define SEHLE_TEXTURE_FILTER_NEAREST 0
#define SEHLE_TEXTURE_FILTER_LINEAR 1
#define SEHLE_TEXTURE_FILTER_MIPMAP 2

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleTextureData {
	// Format of pixels (L, LA, RGB, RGBA...)
	unsigned int format : 4;
	unsigned int ch_type : 3;
	unsigned int color_space : 1;
	unsigned int min_filter : 2;
	unsigned int mag_filter : 1;
	/* Whether to generate mipmaps automatically */
	unsigned int generate_mipmaps : 1;
};

struct _SehleTextureImage {
	unsigned int width;
	unsigned int height;
	unsigned short n_layers;
	unsigned short n_mipmaps;
	unsigned int compressed : 1;
	/* Order is layer, level (max 16 layers, 16 levels) */
	const unsigned char *images[256];
};

struct _SehleTexture {
	SehleResource resource;

	/* Format of this texture (L, LA, RGB, RGBA...) */
	unsigned int format : 4;
	/* Preferred channel type (UINT4, UINT8, UINT12...) */
	unsigned int chtype : 3;
	/* Color space */
	unsigned int color_space : 1;
};

struct _SehleTextureClass {
	SehleResourceClass resource_class;
	void (* bind) (SehleTexture *tex, unsigned int channel);
};

unsigned int sehle_texture_get_type (void);

/* For superclass implementation */
void sehle_texture_setup (SehleTexture *tex, SehleEngine *engine, const unsigned char *id);
/* Should be called whenever SEHLE_RESOURCE_MODIFIED flag is set */
void sehle_texture_bind (SehleTexture *tex, unsigned int channel);

/* Helpers */
/* Get GL data format (GL_R, GL_RG, GL_RGB, GL_RGBA, GL_DEPTH_COMPONENT or GL_DEPTH_COMPONENT) from texture format */
unsigned int sehle_texture_get_gl_data_format_from_format (unsigned int format);
unsigned int sehle_texture_get_gl_data_format (SehleTexture *tex);
/* Get GL data type (GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_FLOAT) from texture format */
unsigned int sehle_texture_get_gl_data_type_from_type (unsigned int ch_type);
unsigned int sehle_texture_get_gl_data_type (SehleTexture *tex);
/* Get GL internal format (GL_R8, GL_RG8...) from texture format, channel type and color space */
unsigned int sehle_texture_get_gl_internal_format (SehleTexture *tex);

unsigned int sehle_texture_get_mipmap_levels (unsigned int width, unsigned int height);

/* Get relevant sehle texture format from NR pixblock mode */
unsigned int sehle_texture_get_format_from_pixblock (const NRPixBlock *pb);
unsigned int sehle_texture_get_channel_type_from_pixblock (const NRPixBlock *pb);
unsigned int sehle_texture_get_color_space_from_pixblock (const NRPixBlock *pb);

unsigned int sehle_texture_get_gl_source_format_from_pixblock (const NRPixBlock *pb);
unsigned int sehle_texture_get_gl_source_type_from_pixblock (const NRPixBlock *pb);

unsigned int sehle_texture_format_from_nr (unsigned int pixblock_mode);
/* Get relevant NR pixblock mode from sehle texture format */
unsigned int sehle_texture_format_to_nr (unsigned int format);

unsigned int sehle_get_pow2_ceil (unsigned int val);
unsigned int sehle_get_pow2_floor (unsigned int val);

#ifdef __cplusplus
};
#endif

#endif

