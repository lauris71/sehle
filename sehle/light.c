#define __SEHLE_LIGHT_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2009
//

#include <stdlib.h>
#include <stdio.h>

#include <az/class.h>

#include "engine.h"
#include <sehle/render-context.h>
#include "shader.h"
#include "program.h"
#include "light.h"

#include <sehle/index-buffer.h>
#include <sehle/vertex-array.h>

static const char *uniform_names[] = {
	"l2w_w2v_projection", "map_rprojection", "viewportinv",
	"depthSampler", "normalSampler", "albedoSampler", "specularSampler",
	"lightpos", "lightdir",
	"ambient", "diffuse", "direct",
	"point_attenuation", "spot_attenuation",
	"splits", "eye2shadow", "shadowSampler", "densitySampler"
};

static SehleShader *
light_get_shader (SehleEngine *engine, unsigned int shader_type, unsigned int flags, unsigned int num_splits)
{
	char c[256];
	sprintf (c, "Sehle::Light::%s_D%dP%dS%dS%dD%dS%d",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment",
		(flags & SEHLE_PROGRAM_LIGHT_DIRECTIONAL) != 0,
		(flags & SEHLE_PROGRAM_LIGHT_POINT) != 0,
		(flags & SEHLE_PROGRAM_LIGHT_SPOT) != 0,
		(flags & SEHLE_PROGRAM_LIGHT_HAS_SHADOW) != 0,
		(flags & SEHLE_PROGRAM_LIGHT_HAS_DENSITY) != 0,
		num_splits);
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const char *sources[1];
		sprintf (c, "#version 140\n#define %s\n#define DIRECTIONAL %d\n#define POINT %d\n#define SPOT %d\n#define HAS_SHADOW %d\n#define HAS_DENSITY %d\n#define NUM_SPLITS %d\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS",
			(flags & SEHLE_PROGRAM_LIGHT_DIRECTIONAL) != 0,
			(flags & SEHLE_PROGRAM_LIGHT_POINT) != 0,
			(flags & SEHLE_PROGRAM_LIGHT_SPOT) != 0,
			(flags & SEHLE_PROGRAM_LIGHT_HAS_SHADOW) != 0,
			(flags & SEHLE_PROGRAM_LIGHT_HAS_DENSITY) != 0,
			num_splits);
		sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "light-vertex.txt" : "light-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

SehleProgram *
sehle_program_light_get_reference (SehleEngine *engine, unsigned int flags, unsigned int num_splits)
{
	/* Directional flag is incompatible with point and spot */
	arikkei_return_val_if_fail (!(flags & SEHLE_PROGRAM_LIGHT_DIRECTIONAL) || !(flags & (SEHLE_PROGRAM_LIGHT_POINT | SEHLE_PROGRAM_LIGHT_SPOT)), NULL);
	/* Every spotlight is also a pointlight */
	arikkei_return_val_if_fail (!(flags & SEHLE_PROGRAM_LIGHT_SPOT) || (flags & SEHLE_PROGRAM_LIGHT_POINT), NULL);
	/* Splits are only meaningful for directional lights */
	arikkei_return_val_if_fail ((flags & SEHLE_PROGRAM_LIGHT_DIRECTIONAL) || !num_splits, NULL);
	if (!(flags & (SEHLE_PROGRAM_LIGHT_HAS_SHADOW | SEHLE_PROGRAM_LIGHT_HAS_DENSITY))) num_splits = 0;
	char c[256];
	sprintf (c, "Sehle::Light_D%dP%dS%dS%dD%dS%d",
		(flags & SEHLE_PROGRAM_LIGHT_DIRECTIONAL) != 0,
		(flags & SEHLE_PROGRAM_LIGHT_POINT) != 0,
		(flags & SEHLE_PROGRAM_LIGHT_SPOT) != 0,
		(flags & SEHLE_PROGRAM_LIGHT_HAS_SHADOW) != 0,
		(flags & SEHLE_PROGRAM_LIGHT_HAS_DENSITY) != 0,
		num_splits);
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 2, SEHLE_LIGHT_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, light_get_shader (engine, SEHLE_SHADER_VERTEX, flags, num_splits));
		sehle_program_add_shader (prog, light_get_shader (engine, SEHLE_SHADER_FRAGMENT, flags, num_splits));
		SehleShader *shader = sehle_shader_fetch_from_file (engine, "decodeGBuffer.txt", SEHLE_SHADER_FRAGMENT);
		sehle_program_add_shader (prog, shader);
		sehle_program_set_uniform_names (prog, 0, SEHLE_LIGHT_NUM_UNIFORMS, (const unsigned char **) uniform_names);
	}
	return prog;
}


