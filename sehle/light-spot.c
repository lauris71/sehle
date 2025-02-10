#define __SEHLE_LIGHT_SPOT_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2015
//

#include "GL/glew.h"

#include "engine.h"
#include "index-buffer.h"
#include "program.h"
#include <sehle/render-context.h>
#include "shader.h"
#include "vertex-buffer.h"

#include "light-spot.h"

static void spot_light_implementation_init (SehleSpotLightImplementation *impl);

/* Light implementation */
static void spotl_setup_forward (SehleLightImplementation *impl, SehleLightInstance *inst, SehleRenderContext *ctx);
/* Material implementation */
static void spot_light_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);
static void spot_light_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data);

static void spot_light_build_volume (SehleSpotLightInstance *inst, float *vertices, unsigned int stride_floats);

enum Attributes {
	VERTEX, NUM_ATTRIBUTES
};

static const char *attribute_names[] = {
	"vertex"
};

static unsigned int spot_light_type = 0;
SehleSpotLightClass *spot_light_class = NULL;

unsigned int
sehle_spot_light_get_type (void)
{
	if (!spot_light_type) {
		spot_light_class = (SehleSpotLightClass *) az_register_interface_type (&spot_light_type, (const unsigned char *) "SehleSpotLight", SEHLE_TYPE_LIGHT,
			sizeof (SehleSpotLightClass), sizeof (SehleSpotLightImplementation), sizeof (SehleSpotLightInstance), 0,
			NULL,
			(void (*) (AZImplementation *)) spot_light_implementation_init,
			NULL, NULL);
	}
	return spot_light_type;
}

static void
spot_light_implementation_init (SehleSpotLightImplementation *impl)
{
	impl->light_impl.setup_forward = spotl_setup_forward;
	/* Material implementation */
	impl->light_impl.material_impl.bind = spot_light_bind;
	impl->light_impl.renderable_impl.render = spot_light_render;
}

static void
spotl_setup_forward (SehleLightImplementation *impl, SehleLightInstance *inst, SehleRenderContext *ctx)
{
	SehleSpotLightImplementation *dirl_impl = (SehleSpotLightImplementation *) impl;
	SehleSpotLightInstance *dirl_inst = (SehleSpotLightInstance *) inst;
	inst->info.pos = elea_vec4f_from_xyzw(inst->l2w.c[3], inst->l2w.c[7], inst->l2w.c[11], 1);
	EleaVec3f p_world, p_eye;
	elea_mat3x4f_get_translation(&p_world, &inst->l2w);
	elea_mat3x4f_transform_point(&p_eye, &ctx->w2v, &p_world);
	inst->info.pos = elea_vec4f_from_xyzw(p_eye.x, p_eye.y, p_eye.z, 1);
	EleaVec3f z_world;
	elea_mat3x4f_get_col_vec(&z_world, &inst->l2w, 2);
	z_world = elea_vec3f_inv(z_world);
	elea_mat3x4f_transform_vec3(&inst->info.dir, &ctx->w2v, &z_world);
	inst->info.point_attn[0] = inst->point_attenuation[1];
	inst->info.point_attn[1] = inst->point_attenuation[3];
	inst->info.spot_attn[0] = inst->spot_attenuation[0] + inst->spot_attenuation[1];
	inst->info.spot_attn[1] = inst->spot_attenuation[0];
	inst->info.spot_attn[2] = inst->spot_attenuation[2];
}

static void
spot_light_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleSpotLightInstance *spot = SEHLE_SPOT_LIGHT_FROM_MATERIAL_INSTANCE (inst);
	EleaVec3f l_dir, l_pos;
	sehle_light_bind_common (&spot->light_inst, ctx);

	elea_mat3x4f_get_translation (&l_pos, &spot->light_inst.l2w);
	elea_mat3x4f_transform_point (&l_pos, &ctx->w2v, &l_pos);
	sehle_program_setUniform3fv (inst->programs[0], SEHLE_LIGHT_LIGHTPOS, 1, l_pos.c);
	elea_mat3x4f_get_col_vec (&l_dir, &spot->light_inst.l2w, 2);
	l_dir = elea_vec3f_inv (l_dir);
	elea_mat3x4f_transform_vec3 (&l_dir, &ctx->w2v, &l_dir);
	sehle_program_setUniform3fv (inst->programs[0], SEHLE_LIGHT_LIGHTDIR, 1, l_dir.c);
	sehle_program_setUniform4fv (inst->programs[0], SEHLE_LIGHT_POINT_ATTENUATION, 1, spot->light_inst.point_attenuation);
	sehle_program_setUniform3fv (inst->programs[0], SEHLE_LIGHT_SPOT_ATTENUATION, 1, spot->light_inst.spot_attenuation);
}

static void
spot_light_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data)
{
	SehleSpotLightInstance *spot = SEHLE_SPOT_LIGHT_FROM_RENDERABLE_INSTANCE (inst);

	// We have to set composite transformation for each light separately
	EleaMat3x4f m;
	EleaMat4x4f m_proj;
	elea_mat3x4f_multiply (&m, &ctx->w2v, &spot->light_inst.l2w);
	elea_mat3x4f_scale_self_right_xyz (&m, spot->light_inst.point_attenuation[1], spot->light_inst.point_attenuation[1], spot->light_inst.point_attenuation[1]);
	elea_mat4x4f_multiply_mat3x4 (&m_proj, &ctx->proj, &m);
	sehle_program_setUniformMatrix4fv (spot->light_inst.material_inst.programs[0], SEHLE_LIGHT_L2W_W2V_PROJECTION, 1, m_proj.c);
	// Same for shadow lookup
	// sehle_program_setUniformMatrix4fv (inst->programs[0], E2S, 1, e2s[light]);
	sehle_vertex_array_render_triangles (spot->light_inst.va, 1, 0, spot->light_inst.va->ibuf->buffer.n_elements);
}

