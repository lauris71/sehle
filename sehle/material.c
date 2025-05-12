#define __SEHLE_MATERIAL_CPP__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007
//

#include "GL/glew.h"

#include "engine.h"
#include <sehle/render-context.h>
#include "texture-2d.h"
#include "program.h"
#include <sehle/index-buffer.h>
#include <sehle/vertex-buffer.h>

#include "material.h"

static void material_class_init (SehleMaterialClass *klass);
static void material_implementation_init (SehleMaterialImplementation *impl);
static void material_instance_init (SehleMaterialImplementation *impl, SehleMaterialInstance *material);
static void material_instance_finalize (SehleMaterialImplementation *impl, SehleMaterialInstance *material);

unsigned int
sehle_material_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_interface_type (&type, (const unsigned char *) "SehleMaterial", AZ_TYPE_INTERFACE,
			sizeof (SehleMaterialClass), sizeof (SehleMaterialImplementation), sizeof (SehleMaterialInstance), AZ_FLAG_ZERO_MEMORY,
			(void (*) (AZClass *)) material_class_init,
			(void (*) (AZImplementation *)) material_implementation_init,
			(void (*) (const AZImplementation *, void *)) material_instance_init,
			(void (*) (const AZImplementation *, void *)) material_instance_finalize);
	}
	return type;
}

static void
material_class_init (SehleMaterialClass *klass)
{
}

static void
material_implementation_init(SehleMaterialImplementation *impl)
{
}

static void
material_instance_init (SehleMaterialImplementation *impl, SehleMaterialInstance *inst)
{
	inst->state_flags = SEHLE_RENDER_STATE_DEFAULT;
	inst->render_stages = SEHLE_RENDER_STAGES_ALL;
	inst->render_types = SEHLE_RENDER_TYPES_ALL;
}

static void
material_instance_finalize (SehleMaterialImplementation *impl, SehleMaterialInstance *inst)
{
	for (unsigned int i = 0; i < inst->n_textures; i++) {
		if (inst->textures[i]) {
			az_object_unref (AZ_OBJECT(inst->textures[i]));
		}
	}
	for (unsigned int i = 0; i < inst->n_programs; i++) {
		if (inst->programs[i]) az_object_unref (AZ_OBJECT(inst->programs[i]));
	}
}

void
sehle_material_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	sehle_render_context_set_render_state (ctx, inst->state_flags);
	impl->bind (impl, inst, ctx, render_type);
}

void
sehle_material_set_texture (SehleMaterialInstance *inst, unsigned int idx, SehleTexture *tex)
{
	if (inst->textures[idx]) {
		az_object_unref (AZ_OBJECT (inst->textures[idx]));
	}
	inst->textures[idx] = tex;
}

void
sehle_material_setup (SehleMaterialInstance *inst, unsigned int n_programs, unsigned int n_textures)
{
	inst->n_programs = n_programs;
	inst->n_textures = n_textures;
	for (unsigned int i = 0; i < n_textures; i++) inst->channels[i] = -1;
}

void
sehle_material_release (SehleMaterialInstance *inst)
{
	for (unsigned int i = 0; i < inst->n_textures; i++) {
		if (inst->textures[i]) {
			az_object_unref ((AZObject *) inst->textures[i]);
			inst->textures[i] = NULL;
		}
	}
	for (unsigned int i = 0; i < inst->n_programs; i++) {
		if (inst->programs[i]) {
			az_object_unref ((AZObject *) inst->programs[i]);
			inst->programs[i] = NULL;
		}
	}
	inst->n_programs = 0;
	inst->n_textures = 0;
}

void
sehle_material_instance_bind_texture (SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int prog_idx, unsigned int tex_idx, int sampler_loc)
{
	if (inst->textures[tex_idx]) {
		/* Allocate new texture channel */
		inst->channels[tex_idx] = sehle_render_context_set_texture (ctx, inst->textures[tex_idx]);
		if (inst->channels[tex_idx] >= 0) sehle_texture_bind (inst->textures[tex_idx], inst->channels[tex_idx]);
	}
	if (sampler_loc >= 0) {
		sehle_program_setUniform1i (inst->programs[prog_idx], sampler_loc, (inst->channels[tex_idx] >= 0) ? inst->channels[tex_idx] : 0);
	}
}

