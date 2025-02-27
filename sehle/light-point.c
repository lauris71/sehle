#define __SEHLE_LIGHT_POINT_C__

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

#include "light-point.h"

static void point_light_implementation_init (SehlePointLightImplementation *impl);

/* Light implementation */
static void pointl_setup_forward (SehleLightImplementation *impl, SehleLightInstance *inst, SehleRenderContext *ctx);
/* Material implementation */
static void point_light_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);
static void point_light_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data);

enum Attributes {
	VERTEX, NUM_ATTRIBUTES
};

static const char *attribute_names[] = {
	"vertex"
};

static unsigned int point_light_type = 0;
SehlePointLightClass *point_light_class = NULL;

unsigned int
sehle_point_light_get_type(void)
{
	if (!point_light_type) {
		point_light_class = (SehlePointLightClass *) az_register_interface_type (&point_light_type, (const unsigned char *) "SehlePointLight", SEHLE_TYPE_LIGHT,
			sizeof (SehlePointLightClass), sizeof (SehlePointLightImplementation), sizeof (SehlePointLightInstance), 0,
			NULL,
			(void(*) (AZImplementation *)) point_light_implementation_init,
			NULL, NULL);
	}
	return point_light_type;
}

static void
point_light_implementation_init (SehlePointLightImplementation *impl)
{
	impl->light_impl.setup_forward = pointl_setup_forward;
	/* Material */
	impl->light_impl.material_impl.bind = point_light_bind;
	impl->light_impl.renderable_impl.render = point_light_render;
}

static void
pointl_setup_forward (SehleLightImplementation *impl, SehleLightInstance *inst, SehleRenderContext *ctx)
{
	SehlePointLightImplementation *dirl_impl = (SehlePointLightImplementation *) impl;
	SehlePointLightInstance *dirl_inst = (SehlePointLightInstance *) inst;
	EleaVec3f p_world, p_eye;
	elea_mat3x4f_get_translation(&p_world, &inst->l2w);
	elea_mat3x4f_transform_point(&p_eye, &ctx->w2v, &p_world);
	inst->info.pos = elea_vec4f_from_xyzw(p_eye.x, p_eye.y, p_eye.z, 1);
	inst->info.point_attn[0] = inst->point_attn.radius;
	inst->info.point_attn[1] = inst->point_attn.falloff;
}

static void
point_light_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehlePointLightInstance *point = SEHLE_POINT_LIGHT_FROM_MATERIAL_INSTANCE (inst);
	EleaVec3f l_pos;
	sehle_light_bind_common (&point->light_inst, ctx);

	/* Light position */
	EleaVec3f p_world, p_eye;
	elea_mat3x4f_get_translation(&p_world, &point->light_inst.l2w);
	elea_mat3x4f_transform_point(&p_eye, &ctx->w2v, &p_world);
	EleaVec4f light_pos = {p_eye.x, p_eye.y, p_eye.z, 1};
	sehle_program_setUniform4fv (inst->programs[0], SEHLE_LIGHT_LIGHTPOS, 1, light_pos.c);

	sehle_program_setUniform3fv (inst->programs[0], SEHLE_LIGHT_POINT_ATTENUATION, 1, point->light_inst.point_attn.c);
}

static void
point_light_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data)
{
	SehlePointLightInstance *point = SEHLE_POINT_LIGHT_FROM_RENDERABLE_INSTANCE (inst);

	// We have to set composite transformation for each light separately
	EleaMat3x4f m;
	EleaMat4x4f m_proj;
	elea_mat3x4f_multiply (&m, &ctx->w2v, &point->light_inst.l2w);
	elea_mat3x4f_scale_self_right_xyz (&m, point->light_inst.point_attn.radius, point->light_inst.point_attn.radius, point->light_inst.point_attn.radius);
	elea_mat4x4f_multiply_mat3x4 (&m_proj, &ctx->proj, &m);
	sehle_program_setUniformMatrix4fv (point->light_inst.material_inst.programs[0], SEHLE_LIGHT_L2W_W2V_PROJECTION, 1, m_proj.c);
	// Same for shadow lookup
	// sehle_program_setUniformMatrix4fv (inst->programs[0], E2S, 1, e2s[light]);
	//glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_ALWAYS);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	sehle_vertex_array_render_triangles (point->light_inst.va, 1, 0, point->light_inst.va->ibuf->buffer.n_elements);
	//glCullFace(GL_BACK);
}

void
sehle_point_light_setup (SehlePointLightInstance *inst, SehleEngine *engine, float priority)
{
	sehle_material_setup (&inst->light_inst.material_inst, 1, 0);
	inst->light_inst.priority = priority;

	inst->light_inst.program_flags = SEHLE_PROGRAM_LIGHT_POINT;

	inst->light_inst.va = sehle_engine_get_standard_geometry (engine, SEHLE_GEOMETRY_UNIT_CUBE_INSIDE);
}

void
sehle_point_light_set_point_attenuation (SehlePointLightInstance *point, float min_dist, float radius, float falloff)
{
	arikkei_return_if_fail (point != NULL);
	point->light_inst.point_attn.min_dist = min_dist;
	point->light_inst.point_attn.radius = radius;
	point->light_inst.point_attn.falloff = falloff;
}

void
sehle_point_light_update_visuals(SehlePointLightInstance *point)
{
	EleaVec3f c;
	elea_mat3x4f_get_translation(&c, &point->light_inst.l2w);
	EleaVec3f p0 = elea_vec3f_sub_xyz(c, point->light_inst.point_attn.radius, point->light_inst.point_attn.radius, point->light_inst.point_attn.radius);
	EleaVec3f p1 = elea_vec3f_add_xyz(c, point->light_inst.point_attn.radius, point->light_inst.point_attn.radius, point->light_inst.point_attn.radius);
	elea_aabox3f_set_minmax(&point->light_inst.renderable_inst.bbox, &p0, &p1);
}
