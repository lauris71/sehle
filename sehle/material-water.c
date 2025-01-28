#define __SEHLE_MATERIAL_WATER_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2011
//

#include <stdio.h>

#include "GL/glew.h"

#include <arikkei/arikkei-utils.h>

#include "engine.h"
#include "texture.h"
#include <sehle/render-context.h>
#include <sehle/material-depth.h>
#include "program.h"
#include "texture-2d.h"
#include "vertex-buffer.h"

#include <sehle/shader.h>
#include <sehle/material-water.h>

enum SehleWaterUniforms {
	SEHLE_WATER_MAX_DEPTH = SEHLE_NUM_UNIFORMS,
	SEHLE_WATER_W2G, SEHLE_WATER_G2E_RPROJECTION, SEHLE_WATER_DEPTH_SAMPLER, SEHLE_WATER_WAVE_MATRIX, SEHLE_WATER_WAVE_SAMPLER,
	SEHLE_WATER_EYE_OBJECT, SEHLE_WATER_O2V_PROJECTION_REFLECTION, SEHLE_WATER_REFLECTION_SAMPLER,
	SEHLE_WATER_OPAQUE_COLOR, SEHLE_WATER_OPAQUE_DEPTH, SEHLE_WATER_MIN_OPACITY, SEHLE_WATER_MIN_REFLECTIVITY, SEHLE_WATER_MAX_REFLECTIVITY,
	SEHLE_WATER_RIPPLE_POSITION, SEHLE_WATER_RIPPLE_SHAPE,
	SEHLE_WATER_NUM_UNIFORMS
};

static const char *uniform_names[] = {
	"max_depth",
	"w2g", "g2e_rprojection", "depthSampler", "wave_matrix", "waveSampler",
	"eye_object", "o2v_projection_reflection", "reflection_sampler",
	"opaque_color", "opaque_depth", "min_opacity", "min_reflectivity", "max_reflectivity",
	"ripple_position", "ripple_shape"
};

enum Programs {
	PROGRAM_TRANSPARENT,
	NUM_PROGRAMS
};

#define NUM_TEXTURES 2

static SehleShader *
water_get_shader (SehleEngine *engine, unsigned int shader_type, unsigned int flags, unsigned int num_ripples)
{
	char c[256];
	sprintf (c, "Sehle::Water::%s_D%uNR%u",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment",
		(flags & SEHLE_PROGRAM_WATER_HAS_DEPTH) != 0,
		num_ripples);
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const char *sources[1];
		sprintf (c, "#version 140\n#define %s\n#define DEPTH %d\n#define NUM_RIPPLES %u\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS",
			(flags & SEHLE_PROGRAM_WATER_HAS_DEPTH) != 0,
			num_ripples);
		sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "water-vertex.txt" : "water-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

SehleProgram *
sehle_program_water_get_reference (SehleEngine *engine, unsigned int flags, unsigned int num_ripples)
{
	char c[256];
	sprintf (c, "Sehle::Water::D%uNR%u",
		(flags & SEHLE_PROGRAM_WATER_HAS_DEPTH) != 0,
		num_ripples);
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 1, SEHLE_WATER_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, water_get_shader (engine, SEHLE_SHADER_VERTEX, flags, num_ripples));
		sehle_program_add_shader (prog, water_get_shader (engine, SEHLE_SHADER_FRAGMENT, flags, num_ripples));
		sehle_program_set_uniform_names (prog, 0, SEHLE_NUM_UNIFORMS, (const unsigned char **) sehle_uniforms);
		sehle_program_set_uniform_names (prog, SEHLE_NUM_UNIFORMS, SEHLE_WATER_NUM_UNIFORMS - SEHLE_NUM_UNIFORMS, (const unsigned char **) uniform_names);
	}
	return prog;
}

/* AZInstance implementation */
static void material_water_class_init (SehleMaterialWaterClass *klass);
static void material_water_init (SehleMaterialWaterClass *klass, SehleMaterialWater *mat);
/* SehleMaterial implementation */
static void material_water_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);
//static void material_water_render (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, SehleRenderData *rd, unsigned int render_type);

unsigned int sehle_material_water_type = 0;
SehleMaterialWaterClass *sehle_material_water_class;

