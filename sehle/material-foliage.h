#ifndef __SEHLE_MATERIAL_FOLIAGE_H__
#define __SEHLE_MATERIAL_FOLIAGE_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2011-2016
//

#include <elea/vector3.h>
#include <sehle/material.h>

/*
 * Material for rendering instanced plants
 *
 * Vertex inputs:
 *   Coordinates
 *   [Normals]
 *   Texture coordinates
 *
 * Parameters:
 *   Instance matrix
 *   [World normal]
 *   Normal factor
 *     (0..1 weight of model normal)
 *   Time
 *   
 */

/* Foliage program flags */

#define SEHLE_PROGRAM_FOLIAGE_HAS_NORMAL 1
#define SEHLE_PROGRAM_FOLIAGE_HAS_AMBIENT_OCCLUSION 2
#define SEHLE_PROGRAM_FOLIAGE_HAS_WIND 4

/* Foliage program uniforms */

enum {
	SEHLE_PROGRAM_FOLIAGE_VIEW_NORMAL = SEHLE_NUM_UNIFORMS,
	SEHLE_PROGRAM_FOLIAGE_NORMAL_FACTOR,
	SEHLE_PROGRAM_FOLIAGE_TIME,
	SEHLE_PROGRAM_FOLIAGE_COLOR_SAMPLER,
	SEHLE_PROGRAM_FOLIAGE_WIND_SAMPLER,
	SEHLE_PROGRAM_FOLIAGE_NUM_UNIFORMS
};

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_foliage_get_reference (SehleEngine *engine, unsigned int flags, unsigned int max_instances);

void sehle_program_foliage_bind_instance (SehleProgram *prog, unsigned int flags, SehleRenderContext *ctx, unsigned int is_first_of_class, const float *world_normal, float normal_factor);

#define SEHLE_TYPE_MATERIAL_FOLIAGE sehle_material_foliage_get_type ()

#define SEHLE_MATERIAL_FOLIAGE_FROM_MATERIAL_INSTANCE(i) (SehleMaterialFoliage *) ARIKKEI_BASE_ADDRESS (SehleMaterialFoliage, material_instance, i)
#define SEHLE_MATERIAL_FOLIAGE_MATERIAL_IMPLEMENTATION (&sehle_material_foliage_class->material_implementation)

typedef struct _SehleMaterialFoliage SehleMaterialFoliage;
typedef struct _SehleMaterialFoliageClass SehleMaterialFoliageClass;

/* Texture types */

enum {
	SEHLE_MATERIAL_FOLIAGE_TEXTURE_DIFFUSE,
	SEHLE_MATERIAL_FOLIAGE_TEXTURE_NORMAL,
	SEHLE_MATERIAL_FOLIAGE_TEXTURE_SPECULAR,
	SEHLE_MATERIAL_FOLIAGE_TEXTURE_WIND,
	SEHLE_MATERIAL_FOLIAGE_TEXTURE_NUM_TEXTURES
};

struct _SehleMaterialFoliage {
	SehleMaterialInstance material_instance;

	unsigned int program_flags;

	/* Global foliage normal in world coordinates */
	EleaVec3f world_normal;
	/* 0 - global normal; 1 - per-pixel normal */
	float normal_factor;
	unsigned int has_ao;
};

struct _SehleMaterialFoliageClass {
	AZClass klass;
	SehleMaterialImplementation material_implementation;
};

unsigned int sehle_material_foliage_get_type (void);

#ifndef __SEHLE_MATERIAL_FOLIAGE_CPP__
extern unsigned int sehle_material_foliage_type;
extern SehleMaterialFoliageClass *sehle_material_foliage_class;
#endif

SehleMaterialFoliage *sehle_material_foliage_new (SehleEngine *engine);
void sehle_material_foliage_delete (SehleMaterialFoliage *flg);

void sehle_material_foliage_update_program_flags (SehleMaterialFoliage *flg);

/* Set texture (grab reference) */
void sehle_material_foliage_set_texture (SehleMaterialFoliage *mflg, unsigned int texture_type, SehleTexture2D *tex2d);

#ifdef __cplusplus
};
#endif

#endif
