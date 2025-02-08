#ifndef __SEHLE_LIGHT_DIRECTIONAL_H__
#define __SEHLE_LIGHT_DIRECTIONAL_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2015
//

typedef struct _SehleDirectionalLightInstance SehleDirectionalLightInstance;
typedef struct _SehleDirectionalLightImplementation SehleDirectionalLightImplementation;
typedef struct _SehleDirectionalLightClass SehleDirectionalLightClass;

#define SEHLE_TYPE_DIRECTIONAL_LIGHT (sehle_directional_light_get_type ())

#define SEHLE_DIRECTIONAL_LIGHT_FROM_RENDERABLE_INSTANCE(i) (SehleDirectionalLightInstance *) ARIKKEI_BASE_ADDRESS (SehleLightInstance, renderable_inst, i)
#define SEHLE_DIRECTIONAL_LIGHT_FROM_RENDERABLE_IMPLEMENTATION(i) (SehleDirectionalLightImplementation *) ARIKKEI_BASE_ADDRESS (SehleLightImplementation, renderable_impl, i)
#define SEHLE_DIRECTIONAL_LIGHT_FROM_MATERIAL_INSTANCE(i) (SehleDirectionalLightInstance *) ARIKKEI_BASE_ADDRESS (SehleLightInstance, material_inst, i)
#define SEHLE_DIRECTIONAL_LIGHT_FROM_MATERIAL_IMPLEMENTATION(i) (SehleDirectionalLightImplementation *) ARIKKEI_BASE_ADDRESS (SehleLightImplementation, material_impl, i)

#include <sehle/light.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleDirectionalLightInstance {
	SehleLightInstance light_inst;
	/* Shadow matrices */
	/* fixme: Is this the right place? */
	EleaMat4x4f v2s[4];
	float splits[4];
};

struct _SehleDirectionalLightImplementation {
	SehleLightImplementation light_impl;
};

struct _SehleDirectionalLightClass {
	SehleLightClass light_class;
};

unsigned int sehle_directional_light_get_type (void);

#ifndef __SEHLE_DIRECTIONAL_LIGHT_C__
extern unsigned int sehle_directional_light_type;
extern SehleDirectionalLightClass *sehle_directional_light_class;
#endif

void sehle_directional_light_setup (SehleDirectionalLightInstance *dlight, SehleEngine *engine, float priority, unsigned int num_splits);

void sehle_directional_light_set_shadow_matrices (SehleDirectionalLightInstance *dlight, EleaMat3x4f *v2w, EleaMat3x4f *l2w, EleaMat4x4f shadow_projections[], float splits[]);

#ifdef __cplusplus
};
#endif

#endif

