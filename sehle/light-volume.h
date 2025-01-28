#ifndef __SEHLE_LIGHT_VOLUME_H__
#define __SEHLE_LIGHT_VOLUME_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2012-2016
//

#include <elea/plane.h>

#include <sehle/light.h>

typedef struct _SehleLightVolume SehleLightVolume;
typedef struct _SehleLightVolumeClass SehleLightVolumeClass;

#define SEHLE_TYPE_LIGHT_VOLUME sehle_light_volume_get_type ()

#define SEHLE_LIGHT_VOLUME_FROM_MATERIAL_INSTANCE(i) (SehleLightVolume *) ARIKKEI_BASE_ADDRESS (SehleLightInstance, material_inst, i)
#define SEHLE_LIGHT_VOLUME_MATERIAL_IMPLEMENTATION (&sehle_light_volume_class->light_implementation.material_impl)

#include <sehle/light.h>

#define SEHLE_LIGHT_VOLUME_PROGRAM_STENCIL 0
#define SEHLE_LIGHT_VOLUME_PROGRAM_LIGHT 1

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_light_volume_get_reference (SehleEngine *engine, unsigned int program);

#define SEHLE_LIGHT_VOLUME_MAX_PLANES 8

struct _SehleLightVolume {
	SehleLightInstance light_instance;
	float factor;
	EleaPlane3f planes[SEHLE_LIGHT_VOLUME_MAX_PLANES];
};

struct _SehleLightVolumeClass {
	AZClass klass;
	SehleLightImplementation light_implementation;
};

#ifndef __SEHLE_LIGHT_VOLUME_CPP__
extern unsigned int sehle_light_volume_type;
extern SehleLightVolumeClass *sehle_light_volume_class;
#endif

unsigned int sehle_light_volume_get_type (void);

void sehle_light_volume_setup (SehleLightVolume *lvol, SehleEngine *engine);
void sehle_light_volume_release (SehleLightVolume *lvol);

#ifdef __cplusplus
};
#endif

#endif
