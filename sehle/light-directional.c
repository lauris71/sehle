#define __SEHLE_LIGHT_DIRECTIONAL_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2015
//

#include "GL/glew.h"

#include "engine.h"
#include "index-buffer.h"
#include "program.h"
#include <sehle/render-context.h>
#include "render-target.h"
#include "shader.h"
#include <sehle/texture-2d.h>
#include <sehle/vertex-array.h>
#include <sehle/vertex-buffer.h>

#include <sehle/light-directional.h>

static void directional_light_implementation_init (SehleDirectionalLightImplementation *impl);
static void directional_light_instance_init (SehleDirectionalLightImplementation *impl, SehleDirectionalLightInstance *dlight);

/* Material implementation */
static void directional_light_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);
static void directional_light_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data);
/* Light implementation */
static void dirl_setup_forward (SehleLightImplementation *impl, SehleLightInstance *inst, SehleRenderContext *ctx);

enum Attributes {
	VERTEX, NUM_ATTRIBUTES
};

static const char *attribute_names[] = {
	"vertex"
};

unsigned int directional_light_type = 0;
SehleDirectionalLightClass *directional_light_class = NULL;

unsigned int
sehle_directional_light_get_type (void)
{
	if (!directional_light_type) {
		directional_light_class = (SehleDirectionalLightClass *) az_register_interface_type (&directional_light_type, (const unsigned char *) "SehleDirectionalLight", SEHLE_TYPE_LIGHT,
			sizeof (SehleDirectionalLightClass), sizeof (SehleDirectionalLightImplementation), sizeof (SehleDirectionalLightInstance), 0,
			NULL,
			(void (*) (AZImplementation *)) directional_light_implementation_init,
			(void (*) (const AZImplementation *, void *)) directional_light_instance_init,
			NULL);
	}
	return directional_light_type;
}

static void
directional_light_implementation_init (SehleDirectionalLightImplementation *impl)
{
	impl->light_impl.setup_forward = dirl_setup_forward;
	/* Material implementation */
	impl->light_impl.material_impl.bind = directional_light_bind;
	impl->light_impl.renderable_impl.render = directional_light_render;
}

static void
directional_light_instance_init (SehleDirectionalLightImplementation *impl, SehleDirectionalLightInstance *dlight)
{
	unsigned int i;
	for (i = 0; i < 4; i++) {
		dlight->v2s[i] = EleaMat4x4fIdentity;
		dlight->splits[i] = -INFINITY;
	}
}

static void
dirl_setup_forward (SehleLightImplementation *impl, SehleLightInstance *inst, SehleRenderContext *ctx)
{
	SehleDirectionalLightImplementation *dirl_impl = (SehleDirectionalLightImplementation *) impl;
	SehleDirectionalLightInstance *dirl_inst = (SehleDirectionalLightInstance *) inst;
	/* Use negative Z with W = 0 as light position */
	EleaVec3f z_world, z_eye;
	elea_mat3x4f_get_col_vec(&z_world, &inst->l2w, 2);
	elea_mat3x4f_transform_vec3(&z_eye, &ctx->w2v, &z_world);
	inst->info.pos = elea_vec4f_from_xyzw(z_eye.x, z_eye.y, z_eye.z, 0);
	inst->info.dir = EleaVec3fX;
}

static void
directional_light_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleDirectionalLightInstance *dlight = SEHLE_DIRECTIONAL_LIGHT_FROM_MATERIAL_INSTANCE (inst);
	EleaVec3f l_dir;
	sehle_light_bind_common (&dlight->light_inst, ctx);

	/* Light position */
	EleaVec3f z_world, z_eye;
	elea_mat3x4f_get_col_vec(&z_world, &dlight->light_inst.l2w, 2);
	elea_mat3x4f_transform_vec3(&z_eye, &ctx->w2v, &z_world);
	EleaVec4f light_pos = {z_eye.x, z_eye.y, z_eye.z, 0};
	sehle_program_setUniform4fv (inst->programs[0], SEHLE_LIGHT_LIGHTPOS, 1, light_pos.c);

	elea_mat3x4f_get_col_vec (&l_dir, &dlight->light_inst.l2w, 2);
	l_dir = elea_vec3f_inv (l_dir);
	elea_mat3x4f_transform_vec3 (&l_dir, &ctx->w2v, &l_dir);
	sehle_program_setUniform3fv (inst->programs[0], SEHLE_LIGHT_LIGHTDIR, 1, l_dir.c);
}