unsigned int
sehle_material_water_get_type (void)
{
	if (!sehle_material_water_type) {
		az_register_type (&sehle_material_water_type, (const unsigned char *) "SehleMaterialWater", AZ_TYPE_BLOCK,
			sizeof (SehleMaterialWaterClass), sizeof (SehleMaterialWater), 0,
			(void (*) (AZClass *)) material_water_class_init,
			(void (*) (const AZImplementation *, void *)) material_water_init,
			NULL);
		sehle_material_water_class = (SehleMaterialWaterClass *) az_type_get_class (sehle_material_water_type);
	}
	return sehle_material_water_type;
}

static void
material_water_class_init (SehleMaterialWaterClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_MATERIAL_REFLECTING,
		ARIKKEI_OFFSET(SehleMaterialWaterClass, reflecting_impl),
		ARIKKEI_OFFSET(SehleMaterialWater, reflecting_inst));
	klass->reflecting_impl.material_impl.bind = material_water_bind;
	//klass->reflecting_impl.material_impl.render = material_water_render;
}

static void
material_water_init (SehleMaterialWaterClass *klass, SehleMaterialWater *mwater)
{
	sehle_material_setup (&mwater->reflecting_inst.material_inst, NUM_PROGRAMS, NUM_TEXTURES);
	// Do not render backside of water
	sehle_render_flags_set (&mwater->reflecting_inst.material_inst.state_flags, SEHLE_CULL | SEHLE_BLEND);
	/* Depth write HAS TO BE DISABLED because we read from shared depth texture */
	sehle_render_flags_clear (&mwater->reflecting_inst.material_inst.state_flags, SEHLE_DEPTH_WRITE);
	mwater->reflecting_inst.material_inst.render_stages = SEHLE_STAGE_TRANSPARENT | SEHLE_STAGE_REFLECTIONS;
	mwater->reflecting_inst.material_inst.render_types = SEHLE_RENDER_TRANSPARENT;

	mwater->maxDepth = 1;
	elea_color4fp_set_rgba (&mwater->color, 0.19f, 0.13f, 0.11f, 1.0f);
	mwater->opaqueDepth = 0.5f;
	mwater->minOpacity = 0.2f;
	mwater->minReflectivity = 0.2f;
	mwater->maxReflectivity = 1;

	for (int i = 0; i < 8; i++) {
		mwater->ripples[i].center = EleaVec2f0;
		mwater->ripples[i].wavelength = 1;
		mwater->ripples[i].phase = 0;
		mwater->ripples[i].amplitude = 0;
	}

	mwater->wave_matrix[0] = EleaMat3x4fIdentity;
	mwater->wave_matrix[1] = EleaMat3x4fIdentity;
}