static void light_class_init (SehleLightClass *klass);
static void light_implementation_init (SehleLightImplementation *impl);
static void light_instance_init (SehleLightImplementation *impl, SehleLightInstance *inst);
static void light_instance_finalize (SehleLightImplementation *impl, SehleLightInstance *inst);

/* Renderable implementation */
static void light_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx);

unsigned int
sehle_light_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_interface_type (&type, (const unsigned char *) "SehleLight", SEHLE_TYPE_RENDERABLE,
			sizeof (SehleLightClass), sizeof (SehleLightImplementation), sizeof (SehleLightInstance), 0,
			(void (*) (AZClass *)) light_class_init,
			(void (*) (AZImplementation *)) light_implementation_init,
			(void (*) (const AZImplementation *, void *)) light_instance_init,
			(void (*) (const AZImplementation *, void *)) light_instance_finalize);
	}
	return type;
}

static void
light_class_init (SehleLightClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	/* Material */
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_MATERIAL,
		ARIKKEI_OFFSET(SehleLightImplementation, material_impl),
		ARIKKEI_OFFSET(SehleLightInstance, material_inst));
}

static void
light_implementation_init (SehleLightImplementation *impl)
{
	impl->renderable_impl.display = light_display;
}

static void
light_instance_init (SehleLightImplementation *impl, SehleLightInstance *inst)
{
	/* By default we are visible both in lighting and transparent pass but instances can change that */
	inst->renderable_inst.render_stages = SEHLE_STAGE_TRANSPARENT | SEHLE_STAGE_LIGHTS;
	sehle_render_flags_clear (&inst->material_inst.state_flags, SEHLE_DEPTH_WRITE);
	sehle_render_flags_set_depth_test (&inst->material_inst.state_flags, 1, SEHLE_DEPTH_GREATER);
	sehle_render_flags_set_blend (&inst->material_inst.state_flags, 1, SEHLE_BLEND_ADD, SEHLE_BLEND_ONE, SEHLE_BLEND_ONE);
	inst->material_inst.render_stages = SEHLE_STAGE_TRANSPARENT | SEHLE_STAGE_LIGHTS;
	inst->material_inst.render_types = SEHLE_RENDER_LIGHTMAP;
	inst->point_attenuation[0] = 0;
	inst->point_attenuation[1] = INFINITY;
	inst->point_attenuation[2] = 1;
	inst->point_attenuation[3] = 1;
	inst->spot_attenuation[0] = -INFINITY;
	inst->spot_attenuation[1] = 1;
	inst->spot_attenuation[2] = 1;
}

static void
light_instance_finalize (SehleLightImplementation *impl, SehleLightInstance *inst)
{
	if (inst->va) az_object_unref (( AZObject*) inst->va);
	if (inst->shadow) az_object_unref ((AZObject *) inst->shadow);
	if (inst->density) az_object_unref ((AZObject *) inst->density);
}

