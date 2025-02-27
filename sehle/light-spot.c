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

static void spot_light_build_volume (SehleSpotLightInstance *inst, EleaVec3f *vertices, unsigned int stride_floats);

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
	inst->info.point_attn[0] = inst->point_attn.radius;
	inst->info.point_attn[1] = inst->point_attn.falloff;
	inst->info.spot_attn[0] = inst->spot_attn.inner_cos;
	inst->info.spot_attn[1] = inst->spot_attn.outer_cos;
	inst->info.spot_attn[2] = inst->spot_attn.falloff;
}

static void
spot_light_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleSpotLightInstance *spot = SEHLE_SPOT_LIGHT_FROM_MATERIAL_INSTANCE (inst);
	EleaVec3f l_dir, l_pos;
	sehle_light_bind_common (&spot->light_inst, ctx);

	/* Light position */
	EleaVec3f p_world, p_eye;
	elea_mat3x4f_get_translation(&p_world, &spot->light_inst.l2w);
	elea_mat3x4f_transform_point(&p_eye, &ctx->w2v, &p_world);
	EleaVec4f light_pos = {p_eye.x, p_eye.y, p_eye.z, 1};
	sehle_program_setUniform4fv (inst->programs[0], SEHLE_LIGHT_LIGHTPOS, 1, light_pos.c);

	elea_mat3x4f_get_col_vec (&l_dir, &spot->light_inst.l2w, 2);
	l_dir = elea_vec3f_inv (l_dir);
	elea_mat3x4f_transform_vec3 (&l_dir, &ctx->w2v, &l_dir);
	sehle_program_setUniform3fv (inst->programs[0], SEHLE_LIGHT_LIGHTDIR, 1, l_dir.c);

	sehle_program_setUniform3fv (inst->programs[0], SEHLE_LIGHT_POINT_ATTENUATION, 1, spot->light_inst.point_attn.c);
	sehle_program_setUniform3fv (inst->programs[0], SEHLE_LIGHT_SPOT_ATTENUATION, 1, spot->light_inst.spot_attn.c);
}

static void
spot_light_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data)
{
	SehleSpotLightInstance *spot = SEHLE_SPOT_LIGHT_FROM_RENDERABLE_INSTANCE (inst);

	// We have to set composite transformation for each light separately
	EleaMat3x4f m;
	EleaMat4x4f m_proj;
	elea_mat3x4f_multiply (&m, &ctx->w2v, &spot->light_inst.l2w);
	//elea_mat3x4f_scale_self_right_xyz (&m, 5, 5, 5);
	elea_mat4x4f_multiply_mat3x4 (&m_proj, &ctx->proj, &m);
	sehle_program_setUniformMatrix4fv (spot->light_inst.material_inst.programs[0], SEHLE_LIGHT_L2W_W2V_PROJECTION, 1, m_proj.c);
	// Same for shadow lookup
	// sehle_program_setUniformMatrix4fv (inst->programs[0], E2S, 1, e2s[light]);
	sehle_vertex_array_render_triangles (spot->light_inst.va, 1, 0, spot->light_inst.va->ibuf->buffer.n_elements);
}

#define N_CORNERS 20
#define N_VERTICES (2 * N_CORNERS + 2)
#define N_INDICES (3 * 4 * N_CORNERS)

void
sehle_spot_light_setup (SehleSpotLightInstance *inst, SehleEngine *engine, float priority)
{
	sehle_material_setup (&inst->light_inst.material_inst, 1, 0);
	inst->light_inst.priority = priority;

	inst->light_inst.program_flags = SEHLE_PROGRAM_LIGHT_POINT | SEHLE_PROGRAM_LIGHT_SPOT;

	inst->light_inst.va = sehle_vertex_array_new (engine, NULL);
	SehleVertexBuffer *vb = sehle_engine_get_vertex_buffer (engine, NULL, SEHLE_BUFFER_STATIC);
	sehle_vertex_buffer_setup_attrs (vb, N_VERTICES + 2, SEHLE_ATTRIBUTE_VERTEX, 3, -1);
	SehleIndexBuffer *ib = sehle_engine_get_index_buffer (engine, NULL, SEHLE_BUFFER_STATIC);
	sehle_index_buffer_resize (ib, N_INDICES);
	unsigned int* indices = sehle_index_buffer_map (ib, SEHLE_BUFFER_WRITE);
	for (unsigned int i = 0; i < N_CORNERS; i++) {
		unsigned int *idx = indices + 12 * i;
		unsigned int n = (i + 1) % N_CORNERS;
		idx[0] = 0;
		idx[1] = 1 + n;
		idx[2] = 1 + i;

		idx[3] = 1 + i;
		idx[4] = 1 + n;
		idx[5] = 1 + N_CORNERS + n;

		idx[6] = 1 + i;
		idx[7] = 1 + N_CORNERS + n;
		idx[8] = 1 + N_CORNERS + i;

		idx[9] = 1 + N_CORNERS + n;
		idx[10] = 1 + N_CORNERS + N_CORNERS;
		idx[11] = 1 + N_CORNERS + i;
	}
	sehle_index_buffer_unmap (ib);
	sehle_vertex_array_set_index_data (inst->light_inst.va, ib);
	sehle_vertex_array_set_vertex_data (inst->light_inst.va, 0, vb);
	az_object_unref ((AZObject *) vb);
	az_object_unref ((AZObject *) ib);
}

