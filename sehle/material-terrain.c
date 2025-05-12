#define __SEHLE_MATERIAL_TERRAIN_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2012
//

#include <stdio.h>

#include <az/class.h>

#include "GL/glew.h"

#include <sehle/material-depth.h>
#include "engine.h"
#include "program.h"
#include <sehle/render-context.h>
#include "shader.h"
#include "shaders-dict.h"
#include "texture-2d.h"
#include "vertex-buffer.h"

#include "material-terrain.h"

static SehleShader *
terrain_get_shader (SehleEngine *engine, unsigned int shader_type, unsigned int flags)
{
	char c[256];
	sprintf (c, "Sehle::TerrainProgram::%s_M%uN%u",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment",
		(flags & SEHLE_PROGRAM_TERRAIN_HAS_MAP_1) != 0,
		(flags & SEHLE_PROGRAM_TERRAIN_NOISE) != 0);
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const char *sources[1];
		sprintf (c, "#version 140\n"
			"#define %s\n"
			"#define MAP_1 %u\n"
			"#define NOISE %u\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS",
			(flags & SEHLE_PROGRAM_TERRAIN_HAS_MAP_1) != 0,
			(flags & SEHLE_PROGRAM_TERRAIN_HAS_NOISE) != 0);
		sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "terrain-gbuf-vertex.txt" : "terrain-gbuf-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

static const char *uniforms[] = {
	"mapSampler", "noiseSampler", "textureSampler", "mapping", "scale"
};

SehleProgram *
sehle_program_terrain_get_reference (SehleEngine *engine, unsigned int flags)
{
	char c[256];
	sprintf (c, "Sehle::TerrainProgram_M%uN%u",
		(flags & SEHLE_PROGRAM_TERRAIN_HAS_MAP_1) != 0,
		(flags & SEHLE_PROGRAM_TERRAIN_HAS_NOISE) != 0);
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 2, SEHLE_PROGRAM_TERRAIN_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, terrain_get_shader (engine, SEHLE_SHADER_VERTEX, flags));
		sehle_program_add_shader (prog, terrain_get_shader (engine, SEHLE_SHADER_FRAGMENT, flags));
		SehleShader *shader = sehle_shader_fetch_from_file (engine, "encodeGBuffer.txt", SEHLE_SHADER_FRAGMENT);
		sehle_program_add_shader (prog, shader);
		sehle_program_set_uniform_names (prog, 0, SEHLE_NUM_UNIFORMS, (const unsigned char **) sehle_uniforms);
		sehle_program_set_uniform_names (prog, SEHLE_NUM_UNIFORMS, SEHLE_PROGRAM_TERRAIN_NUM_UNIFORMS - SEHLE_NUM_UNIFORMS, (const unsigned char **) uniforms);
	}
	return prog;
}

enum {
	TERRAIN_PROGRAM_DEPTH,
	TERRAIN_PROGRAM_GBUFFER,
	TERRAIN_NUM_PROGRAMS
};

static void material_terrain_class_init (SehleMaterialTerrainClass *klass);
static void material_terrain_init (SehleMaterialTerrainClass *klass, SehleMaterialTerrain *dns);
/* Material implementation */
static void material_terrain_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);

static void material_terrain_update_program_flags (SehleMaterialTerrain *tmat);
static void material_terrain_update_programs (SehleMaterialInstance *inst, SehleEngine *engine, unsigned int flags);

unsigned int sehle_material_terrain_type = 0;
SehleMaterialTerrainClass *sehle_material_terrain_class;

unsigned int
sehle_material_terrain_get_type (void)
{
	if (!sehle_material_terrain_type) {
		az_register_type (&sehle_material_terrain_type, (const unsigned char *) "SehleMaterialTerrain", AZ_TYPE_BLOCK, sizeof (SehleMaterialTerrainClass), sizeof (SehleMaterialTerrain), AZ_FLAG_ZERO_MEMORY,
			(void (*) (AZClass *)) material_terrain_class_init,
			(void (*) (const AZImplementation *, void *)) material_terrain_init,
			NULL);
		sehle_material_terrain_class = (SehleMaterialTerrainClass *) az_type_get_class (sehle_material_terrain_type);
	}
	return sehle_material_terrain_type;
}

static void
material_terrain_class_init (SehleMaterialTerrainClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_MATERIAL,
		ARIKKEI_OFFSET(SehleMaterialTerrainClass, material_implementation),
		ARIKKEI_OFFSET(SehleMaterialTerrain, material_instance));
	klass->material_implementation.bind = material_terrain_bind;
}

