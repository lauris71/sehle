#ifndef __SEHLE_LIGHT_POINT_H__
#define __SEHLE_LIGHT_POINT_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2015
//

typedef struct _SehlePointLightInstance SehlePointLightInstance;
typedef struct _SehlePointLightImplementation SehlePointLightImplementation;
typedef struct _SehlePointLightClass SehlePointLightClass;

#define SEHLE_TYPE_POINT_LIGHT (sehle_point_light_get_type ())

#define SEHLE_POINT_LIGHT_FROM_RENDERABLE_INSTANCE(i) (SehlePointLightInstance *) ARIKKEI_BASE_ADDRESS (SehleLightInstance, renderable_inst, i)
#define SEHLE_POINT_LIGHT_FROM_RENDERABLE_IMPLEMENTATION(i) (SehlePointLightImplementation *) ARIKKEI_BASE_ADDRESS (SehleLightImplementation, renderable_impl, i)
#define SEHLE_POINT_LIGHT_FROM_MATERIAL_INSTANCE(i) (SehlePointLightInstance *) ARIKKEI_BASE_ADDRESS (SehleLightInstance, material_inst, i)
#define SEHLE_POINT_LIGHT_FROM_MATERIAL_IMPLEMENTATION(i) (SehlePointLightImplementation *) ARIKKEI_BASE_ADDRESS (SehleLightImplementation, material_impl, i)

#include <sehle/light.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehlePointLightInstance {
	SehleLightInstance light_inst;
};

struct _SehlePointLightImplementation {
	SehleLightImplementation light_impl;
};

struct _SehlePointLightClass {
	SehleLightClass light_klass;
};

unsigned int sehle_point_light_get_type (void);

void sehle_point_light_setup (SehlePointLightInstance *inst, SehleEngine *engine, float priority);

void sehle_point_light_set_point_attenuation (SehlePointLightInstance *point, float min_dist, float radius, float falloff);

void sehle_point_light_update_visuals(SehlePointLightInstance *point);

#ifdef __cplusplus
};
#endif

#endif