#define N_CORNERS 8

void
sehle_spot_light_setup (SehleSpotLightInstance *inst, SehleEngine *engine, float priority)
{
	sehle_material_setup (&inst->light_inst.material_inst, 1, 0);
	inst->light_inst.priority = priority;

	inst->light_inst.program_flags = SEHLE_PROGRAM_LIGHT_POINT | SEHLE_PROGRAM_LIGHT_SPOT;

	inst->light_inst.va = sehle_vertex_array_new (engine, NULL);
	SehleVertexBuffer *vb = sehle_engine_get_vertex_buffer (engine, NULL, SEHLE_BUFFER_STATIC);
	sehle_vertex_buffer_setup_attrs (vb, N_CORNERS + 2, SEHLE_ATTRIBUTE_VERTEX, 3, -1);
	sehle_vertex_array_set_vertex_data (inst->light_inst.va, 0, vb);
	az_object_unref (( AZObject*) vb);
	SehleIndexBuffer *ib = sehle_engine_get_index_buffer (engine, NULL, SEHLE_BUFFER_STATIC);
	sehle_index_buffer_resize (ib, 6 * N_CORNERS);
	unsigned int* indices = sehle_index_buffer_map (ib, SEHLE_BUFFER_WRITE);
	for (unsigned int i = 0; i < N_CORNERS; i++) {
		indices[6 * i + 0] = i + 1;
		indices[6 * i + 1] = (i + 1) % N_CORNERS + 1;
		indices[6 * i + 2] = 0;
		indices[6 * i + 3] = (i + 1) % N_CORNERS + 1;
		indices[6 * i + 4] = i + 1;
		indices[6 * i + 5] = N_CORNERS;
	}
	sehle_index_buffer_unmap (ib);
	sehle_vertex_array_set_index_data (inst->light_inst.va, ib);
	az_object_unref ((AZObject *) ib);
}

static void
spot_light_build_volume (SehleSpotLightInstance *inst, float *vertices, unsigned int stride_floats)
{
	EleaVec3f v;
	float radius = inst->light_inst.point_attenuation[1];
	float cos_beta_2 = inst->light_inst.spot_attenuation[0];
	if (cos_beta_2 < 0.01f) cos_beta_2 = 0.01f;
	if (cos_beta_2 > 1) cos_beta_2 = 1;
	float sin_beta_2 = sqrtf (1 - cos_beta_2 * cos_beta_2);
	float l1 = radius / cos_beta_2;
	float l2 = radius * cos_beta_2;
	float r = radius * sin_beta_2;
	float alpha_2 = ELEA_M_PI_F / N_CORNERS;
	float r1 = r / cosf (alpha_2);
	memcpy(vertices, &EleaVec3f0, 12);
	for (unsigned int i = 0; i < N_CORNERS; i++) {
		float phi = i * ELEA_M_2PI_F / N_CORNERS;
		elea_vec3fp_set_xyz (&v, r1 * cosf (phi), r1 * sinf (phi), -l2);
		memcpy(vertices + (i + 1) * stride_floats, &v, 12);
	}
	elea_vec3fp_set_xyz (&v, 0, 0, -l1);
	memcpy(vertices + (N_CORNERS + 1) * stride_floats, &v, 12);
}

void
sehle_spot_light_set_point_attenuation (SehleSpotLightInstance *inst, float min_distance, float inner_radius, float outer_radius, float power)
{
	arikkei_return_if_fail (inst != NULL);
	inst->light_inst.point_attenuation[0] = min_distance;
	inst->light_inst.point_attenuation[1] = outer_radius;
	inst->light_inst.point_attenuation[2] = outer_radius - inner_radius;
	inst->light_inst.point_attenuation[3] = power;
}

void
sehle_spot_light_set_spot_attenuation (SehleSpotLightInstance *inst, float inner_angle, float outer_angle, float power)
{
	arikkei_return_if_fail (inst != NULL);
	inst->light_inst.spot_attenuation[0] = cosf (outer_angle);
	inst->light_inst.spot_attenuation[1] = cosf (inner_angle) - cosf (outer_angle);
	inst->light_inst.spot_attenuation[2] = power;
}

void
sehle_spot_light_update_geometry (SehleSpotLightInstance *inst)
{
	arikkei_return_if_fail (inst != NULL);
	float *v = sehle_vertex_buffer_map (inst->light_inst.va->vbufs[0], SEHLE_BUFFER_WRITE);
	spot_light_build_volume (inst, v, 3);
	sehle_vertex_buffer_unmap (inst->light_inst.va->vbufs[0]);
}

void
sehle_spot_light_update_visuals(SehleSpotLightInstance *spot)
{
	spot->light_inst.renderable_inst.bbox = EleaAABox3fEmpty;
	EleaVec3f v[N_CORNERS + 2];
	spot_light_build_volume (spot, v[0].c, 3);
	for (unsigned int i = 0; i < (N_CORNERS + 2); i++) {
		elea_aabox3f_grow_p_mat(&spot->light_inst.renderable_inst.bbox, &spot->light_inst.renderable_inst.bbox, &v[i], &spot->light_inst.l2w);
	}
}
