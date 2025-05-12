#define __SEHLE_MATERIAL_FOLIAGE_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2011-2021
 */

#include <stdio.h>

#include <az/class.h>

#include <arikkei/arikkei-iolib.h>

#include "GL/glew.h"

#include "engine.h"
#include <sehle/index-buffer.h>
#include "texture.h"
#include <sehle/render-context.h>
#include <sehle/renderable.h>
#include "shader.h"
#include "program.h"
#include "texture-2d.h"
#include "vertex-buffer.h"
#include <sehle/material-depth.h>

#include "material-foliage.h"

static SehleShader *
foliage_get_shader (SehleEngine *engine, unsigned int shader_type, unsigned int flags, unsigned int max_instances)
{
	char c[256];
	sprintf (c, "Sehle::FoliageProgram::%s_N%uAO%uW%u_I%u",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment",
		(flags & SEHLE_PROGRAM_FOLIAGE_HAS_NORMAL) != 0,
		(flags & SEHLE_PROGRAM_FOLIAGE_HAS_AMBIENT_OCCLUSION) != 0,
		(flags & SEHLE_PROGRAM_FOLIAGE_HAS_WIND) != 0,
		max_instances);
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const char *sources[1];
		sprintf (c, "#version 140\n"
			"#define %s\n"
			"#define MAX_INSTANCES %u\n"
			"#define HAS_NORMAL %u\n"
			"#define HAS_AMBIENT_OCCLUSION %u\n"
			"#define HAS_WIND %u\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS",
			max_instances,
			(flags & SEHLE_PROGRAM_FOLIAGE_HAS_NORMAL) != 0,
			(flags & SEHLE_PROGRAM_FOLIAGE_HAS_AMBIENT_OCCLUSION) != 0,
			(flags & SEHLE_PROGRAM_FOLIAGE_HAS_WIND) != 0);
		sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "foliage-gbuf-vertex.txt" : "foliage-gbuf-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

static const char *uniforms[] = {
	"view_normal", "normal_factor", "time", "colorSampler", "windSampler"
};

SehleProgram *
sehle_program_foliage_get_reference (SehleEngine *engine, unsigned int flags, unsigned int max_instances)
{
	char c[256];
	sprintf (c, "Sehle::FoliageProgram_N%uAO%uW%u_I%u",
		(flags & SEHLE_PROGRAM_FOLIAGE_HAS_NORMAL) != 0,
		(flags & SEHLE_PROGRAM_FOLIAGE_HAS_AMBIENT_OCCLUSION) != 0,
		(flags & SEHLE_PROGRAM_FOLIAGE_HAS_WIND) != 0,
		max_instances);
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 3, SEHLE_PROGRAM_FOLIAGE_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, foliage_get_shader (engine, SEHLE_SHADER_VERTEX, flags, max_instances));
		sehle_program_add_shader (prog, foliage_get_shader (engine, SEHLE_SHADER_FRAGMENT, flags, max_instances));
		SehleShader *shader = sehle_shader_fetch_from_file (engine, "findTangentSpace.txt", SEHLE_SHADER_FRAGMENT);
		sehle_program_add_shader (prog, shader);
		shader = sehle_shader_fetch_from_file (engine, "encodeGBuffer.txt", SEHLE_SHADER_FRAGMENT);
		sehle_program_add_shader (prog, shader);
		sehle_program_set_uniform_names (prog, 0, SEHLE_NUM_UNIFORMS, (const unsigned char **) sehle_uniforms);
		sehle_program_set_uniform_names (prog, SEHLE_NUM_UNIFORMS, SEHLE_PROGRAM_FOLIAGE_NUM_UNIFORMS - SEHLE_NUM_UNIFORMS, (const unsigned char **) uniforms);
	}
	return prog;
}

void
sehle_program_foliage_bind_instance (SehleProgram *prog, unsigned int flags, SehleRenderContext *ctx, unsigned int is_first_of_class, const float *world_normal, float normal_factor)
{
	EleaVec3f view_normal;
	elea_mat3x4f_transform_vec3 (&view_normal, &ctx->w2v, (EleaVec3f *) world_normal);
	sehle_program_setUniform3fv (prog, SEHLE_PROGRAM_FOLIAGE_VIEW_NORMAL, 1, view_normal.c);
	sehle_program_setUniform1f (prog, SEHLE_PROGRAM_FOLIAGE_NORMAL_FACTOR, normal_factor);
	sehle_program_setUniform1f (prog, SEHLE_PROGRAM_FOLIAGE_TIME, prog->resource.engine->time);
}

enum {
	FOLIAGE_PROGRAM_DEPTH,
	FOLIAGE_PROGRAM_GBUFFER,
	FOLIAGE_NUM_PROGRAMS
};

static void material_foliage_class_init (SehleMaterialFoliageClass *klass);
static void material_foliage_init (SehleMaterialFoliageClass *klass, SehleMaterialFoliage *flg);

static void material_foliage_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);

static void material_foliage_update_programs (SehleMaterialInstance *inst, SehleEngine *engine, unsigned int flags);

static unsigned int material_foliage_type = 0;
SehleMaterialFoliageClass *sehle_material_foliage_class;

