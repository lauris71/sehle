#ifndef __SEHLE_SHADER_H__
#define __SEHLE_SHADER_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2015
//

typedef struct _SehleShader SehleShader;
typedef struct _SehleShaderClass SehleShaderClass;

#define SEHLE_TYPE_SHADER (sehle_shader_get_type ())
#define SEHLE_SHADER(s) (ARIKKEI_CHECK_INSTANCE_CAST ((s), SEHLE_TYPE_SHADER, SehleShader))
#define SEHLE_IS_SHADER(s) (ARIKKEI_CHECK_INSTANCE_TYPE ((s), SEHLE_TYPE_SHADER))

#define SEHLE_SHADER_VERTEX 0
#define SEHLE_SHADER_TESSELATION_CONTROL 1
#define SEHLE_SHADER_TESSELATION_EVALUATION 2
#define SEHLE_SHADER_TESSELATION_GEOMETRY 3
#define SEHLE_SHADER_FRAGMENT 4
#define SEHLE_NUM_SHADER_TYPES 5

#include <sehle/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleShader {
	SehleResource resource;
	unsigned int shader_type;
	unsigned char *code;
};

struct _SehleShaderClass {
	SehleResourceClass resource_class;
};

unsigned int sehle_shader_get_type (void);

SehleShader *sehle_shader_new (SehleEngine *engine, unsigned int shader_type, const unsigned char *id);

/*
 * Build and initialize shader
 * If lengths is NULL or specific length <= 0 sources is assumed to be zero-terminated
 */
void sehle_shader_build (SehleShader *shader, const unsigned char *sources[], unsigned int nsources, const int64_t lengths[]);

/*
 * Frontends to build
 */
void sehle_shader_build_from_file (SehleShader *shader, const unsigned char *filename);
void sehle_shader_build_from_files (SehleShader *shader, unsigned int nfiles, const unsigned char *filenames[]);
void sehle_shader_build_from_header_files (SehleShader *shader, const unsigned char *hdata, int hsize, const unsigned char *filenames[], unsigned int nfiles);
void sehle_shader_build_from_data (SehleShader *shader, const unsigned char *cdata, unsigned int csize);
/* Fetch existing or build new */
SehleShader *sehle_shader_fetch_from_file (SehleEngine *engine, const char *filename, unsigned int type);

const unsigned char *sehle_shader_map (const unsigned char *key, unsigned int *size, unsigned int *is_file);

#ifdef __cplusplus
};
#endif

#endif