static void
material_water_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleMaterialWater *mwater = SEHLE_MATERIAL_WATER_FROM_MATERIAL_INSTANCE (inst);
	if (render_type == SEHLE_RENDER_TRANSPARENT) {
		if (!mwater->reflecting_inst.material_inst.programs[PROGRAM_TRANSPARENT]) {
			mwater->reflecting_inst.material_inst.programs[PROGRAM_TRANSPARENT] = sehle_program_water_get_reference (ctx->engine, SEHLE_PROGRAM_WATER_HAS_DEPTH, 8);
		}
		SehleProgram *prog = inst->programs[PROGRAM_TRANSPARENT];
		sehle_render_context_set_program (ctx, prog);

		sehle_render_context_set_gbuffer_uniforms (ctx, prog, SEHLE_WATER_W2G, SEHLE_WATER_G2E_RPROJECTION);

		if (ctx->depthchannel >= 0) {
			sehle_program_setUniform1i (prog, SEHLE_WATER_DEPTH_SAMPLER, ctx->depthchannel);
		}

		float wm[18];
		for (int i = 0; i < 2; i++) {
			for (int r = 0; r < 3; r++) {
				for (int c = 0; c < 3; c++) {
					wm[i * 9 + c * 3 + r] = mwater->wave_matrix[i].c[r * 4 + c + (c == 2)];
				}
			}
		}
		sehle_program_setUniformMatrix3fv (prog, SEHLE_WATER_WAVE_MATRIX, 2, wm);
		sehle_material_instance_bind_texture (inst, ctx, PROGRAM_TRANSPARENT, SEHLE_MATERIAL_WATER_TEXTURE_WAVES, SEHLE_WATER_WAVE_SAMPLER);

		sehle_material_instance_bind_texture (inst, ctx, PROGRAM_TRANSPARENT, SEHLE_MATERIAL_REFLECTING_TEXTURE_REFLECTION, SEHLE_WATER_REFLECTION_SAMPLER);
		sehle_program_setUniformMatrix4fv (prog, SEHLE_WATER_O2V_PROJECTION_REFLECTION, 1, mwater->reflecting_inst.o2v_projection.c);
		sehle_program_setUniform1f (prog, SEHLE_WATER_MAX_DEPTH, mwater->maxDepth);
		EleaColor4f ambient;
		for (unsigned int i = 0; i < 4; i++) {
			ambient.c[i] = mwater->color.c[i] * ctx->global_ambient.c[i];
		}
		sehle_program_setUniform3fv (prog, SEHLE_WATER_OPAQUE_COLOR, 1, ambient.c);
		sehle_program_setUniform1f (prog, SEHLE_WATER_OPAQUE_DEPTH, mwater->opaqueDepth);
		sehle_program_setUniform1f (prog, SEHLE_WATER_MIN_OPACITY, mwater->minOpacity);
		sehle_program_setUniform1f (prog, SEHLE_WATER_MIN_REFLECTIVITY, mwater->minReflectivity);
		sehle_program_setUniform1f (prog, SEHLE_WATER_MAX_REFLECTIVITY, mwater->maxReflectivity);
		EleaVec4f v[8];
		for (int i = 0; i < 8; i++) elea_vec4fp_set_xyzw (&v[i], mwater->ripples[i].center.x, mwater->ripples[i].center.y, mwater->ripples[i].start, mwater->ripples[i].end);
		sehle_program_setUniform4fv (prog, SEHLE_WATER_RIPPLE_POSITION, 8, v[0].c);
		EleaVec3f w[8];
		for (int i = 0; i < 8; i++) elea_vec3fp_set_xyz (&w[i], mwater->ripples[i].amplitude, mwater->ripples[i].wavelength, mwater->ripples[i].phase);
		sehle_program_setUniform3fv (prog, SEHLE_WATER_RIPPLE_SHAPE, 8, w[0].c);
	}
}

#if 0
static void
material_water_render (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, SehleRenderData *rd, unsigned int render_type)
{
	SehleMaterialWater *water_inst = SEHLE_MATERIAL_WATER_FROM_MATERIAL_INSTANCE (inst);

	const EleaMat3x4f *i2w = (const EleaMat3x4f *) rd->instances;
	EleaMat3x4f o2v;
	if (render_type == SEHLE_RENDER_TRANSPARENT) {
		SehleProgram *prog = inst->programs[PROGRAM_TRANSPARENT];
		EleaMat4x4f o2v4, o2v_proj;
		elea_mat3x4f_multiply (&o2v, &ctx->w2v, &i2w[0]);
		elea_mat4x4f_multiply_mat3x4 (&o2v_proj, &ctx->proj, &o2v);
		sehle_program_setUniformMatrix4fv (prog, SEHLE_UNIFORM_O2V, 1, o2v4.c);
		sehle_program_setUniformMatrix4fv (prog, SEHLE_UNIFORM_O2V_PROJECTION, 1, o2v_proj.c);
		EleaMat3x4f v2o;
		elea_mat3x4f_invert_normalized (&v2o, &o2v);
		EleaVec3f t;
		elea_mat3x4f_get_translation (&t, &v2o);
		sehle_program_setUniform3fv (prog, SEHLE_WATER_EYE_OBJECT, 1, t.c);

		sehle_vertex_array_bind (rd->va);
		SEHLE_CHECK_ERRORS (0);
		glDrawElements (GL_TRIANGLES, rd->n_indices, GL_UNSIGNED_INT, ( uint32_t*) NULL + rd->first);
		glBindVertexArray (0);
	}
}
#endif

void
sehle_material_water_set_waves (SehleMaterialWater *mwater, SehleTexture2D *waves)
{
	sehle_material_set_texture (&mwater->reflecting_inst.material_inst, SEHLE_MATERIAL_WATER_TEXTURE_WAVES, (SehleTexture *) waves);
}
