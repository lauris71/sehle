#ifndef __SEHLE_LIGHT_SPOT_H__
#define __SEHLE_LIGHT_SPOT_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2016
//

typedef struct _SehleSpotLightInstance SehleSpotLightInstance;
typedef struct _SehleSpotLightImplementation SehleSpotLightImplementation;
typedef struct _SehleSpotLightClass SehleSpotLightClass;

#define SEHLE_TYPE_SPOT_LIGHT (sehle_spot_light_get_type ())

#define SEHLE_SPOT_LIGHT_FROM_RENDERABLE_INSTANCE(i) (SehleSpotLightInstance *) ARIKKEI_BASE_ADDRESS (SehleLightInstance, renderable_inst, i)
#define SEHLE_SPOT_LIGHT_FROM_RENDERABLE_IMPLEMENTATION(i) (SehleSpotLightImplementation *) ARIKKEI_BASE_ADDRESS (SehleLightImplementation, renderable_impl, i)
#define SEHLE_SPOT_LIGHT_FROM_MATERIAL_INSTANCE(i) (SehleSpotLightInstance *) ARIKKEI_BASE_ADDRESS (SehleLightInstance, material_inst, i)
#define SEHLE_SPOT_LIGHT_FROM_MATERIAL_IMPLEMENTATION(i) (SehleSpotLightImplementation *) ARIKKEI_BASE_ADDRESS (SehleLightClass, material_impl, i)

#include <sehle/light.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleSpotLightInstance {
	SehleLightInstance light_inst;
};

struct _SehleSpotLightImplementation {
	SehleLightImplementation light_impl;
};

struct _SehleSpotLightClass {
	SehleLightClass light_class;
};

unsigned int sehle_spot_light_get_type (void);

void sehle_spot_light_setup (SehleSpotLightInstance *inst, SehleEngine *engine, float priority);

void sehle_spot_light_set_point_attenuation (SehleSpotLightInstance *inst, float min_dist, float radius, float falloff);
void sehle_spot_light_set_spot_attenuation (SehleSpotLightInstance *inst, float inner_angle, float outer_angle, float falloff);

void sehle_spot_light_update_geometry (SehleSpotLightInstance *inst);

void sehle_spot_light_update_visuals(SehleSpotLightInstance *spot);

#ifdef __cplusplus
};
#endif

#endif

