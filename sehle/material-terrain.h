#ifndef __SEHLE_MATERIAL_TERRAIN_H__
#define __SEHLE_MATERIAL_TERRAIN_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2012
//

#include <sehle/material.h>

#define SEHLE_PROGRAM_TERRAIN_HAS_MAP_1 1
#define SEHLE_PROGRAM_TERRAIN_HAS_NOISE 2

enum {
	SEHLE_PROGRAM_TERRAIN_MAP = SEHLE_NUM_UNIFORMS,
	SEHLE_PROGRAM_TERRAIN_NOISE, SEHLE_PROGRAM_TERRAIN_TEXTURE,
	SEHLE_PROGRAM_TERRAIN_MAPPING, SEHLE_PROGRAM_TERRAIN_SCALE,
	SEHLE_PROGRAM_TERRAIN_NUM_UNIFORMS
};

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_terrain_get_reference (SehleEngine *engine, unsigned int flags);

typedef struct _SehleMaterialTerrain SehleMaterialTerrain;
typedef struct _SehleMaterialTerrainClass SehleMaterialTerrainClass;

#define SEHLE_TYPE_MATERIAL_TERRAIN sehle_material_terrain_get_type ()

#define SEHLE_MATERIAL_TERRAIN_FROM_MATERIAL_INSTANCE(i) (SehleMaterialTerrain *) ARIKKEI_BASE_ADDRESS (SehleMaterialTerrain, material_instance, i)
#define SEHLE_MATERIAL_TERRAIN_MATERIAL_IMPLEMENTATION (&sehle_material_terrain_class->material_implementation)

#define SEHLE_MATERIAL_TERRAIN_MAPPING_UV 0
#define SEHLE_MATERIAL_TERRAIN_MAPPING_MAP 1
#define SEHLE_MATERIAL_TERRAIN_MAPPING_GENERATED 2

#define SEHLE_MATERIAL_TERRAIN_NUM_GROUND_TEXTURES 9
#define SEHLE_MATERIAL_TERRAIN_TEXTURE_MAP_0 0
#define SEHLE_MATERIAL_TERRAIN_TEXTURE_MAP_1 1
#define SEHLE_MATERIAL_TERRAIN_TEXTURE_NOISE 2
#define SEHLE_MATERIAL_TERRAIN_TEXTURE_GROUND_0 3
#define SEHLE_MATERIAL_TERRAIN_NUM_SAMPLERS 12

struct _SehleMaterialTerrain {
	SehleMaterialInstance material_instance;
	/* Current program flags */
	unsigned int program_flags;

	float scale[SEHLE_MATERIAL_TERRAIN_NUM_GROUND_TEXTURES];
	unsigned int mapping[SEHLE_MATERIAL_TERRAIN_NUM_GROUND_TEXTURES];
	unsigned int num_maps;
};

struct _SehleMaterialTerrainClass {
	AZClass klass;
	SehleMaterialImplementation material_implementation;
};

unsigned int sehle_material_terrain_get_type (void);

#ifndef __SEHLE_MATERIAL_TERRAIN_CPP__
extern unsigned int sehle_material_terrain_type;
extern SehleMaterialTerrainClass *sehle_material_terrain_class;
#endif

SehleMaterialTerrain *sehle_material_terrain_new (SehleEngine *engine);
void sehle_material_terrain_delete (SehleMaterialTerrain *tmat);

/* All methods grab reference */
void sehle_material_terrain_set_ground_texture (SehleMaterialTerrain *tmat, unsigned int idx, SehleTexture2D *texture, unsigned int mapping, float scale);
void sehle_material_terrain_set_map_texture (SehleMaterialTerrain *tmat, unsigned int idx, SehleTexture2D *texture);
void sehle_material_terrain_set_noise_texture (SehleMaterialTerrain *tmat, SehleTexture2D *texture);

#ifdef __cplusplus
};
#endif

#endif
