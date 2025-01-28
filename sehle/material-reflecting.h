#ifndef __SEHLE_MATERIAL_REFLECTING_H__
#define __SEHLE_MATERIAL_REFLECTING_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2016
//

typedef struct _SehleMaterialReflectingInstance SehleMaterialReflectingInstance;
typedef struct _SehleMaterialReflectingImplementation SehleMaterialReflectingImplementation;
typedef struct _SehleMaterialReflectingClass SehleMaterialReflectingClass;

#define SEHLE_TYPE_MATERIAL_REFLECTING sehle_material_reflecting_get_type ()

#define SEHLE_MATERIAL_REFLECTING_TEXTURE_REFLECTION 0

#include <elea/matrix3x4.h>
#include <elea/matrix4x4.h>
#include <sehle/material.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleMaterialReflectingInstance {
	SehleMaterialInstance material_inst;
	/* Reflection transform */
	EleaMat4x4f o2v_projection;
};

struct _SehleMaterialReflectingImplementation {
	SehleMaterialImplementation material_impl;
	void (*render_reflection) (SehleMaterialReflectingImplementation *impl, SehleMaterialReflectingInstance *inst);
};

struct _SehleMaterialReflectingClass {
	SehleMaterialClass material_klass;
};

unsigned int sehle_material_reflecting_get_type (void);

/* Set reflection texture (grab reference) */
void sehle_material_reflecting_set_reflection (SehleMaterialReflectingImplementation *impl, SehleMaterialReflectingInstance *inst, SehleTexture2D *reflection);
/* Set reflection transformations and bias */
void sehle_material_reflecting_set_o2v_reflection (SehleMaterialReflectingImplementation *impl, SehleMaterialReflectingInstance *inst, const EleaMat3x4f *o2v, const EleaMat4x4f *proj, float x0, float y0, float x1, float y1);

#ifdef __cplusplus
};
#endif

#endif

