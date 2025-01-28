#ifndef __SEHLE_TERRAIN_H__
#define __SEHLE_TERRAIN_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2022
 */

typedef struct _SehleTerrain SehleTerrain;
typedef struct _SehleTerrainClass SehleTerrainClass;
typedef struct _SehleStaticMeshFragment SehleStaticMeshFragment;

#define SEHLE_TERRAIN_FROM_RENDERABLE_INSTANCE(i) (SehleTerrain *) ARIKKEI_BASE_ADDRESS (SehleTerrain, renderable_inst, i)
#define SEHLE_TERRAIN_RENDERABLE_IMPLEMENTATION (&sehle_terrain_class->renderable_impl)

#include <sehle/renderable.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleTerrain {
	SehleRenderable renderable_inst;

	EleaMat3x4f r2w;

	SehleVertexArray *va;
};

struct _SehleTerrainClass {
	AZClass klass;
	SehleRenderableImplementation renderable_impl;
};

unsigned int sehle_static_mesh_get_type (void);

#ifndef __SEHLE_TERRAIN_C__
extern unsigned int sehle_terrain_type;
extern SehleTerrainClass *sehle_terrain_class;
#endif

void sehle_terrain_setup (SehleTerrain *terrain, SehleEngine *engine, unsigned int layer_mask);
void sehle_terrain_release (SehleTerrain *terrain);

#ifdef __cplusplus
};
#endif

#endif