static void
spot_light_build_volume (SehleSpotLightInstance *inst, EleaVec3f *v, unsigned int stride_floats)
{
	float outer_cos = inst->light_inst.spot_attn.outer_cos;
	float alpha = acosf(outer_cos) * 2;
	float beta = (float) (2 * M_PI / N_CORNERS);
	float cos_a_2 = outer_cos;
	float sin_a_2 = sinf(alpha / 2);
	float cos_a_4 = cosf(alpha / 4);
	float tan_a_2 = sin_a_2 / cos_a_2;
	float cos_b_2 = cosf(beta / 2);
	float outer_dist = inst->light_inst.point_attn.radius;
	float inner_dist = inst->light_inst.point_attn.min_dist;
	float inner_adjusted = inner_dist * cos_a_2;

	unsigned int vidx = 0;

	/* Inner cap */
	float di = inner_dist;
	float ri = di * tan_a_2;
	float ri_p = ri / cos_b_2;
	v[vidx++] = (EleaVec3f) {0, 0, -di};
	for (unsigned int i = 0; i < N_CORNERS; i++) {
		float phi = i * ELEA_M_2PI_F / N_CORNERS;
		float x = ri_p * cosf(phi);
		float y = ri_p * sinf(phi);
		v[vidx++] = (EleaVec3f) {x, y, -di};
	}
	/* Outer cap */
	float Dmax = outer_dist;
	float Dmax_p = Dmax / cos_a_4;
	float r_o = Dmax_p * sin_a_2;
	float ro_p = r_o / cos_b_2;
	float d_o = Dmax_p * cos_a_2;
	for (unsigned int i = 0; i < N_CORNERS; i++) {
		float phi = i * ELEA_M_2PI_F / N_CORNERS;
		float x = ro_p * cosf(phi);
		float y = ro_p * sinf(phi);
		v[vidx++] = (EleaVec3f) {x, y, -d_o};
	}
	v[vidx++] = (EleaVec3f) {0, 0, -Dmax_p};
}

void
sehle_spot_light_set_point_attenuation (SehleSpotLightInstance *inst, float min_dist, float radius, float falloff)
{
	arikkei_return_if_fail (inst != NULL);
	inst->light_inst.point_attn.min_dist = min_dist;
	inst->light_inst.point_attn.radius = radius;
	inst->light_inst.point_attn.falloff = falloff;
}

void
sehle_spot_light_set_spot_attenuation (SehleSpotLightInstance *inst, float inner_angle, float outer_angle, float falloff)
{
	arikkei_return_if_fail (inst != NULL);
	inst->light_inst.spot_attn.inner_cos = cosf (inner_angle);
	inst->light_inst.spot_attn.outer_cos = cosf (outer_angle);
	inst->light_inst.spot_attn.falloff = falloff;
}

void
sehle_spot_light_update_geometry (SehleSpotLightInstance *inst)
{
	arikkei_return_if_fail (inst != NULL);
	float *v = sehle_vertex_buffer_map (inst->light_inst.va->vbufs[0], SEHLE_BUFFER_WRITE);
	spot_light_build_volume (inst, (EleaVec3f *) v, 3);
	sehle_vertex_buffer_unmap (inst->light_inst.va->vbufs[0]);
}

void
sehle_spot_light_update_visuals(SehleSpotLightInstance *spot)
{
	spot->light_inst.renderable_inst.bbox = EleaAABox3fEmpty;
	float sin_a_2 = sqrtf(1 - spot->light_inst.spot_attn.outer_cos * spot->light_inst.spot_attn.outer_cos);
	EleaVec3f p;
	/* Inner */
	float r = sin_a_2 * spot->light_inst.point_attn.min_dist;
	elea_mat3x4f_transform_point_xyz(&p, &spot->light_inst.l2w, 0, 0, -spot->light_inst.point_attn.min_dist);
	elea_aabox3f_grow_xyz(&spot->light_inst.renderable_inst.bbox, &spot->light_inst.renderable_inst.bbox, p.x - r, p.y - r, p.z - r);
	elea_aabox3f_grow_xyz(&spot->light_inst.renderable_inst.bbox, &spot->light_inst.renderable_inst.bbox, p.x + r, p.y + r, p.z + r);
	/* Outer */
	r = sin_a_2 * spot->light_inst.point_attn.radius;
	elea_mat3x4f_transform_point_xyz(&p, &spot->light_inst.l2w, 0, 0, -spot->light_inst.point_attn.radius);
	elea_aabox3f_grow_xyz(&spot->light_inst.renderable_inst.bbox, &spot->light_inst.renderable_inst.bbox, p.x - r, p.y - r, p.z - r);
	elea_aabox3f_grow_xyz(&spot->light_inst.renderable_inst.bbox, &spot->light_inst.renderable_inst.bbox, p.x + r, p.y + r, p.z + r);
}
