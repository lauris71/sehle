#ifndef __SEHLE_MATERIAL_WATER_H__
#define __SEHLE_MATERIAL_WATER_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2011-2016
//

typedef struct _SehleMaterialWater SehleMaterialWater;
typedef struct _SehleMaterialWaterClass SehleMaterialWaterClass;
typedef struct _SehleMaterialWaterRipple SehleMaterialWaterRipple;

#define SEHLE_TYPE_MATERIAL_WATER sehle_material_water_get_type ()

#define SEHLE_MATERIAL_WATER_FROM_MATERIAL_INSTANCE(i) (SehleMaterialWater *) ARIKKEI_BASE_ADDRESS (SehleMaterialWater, reflecting_inst, i)
#define SEHLE_MATERIAL_WATER_MATERIAL_IMPLEMENTATION (&sehle_material_water_class->reflecting_impl.material_impl)

#define SEHLE_MATERIAL_WATER_TEXTURE_WAVES 1

#include <elea/color.h>
#include <elea/matrix3x4.h>
#include <elea/vector2.h>
#include <sehle/material-reflecting.h>

/* Relative depth is in red color channel */
#define SEHLE_PROGRAM_WATER_HAS_DEPTH 1

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_water_get_reference (SehleEngine *engine, unsigned int flags, unsigned int num_ripples);

struct _SehleMaterialWaterRipple {
	EleaVec2f center;
	float start;
	float end;
	float amplitude;
	float wavelength;
	/* 0 ... 2_Pi */
	float phase;
};

struct _SehleMaterialWater {
	SehleMaterialReflectingInstance reflecting_inst;

	// Maximum depth (correspoinding to 1.0 in depth map)
	float maxDepth;
	// Color of opaque water column
	EleaColor4f color;
	// The thickness of fully opaque water column
	float opaqueDepth;
	// Minimum reflection and opacity values
	float minOpacity;
	float minReflectivity;
	float maxReflectivity;

	EleaMat3x4f wave_matrix[2];

	// Ripples
	SehleMaterialWaterRipple ripples[8];
};

struct _SehleMaterialWaterClass {
	AZClass klass;
	SehleMaterialReflectingImplementation reflecting_impl;
};

#ifndef __SEHLE_MATERIAL_WATER_CPP__
extern unsigned int sehle_material_water_type;
extern SehleMaterialWaterClass *sehle_material_water_class;
#endif

unsigned int sehle_material_water_get_type (void);

/* Set wave texture (grab reference) */
void sehle_material_water_set_waves (SehleMaterialWater *mwater, SehleTexture2D *waves);

#ifdef __cplusplus
};
#endif

#endif