static void
material_terrain_init (SehleMaterialTerrainClass *klass, SehleMaterialTerrain *tmat)
{
	sehle_material_setup (&tmat->material_instance, TERRAIN_NUM_PROGRAMS, SEHLE_MATERIAL_TERRAIN_NUM_SAMPLERS);
	tmat->material_instance.render_stages = SEHLE_STAGE_SOLID;
	tmat->material_instance.render_types = SEHLE_RENDER_DEPTH | SEHLE_RENDER_GBUFFER;
	tmat->program_flags = 0;
	for (unsigned int i = 0; i < SEHLE_MATERIAL_TERRAIN_NUM_GROUND_TEXTURES; i++) {
		tmat->mapping[i] = SEHLE_MATERIAL_TERRAIN_MAPPING_GENERATED;
		tmat->scale[i] = 1;
	}
	tmat->num_maps = 1;
}

static void
terrain_bind_textures (SehleMaterialTerrain *tmat, SehleRenderContext *ctx)
{
	sehle_material_instance_bind_texture (&tmat->material_instance, ctx, TERRAIN_PROGRAM_GBUFFER, SEHLE_MATERIAL_TERRAIN_TEXTURE_MAP_0, -1);
	unsigned int num_grounds = 5;
	if (tmat->num_maps > 1) {
		sehle_material_instance_bind_texture (&tmat->material_instance, ctx, TERRAIN_PROGRAM_GBUFFER, SEHLE_MATERIAL_TERRAIN_TEXTURE_MAP_1, -1);
		num_grounds = 9;
	}
	int ch[SEHLE_MATERIAL_TERRAIN_NUM_GROUND_TEXTURES];
	for (int i = 0; i < 2; i++) {
		ch[i] = (tmat->material_instance.channels[i] >= 0) ? tmat->material_instance.channels[i] : 0;
	}
	sehle_program_setUniform1iv (tmat->material_instance.programs[TERRAIN_PROGRAM_GBUFFER], SEHLE_PROGRAM_TERRAIN_MAP, tmat->num_maps, ch);

	sehle_material_instance_bind_texture (&tmat->material_instance, ctx, TERRAIN_PROGRAM_GBUFFER, SEHLE_MATERIAL_TERRAIN_TEXTURE_NOISE, SEHLE_PROGRAM_TERRAIN_NOISE);
	for (unsigned int i = 0; i < num_grounds; i++) {
		sehle_material_instance_bind_texture (&tmat->material_instance, ctx, TERRAIN_PROGRAM_GBUFFER, SEHLE_MATERIAL_TERRAIN_TEXTURE_GROUND_0 + i, -1);
	}
	for (unsigned int i = 0; i < num_grounds; i++) {
		ch[i] = (tmat->material_instance.channels[SEHLE_MATERIAL_TERRAIN_TEXTURE_GROUND_0 + i] >= 0) ? tmat->material_instance.channels[SEHLE_MATERIAL_TERRAIN_TEXTURE_GROUND_0 + i] : 0;
	}
	sehle_program_setUniform1iv (tmat->material_instance.programs[TERRAIN_PROGRAM_GBUFFER], SEHLE_PROGRAM_TERRAIN_TEXTURE, num_grounds, ch);
}

static void
material_terrain_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleMaterialTerrain *tmat = SEHLE_MATERIAL_TERRAIN_FROM_MATERIAL_INSTANCE (inst);
	if (!tmat->material_instance.program_initialized) {
		material_terrain_update_programs (&tmat->material_instance, ctx->engine, tmat->program_flags);
	}

	if (render_type == SEHLE_RENDER_DEPTH) {
		sehle_render_context_set_program (ctx, inst->programs[TERRAIN_PROGRAM_DEPTH]);
		sehle_program_depth_bind_instance (inst->programs[TERRAIN_PROGRAM_DEPTH], ctx, render_type);
	} else if (render_type == SEHLE_RENDER_GBUFFER) {
		unsigned int pidx = TERRAIN_PROGRAM_GBUFFER;
		SehleProgram *prog = tmat->material_instance.programs[pidx];
		sehle_render_context_set_program (ctx, prog);
		EleaPlane3f clip_view;
		elea_mat3x4f_transform_plane (&clip_view, &ctx->w2v, &ctx->clip_plane);
		sehle_program_setUniform4fv (prog, SEHLE_UNIFORM_CLIP_PLANE, 1, clip_view.c);
		terrain_bind_textures (tmat, ctx);
		sehle_program_setUniform1iv (prog, SEHLE_PROGRAM_TERRAIN_MAPPING, SEHLE_MATERIAL_TERRAIN_NUM_GROUND_TEXTURES, (const int *) tmat->mapping);
		sehle_program_setUniform1fv (prog, SEHLE_PROGRAM_TERRAIN_SCALE, SEHLE_MATERIAL_TERRAIN_NUM_GROUND_TEXTURES, tmat->scale);
	}
}