unsigned int
sehle_material_foliage_get_type (void)
{
	if (!material_foliage_type) {
		az_register_type (&material_foliage_type, (const unsigned char *) "SehleMaterialFoliage", AZ_TYPE_BLOCK, sizeof (SehleMaterialFoliageClass), sizeof (SehleMaterialFoliage), AZ_FLAG_ZERO_MEMORY,
			(void (*) (AZClass *)) material_foliage_class_init,
			(void (*) (const AZImplementation *, void *)) material_foliage_init,
			NULL);
		sehle_material_foliage_class = (SehleMaterialFoliageClass *) az_type_get_class (material_foliage_type);
	}
	return material_foliage_type;
}

static void
material_foliage_class_init (SehleMaterialFoliageClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_MATERIAL,
		ARIKKEI_OFFSET(SehleMaterialFoliageClass, material_implementation),
		ARIKKEI_OFFSET(SehleMaterialFoliage, material_instance));
	klass->material_implementation.bind = material_foliage_bind;
}

static void
material_foliage_init (SehleMaterialFoliageClass *klass, SehleMaterialFoliage *flg)
{
	sehle_material_setup (&flg->material_instance, FOLIAGE_NUM_PROGRAMS, SEHLE_MATERIAL_FOLIAGE_TEXTURE_NUM_TEXTURES);
	flg->material_instance.render_stages = SEHLE_STAGE_SOLID;
	flg->material_instance.render_types = SEHLE_RENDER_DEPTH | SEHLE_RENDER_GBUFFER;
	flg->world_normal = EleaVec3fZ;
}

static void
material_foliage_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleMaterialFoliage *flg = SEHLE_MATERIAL_FOLIAGE_FROM_MATERIAL_INSTANCE (inst);
	if (!flg->material_instance.program_initialized) {
		material_foliage_update_programs (&flg->material_instance, ctx->engine, flg->program_flags);
	}
	if (render_type == SEHLE_RENDER_DEPTH) {
		unsigned int pidx = FOLIAGE_PROGRAM_DEPTH;
		SehleProgram *prog = flg->material_instance.programs[pidx];
		sehle_render_context_set_program (ctx, prog);
		sehle_material_instance_bind_texture (inst, ctx, pidx, 0, SEHLE_PROGRAM_DEPTH_COLOR_SAMPLER);
		/* fixme: Need wind here too */
		sehle_program_depth_bind_instance (prog, ctx, render_type);
	} else if (render_type == SEHLE_RENDER_GBUFFER) {
		unsigned int pidx = FOLIAGE_PROGRAM_GBUFFER;
		SehleProgram *prog = flg->material_instance.programs[pidx];
		sehle_render_context_set_program (ctx, prog);
		sehle_material_instance_bind_texture (inst, ctx, pidx, 0, SEHLE_PROGRAM_FOLIAGE_COLOR_SAMPLER);
		sehle_material_instance_bind_texture (inst, ctx, pidx, 0, SEHLE_PROGRAM_FOLIAGE_WIND_SAMPLER);
		sehle_program_foliage_bind_instance (prog, flg->program_flags, ctx, 1, flg->world_normal.c, flg->normal_factor);
	}
}

static void
material_foliage_update_programs (SehleMaterialInstance *inst, SehleEngine *engine, unsigned int flags)
{
	if (inst->programs[FOLIAGE_PROGRAM_DEPTH]) az_object_unref (AZ_OBJECT (inst->programs[FOLIAGE_PROGRAM_DEPTH]));
	inst->programs[FOLIAGE_PROGRAM_DEPTH] = sehle_depth_program_get_reference (engine, SEHLE_PROGRAM_DEPTH_HAS_TEXTURE, 16);
	if (inst->programs[FOLIAGE_PROGRAM_GBUFFER]) az_object_unref (AZ_OBJECT (inst->programs[FOLIAGE_PROGRAM_GBUFFER]));
	inst->programs[FOLIAGE_PROGRAM_GBUFFER] = sehle_program_foliage_get_reference (engine, flags, 16);
	inst->program_initialized = 1;
}

SehleMaterialFoliage *
sehle_material_foliage_new (SehleEngine *engine)
{
	SehleMaterialFoliage *flg = (SehleMaterialFoliage *) az_instance_new (SEHLE_TYPE_MATERIAL_FOLIAGE);
	return flg;
}

void
sehle_material_foliage_delete (SehleMaterialFoliage *flg)
{
	az_instance_delete (SEHLE_TYPE_MATERIAL_FOLIAGE, flg);
}

void
sehle_material_foliage_update_program_flags (SehleMaterialFoliage *flg)
{
	unsigned int new_flags = 0;
	if (flg->material_instance.textures[SEHLE_MATERIAL_FOLIAGE_TEXTURE_WIND] != NULL) new_flags |= SEHLE_PROGRAM_FOLIAGE_HAS_WIND;
	if (flg->normal_factor != 0) new_flags |= SEHLE_PROGRAM_FOLIAGE_HAS_NORMAL;
	if (flg->has_ao) new_flags |= SEHLE_PROGRAM_FOLIAGE_HAS_AMBIENT_OCCLUSION;
	if (new_flags != flg->program_flags) {
		flg->program_flags = new_flags;
		flg->material_instance.program_initialized = 0;
	}
}

void
sehle_material_foliage_set_texture (SehleMaterialFoliage *mflg, unsigned int texture_type, SehleTexture2D *tex2d)
{
	sehle_material_set_texture (&mflg->material_instance, texture_type, (SehleTexture *) tex2d);
	sehle_material_foliage_update_program_flags (mflg);
}

