#ifndef __SEHLE_MATERIAL_CONTROL_H__
#define __SEHLE_MATERIAL_CONTROL_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2010-2016
//

#include <elea/color.h>
#include <elea/vector3.h>
#include <sehle/material.h>

#define SEHLE_PROGRAM_CONTROL_MAX_LIGHTS 4

/* Control program flags */

#define SEHLE_PROGRAM_CONTROL_HAS_COLORS 1
#define SEHLE_PROGRAM_CONTROL_HAS_TEXTURE 2

/* Control program uniforms */

enum SehleControlUniforms {
	SEHLE_PROGRAM_CONTROL_DIFFUSE = SEHLE_NUM_UNIFORMS,
	SEHLE_PROGRAM_CONTROL_COLOR_SAMPLER,
	SEHLE_PROGRAM_CONTROL_OPACITY,
	SEHLE_PROGRAM_CONTROL_AMBIENT,
	SEHLE_PROGRAM_CONTROL_LIGHT_DIRECTION,
	SEHLE_PROGRAM_CONTROL_LIGHT_COLOR,
	SEHLE_PROGRAM_CONTROL_NUM_UNIFORMS
};

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_control_get_reference (SehleEngine *engine, unsigned int flags, unsigned int n_lights);

#define SEHLE_TYPE_MATERIAL_CONTROL sehle_material_control_get_type ()

#define SEHLE_MATERIAL_CONTROL_FROM_MATERIAL_INSTANCE(i) (SehleMaterialControl *) ARIKKEI_BASE_ADDRESS (SehleMaterialControl, material_inst, i)
#define SEHLE_MATERIAL_CONTROL_MATERIAL_IMPLEMENTATION (&sehle_material_control_class->material_impl)

typedef struct _SehleMaterialControl SehleMaterialControl;
typedef struct _SehleMaterialControlClass SehleMaterialControlClass;

struct _SehleMaterialControl {
	SehleMaterialInstance material_inst;

	unsigned int program_flags;

	EleaColor4f color;
	float opacity;
	EleaColor4f ambient;
	EleaVec3f light_directions[SEHLE_PROGRAM_CONTROL_MAX_LIGHTS];
	EleaColor4f light_colors[SEHLE_PROGRAM_CONTROL_MAX_LIGHTS];
	unsigned int n_lights : 4;
	unsigned int has_colors : 1;
};

struct _SehleMaterialControlClass {
	AZClass klass;
	SehleMaterialImplementation material_impl;
};

unsigned int sehle_material_control_get_type (void);

#ifndef __SEHLE_MATERIAL_CONTROL_CPP__
extern unsigned int sehle_material_control_type;
extern SehleMaterialControlClass *sehle_material_control_class;
#endif

SehleMaterialControl *sehle_material_control_new (SehleEngine *engine);
void sehle_material_control_delete (SehleMaterialControl *flg);

/* Set texture (grab reference) */
void sehle_material_control_set_texture (SehleMaterialControl *mflg, SehleTexture2D *tex2d);
void sehle_material_control_set_has_colors (SehleMaterialControl *mctrl, unsigned int has_colors);

#ifdef __cplusplus
};
#endif

#endif