static void
light_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx)
{
	SehleLightImplementation *light_impl = SEHLE_LIGHT_FROM_RENDERABLE_IMPLEMENTATION(impl);
	SehleLightInstance *light_inst = SEHLE_LIGHT_FROM_RENDERABLE_INSTANCE(inst);
	if (displayctx->stages & SEHLE_STAGE_LIGHTS) {
		sehle_render_context_schedule_render (ctx, impl, inst, &light_impl->material_impl, &light_inst->material_inst, NULL);
	}
	if (displayctx->stages & SEHLE_STAGE_TRANSPARENT) {
		/* fixme: We should calculate matrices etc. here */
		sehle_render_context_add_light (ctx, light_inst);
	}
}

void
sehle_light_release (SehleLightInstance *inst)
{
	sehle_material_release (&inst->material_inst);
	az_object_unref ((AZObject*) inst->va);
	inst->va = NULL;
}

void
sehle_light_instance_set_shadow_texture (SehleLightInstance *inst, SehleTexture *shadow, SehleTexture *density)
{
	unsigned int new_flags;
	if (shadow != inst->shadow) {
		if (shadow) az_object_ref (AZ_OBJECT (shadow));
		if (inst->shadow) az_object_unref (AZ_OBJECT (inst->shadow));
		inst->shadow = shadow;
	}
	if (density != inst->density) {
		if (density) az_object_ref (AZ_OBJECT (density));
		if (inst->density) az_object_unref (AZ_OBJECT (inst->density));
		inst->density = density;
	}
	new_flags = inst->program_flags;
	if (inst->shadow) {
		new_flags |= SEHLE_PROGRAM_LIGHT_HAS_SHADOW;
	} else {
		new_flags &= ~SEHLE_PROGRAM_LIGHT_HAS_SHADOW;
	}
	if (inst->density) {
		new_flags |= SEHLE_PROGRAM_LIGHT_HAS_DENSITY;
	} else {
		new_flags &= ~SEHLE_PROGRAM_LIGHT_HAS_DENSITY;
	}
	if (new_flags != inst->program_flags) {
		inst->program_flags = new_flags;
		if (inst->material_inst.programs[0]) {
			az_object_unref (AZ_OBJECT (inst->material_inst.programs[0]));
			inst->material_inst.programs[0] = NULL;
		}
	}
}

void
sehle_light_bind_common (SehleLightInstance *inst, SehleRenderContext *ctx)
{
	/* Build program if needed */
	if (!inst->material_inst.programs[0]) {
		inst->material_inst.programs[0] = sehle_program_light_get_reference (ctx->engine, inst->program_flags, inst->num_splits);
	}
	/* Bind program */
	sehle_render_context_set_program (ctx, inst->material_inst.programs[0]);

	sehle_render_context_set_gbuffer_uniforms (ctx, inst->material_inst.programs[0], SEHLE_LIGHT_W2G, SEHLE_LIGHT_G2D_RPROJECTION);

	if (ctx->depthchannel >= 0) {
		sehle_program_setUniform1i (inst->material_inst.programs[0], SEHLE_LIGHT_DEPTH_SAMPLER, ctx->depthchannel);
	}
	if (ctx->normalchannel >= 0) {
		sehle_program_setUniform1i (inst->material_inst.programs[0], SEHLE_LIGHT_NORMAL_SAMPLER, ctx->normalchannel);
	}
	if (ctx->albedochannel >= 0) {
		sehle_program_setUniform1i (inst->material_inst.programs[0], SEHLE_LIGHT_ALBEDO_SAMPLER, ctx->albedochannel);
	}
	if (ctx->specularchannel >= 0) {
		sehle_program_setUniform1i (inst->material_inst.programs[0], SEHLE_LIGHT_SPECULAR_SAMPLER, ctx->specularchannel);
	}
	/* Colors */
	sehle_program_setUniform3fv (inst->material_inst.programs[0], SEHLE_LIGHT_AMBIENT, 1, inst->ambient.c);
	sehle_program_setUniform3fv (inst->material_inst.programs[0], SEHLE_LIGHT_DIFFUSE, 1, inst->diffuse.c);
	sehle_program_setUniform3fv (inst->material_inst.programs[0], SEHLE_LIGHT_DIRECT, 1, inst->direct.c);
}