static void
material_terrain_update_program_flags (SehleMaterialTerrain *tmat)
{
	unsigned int new_flags = 0;
	if (tmat->num_maps > 1) new_flags |= SEHLE_PROGRAM_TERRAIN_HAS_MAP_1;
	if (tmat->material_instance.textures[SEHLE_MATERIAL_TERRAIN_TEXTURE_NOISE]) new_flags |= SEHLE_PROGRAM_TERRAIN_HAS_NOISE;
	if (new_flags != tmat->program_flags) {
		tmat->program_flags = new_flags;
		tmat->material_instance.program_initialized = 0;
	}
}

static void
material_terrain_update_programs (SehleMaterialInstance *inst, SehleEngine *engine, unsigned int flags)
{
	if (inst->programs[TERRAIN_PROGRAM_DEPTH]) az_object_unref (AZ_OBJECT (inst->programs[TERRAIN_PROGRAM_DEPTH]));
	unsigned int depth_flags = 0;
	inst->programs[TERRAIN_PROGRAM_DEPTH] = sehle_depth_program_get_reference (engine, depth_flags, 1);
	if (inst->programs[TERRAIN_PROGRAM_GBUFFER]) az_object_unref (AZ_OBJECT (inst->programs[TERRAIN_PROGRAM_GBUFFER]));
	inst->programs[TERRAIN_PROGRAM_GBUFFER] = sehle_program_terrain_get_reference (engine, flags);
	inst->program_initialized = 1;
}

SehleMaterialTerrain *
sehle_material_terrain_new (SehleEngine *engine)
{
	SehleMaterialTerrain *tmat = (SehleMaterialTerrain *) az_instance_new (SEHLE_TYPE_MATERIAL_TERRAIN);

	/* fixme: Bad things happen if map texture is NULL */
	//sehle_material_terrain_set_map_texture (tmat, 0, sehle_engine_get_standard_texture (engine, SehleEngine::TEXTURE_TRANSPARENT));
	//sehle_material_terrain_set_map_texture (tmat, 1, sehle_engine_get_standard_texture (engine, SehleEngine::TEXTURE_TRANSPARENT));
	//sehle_material_terrain_set_noise_texture (tmat, sehle_engine_get_standard_texture (engine, SehleEngine::TEXTURE_WHITE));

	return tmat;
}

void
sehle_material_terrain_delete (SehleMaterialTerrain *tmat)
{
	az_instance_delete (SEHLE_TYPE_MATERIAL_TERRAIN, tmat);
}

void
sehle_material_terrain_set_ground_texture (SehleMaterialTerrain *tmat, unsigned int idx, SehleTexture2D *texture, unsigned int mapping, float scale)
{
	sehle_material_set_texture (&tmat->material_instance, SEHLE_MATERIAL_TERRAIN_TEXTURE_GROUND_0 + idx, (SehleTexture *) texture);
	tmat->mapping[idx] = mapping;
	tmat->scale[idx] = scale;
}

void
sehle_material_terrain_set_map_texture (SehleMaterialTerrain *tmat, unsigned int idx, SehleTexture2D *texture)
{
	if (idx == 1) {
		if (texture) {
			tmat->num_maps = 2;
		} else {
			tmat->num_maps = 1;
		}
	}
	sehle_material_set_texture (&tmat->material_instance, SEHLE_MATERIAL_TERRAIN_TEXTURE_MAP_0 + idx, (SehleTexture *) texture);
	material_terrain_update_program_flags (tmat);
}

void
sehle_material_terrain_set_noise_texture (SehleMaterialTerrain *tmat, SehleTexture2D *texture)
{
	sehle_material_set_texture (&tmat->material_instance, SEHLE_MATERIAL_TERRAIN_TEXTURE_NOISE, (SehleTexture *) texture);
}