static void
directional_light_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data)
{
	SehleDirectionalLightInstance *dirl_inst = SEHLE_DIRECTIONAL_LIGHT_FROM_RENDERABLE_INSTANCE (inst);
	SehleDirectionalLightImplementation *dirl_impl = SEHLE_DIRECTIONAL_LIGHT_FROM_RENDERABLE_IMPLEMENTATION (impl);
	int shadow_channel = -1;
	int density_channel = -1;
	if (dirl_inst->light_inst.render_shadow && dirl_impl->light_impl.render_shadow) {
		dirl_impl->light_impl.render_shadow (&dirl_impl->light_impl, &dirl_inst->light_inst, ctx);
		sehle_render_context_bind (ctx);
		/* fixme: We were bound before render call but shadow most probably messed everything up */
		sehle_material_bind (&dirl_impl->light_impl.material_impl, &dirl_inst->light_inst.material_inst, ctx, render_type);
	}

	/* fixme: Because we set shadow texture the program flags change and prog is not correct anymore */
	/* fixme: Think how to handle it */
	prog = dirl_inst->light_inst.material_inst.programs[0];

	//sehle_material_invoke_bind (impl, inst, ctx, render_type);
	if (dirl_inst->light_inst.shadow && (dirl_inst->light_inst.program_flags & SEHLE_PROGRAM_LIGHT_HAS_SHADOW)) {
		//shadow_channel = sehle_render_context_set_texture (ctx, NULL);
		shadow_channel = sehle_render_context_set_texture (ctx, dirl_inst->light_inst.shadow);
		if (shadow_channel >= 0) {
			sehle_texture_2d_bind_shadow ((SehleTexture2D *) dirl_inst->light_inst.shadow, shadow_channel);
			sehle_program_setUniform1i (prog, SEHLE_LIGHT_SHADOW_SAMPLER, shadow_channel);
		}
	}
	if (dirl_inst->light_inst.density && (dirl_inst->light_inst.program_flags & SEHLE_PROGRAM_LIGHT_HAS_DENSITY)) {
		density_channel = sehle_render_context_set_texture (ctx, dirl_inst->light_inst.density);
		if (density_channel >= 0) {
			sehle_texture_bind (dirl_inst->light_inst.density, density_channel);
			sehle_program_setUniform1i (prog, SEHLE_LIGHT_DENSITY_SAMPLER, density_channel);
		}
	}

	// We have to set composite transformation for each light separately
	EleaMat4x4f l_proj;
	elea_mat4x4f_set_ortho_wh (&l_proj, 2, 2, -1, 1);
	sehle_program_setUniformMatrix4fv (prog, SEHLE_LIGHT_L2W_W2V_PROJECTION, 1, l_proj.c);
	// programs[programtype]->setUniformMatrix4fv (L2W_W2V_PROJECTION, 1, Elea::Matrix4x4fIdentity);
	// Same for shadow lookup
	// sehle_program_setUniformMatrix4fv (inst->programs[0], E2S, 1, e2s[light]);

	sehle_program_setUniform1fv (prog, SEHLE_LIGHT_SPLITS, dirl_inst->light_inst.num_splits, dirl_inst->splits);
	sehle_program_setUniformMatrix4fv (prog, E2S, dirl_inst->light_inst.num_splits, dirl_inst->v2s[0].c);

	sehle_vertex_array_render_triangles (dirl_inst->light_inst.va, 1, 0, dirl_inst->light_inst.va->ibuf->buffer.n_elements);
}

void
sehle_directional_light_setup (SehleDirectionalLightInstance *dlight, SehleEngine *engine, float priority, unsigned int num_splits)
{
	sehle_material_setup (&dlight->light_inst.material_inst, 1, 0);
	dlight->light_inst.renderable_inst.bbox = EleaAABox3fInfinite;
	dlight->light_inst.priority = priority;

	dlight->light_inst.program_flags = SEHLE_PROGRAM_LIGHT_DIRECTIONAL;
	dlight->light_inst.num_splits = num_splits;

	dlight->light_inst.va = sehle_engine_get_standard_geometry (engine, SEHLE_GEOMETRY_UNIT_CUBE_INSIDE);
}

void
sehle_directional_light_set_shadow_matrices (SehleDirectionalLightInstance *dlight, EleaMat3x4f *v2w, EleaMat3x4f *l2w, EleaMat4x4f shadow_projections[], float splits[])
{
	EleaMat3x4f w2l, v2l;
	EleaMat4x4f b;
	elea_mat3x4f_invert_normalized (&w2l, l2w);
	elea_mat3x4f_multiply (&v2l, &w2l, v2w);
	if (dlight->light_inst.num_splits == 1) {
		static const EleaMat4x4f bias = { 0.5f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0, 0.5f, 0, 0.5f, 0.5f, 0.5f, 1 };
		for (int i = 0; i < 3; i++) dlight->splits[i] = -INFINITY;
		dlight->splits[3] = INFINITY;
		elea_mat4x4f_multiply (&b, &bias, &shadow_projections[0]);
		elea_mat4x4f_multiply_mat3x4 (&dlight->v2s[0], &b, &v2l);
	} else if (dlight->light_inst.num_splits == 4) {
		static const EleaMat4x4f bias = { 0.25f, 0, 0, 0, 0, 0.25f, 0, 0, 0, 0, 0.5f, 0, 0.25f, 0.25f, 0.5f, 1 };
		for (int i = 0; i < 3; i++) dlight->splits[i] = splits[i];

		elea_mat4x4f_translate_left_xyz (&b, &bias, 0, 0, 0);
		elea_mat4x4f_multiply_self_right (&b, &shadow_projections[0]);
		elea_mat4x4f_multiply_mat3x4 (&dlight->v2s[0], &b, &v2l);

		elea_mat4x4f_translate_left_xyz (&b, &bias, 0.5f, 0, 0);
		elea_mat4x4f_multiply_self_right (&b, &shadow_projections[1]);
		elea_mat4x4f_multiply_mat3x4 (&dlight->v2s[1], &b, &v2l);

		elea_mat4x4f_translate_left_xyz (&b, &bias, 0, 0.5f, 0);
		elea_mat4x4f_multiply_self_right (&b, &shadow_projections[2]);
		elea_mat4x4f_multiply_mat3x4 (&dlight->v2s[2], &b, &v2l);

		elea_mat4x4f_translate_left_xyz (&b, &bias, 0.5f, 0.5f, 0);
		elea_mat4x4f_multiply_self_right (&b, &shadow_projections[3]);
		elea_mat4x4f_multiply_mat3x4 (&dlight->v2s[3], &b, &v2l);
	}
}
