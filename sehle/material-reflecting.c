#define __SEHLE_MATERIAL_REFLECTING_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2016
//

#include <sehle/material-reflecting.h>

static void material_reflecting_implementation_init (SehleMaterialReflectingImplementation *impl);
static void material_reflecting_instance_init (SehleMaterialReflectingImplementation *impl, SehleMaterialReflectingInstance *inst);

unsigned int
sehle_material_reflecting_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_interface_type (&type, (const unsigned char *) "SehleMaterialReflecting", SEHLE_TYPE_MATERIAL,
			sizeof (SehleMaterialReflectingClass), sizeof (SehleMaterialReflectingImplementation), sizeof (SehleMaterialReflectingInstance), 0,
			NULL,
			(void (*) (AZImplementation *)) material_reflecting_implementation_init,
			(void (*) (const AZImplementation *, void *)) material_reflecting_instance_init,
			NULL);
	}
	return type;
}

static void
material_reflecting_implementation_init (SehleMaterialReflectingImplementation *impl)
{
}

static void
material_reflecting_instance_init (SehleMaterialReflectingImplementation *impl, SehleMaterialReflectingInstance *inst)
{
	// Request reflection map
	inst->material_inst.properties |= SEHLE_MATERIAL_REFLECTION;
	inst->o2v_projection = EleaMat4x4fIdentity;
}

void
sehle_material_reflecting_set_reflection (SehleMaterialReflectingImplementation *impl, SehleMaterialReflectingInstance *inst, SehleTexture2D *reflection)
{
	sehle_material_set_texture (&inst->material_inst, SEHLE_MATERIAL_REFLECTING_TEXTURE_REFLECTION, (SehleTexture *) reflection);
}

void
sehle_material_reflecting_set_o2v_reflection (SehleMaterialReflectingImplementation *impl, SehleMaterialReflectingInstance *inst, const EleaMat3x4f *o2v, const EleaMat4x4f *proj, float x0, float y0, float x1, float y1)
{
	EleaMat4x4f bias = { (x1 - x0) / 2, 0, 0, 0, 0, (y1 - y0) / 2, 0, 0, 0, 0, 0.5f, 0, x0 + (x1 - x0) / 2, y0 + (y1 - y0) / 2, 0.5f, 1 };
	EleaMat4x4f m;
	elea_mat4x4f_multiply_mat3x4 (&m, proj, o2v);
	elea_mat4x4f_multiply (&inst->o2v_projection, &bias, &m);
}
