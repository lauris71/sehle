#define __SEHLE_PARTICLE_RENDERER_CPP__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2012-2016
//

#include <stdio.h>

#include <az/class.h>

#include "GL/glew.h"

#include <sehle/index-buffer.h>
#include "engine.h"
#include <sehle/render-context.h>
#include <sehle/program.h>
#include <sehle/shader.h>
#include "vertex-buffer.h"

#include <sehle/particle-renderer.h>

enum SehleParticleUniforms {
	PARTICLE_VP2T = SEHLE_NUM_UNIFORMS,
	PARTICLE_MAP_RPROJECTION, PARTICLE_DEPTH_SAMPLER,
	PARTICLE_PROJECTION, PARTICLE_O2V_ROTATION, PARTICLE_O2V_POSITION, PARTICLE_SAMPLER, PARTICLE_GRADIENT_SAMPLER, PARTICLE_COLORS,
	SEHLE_PARTICLE_NUM_UNIFORMS
};

static const char *uniform_names[] = {
	"vp2t", "map_rprojection", "depthSampler",
	"particleSampler", "gradientSampler", "colors"
};

static SehleShader *
particle_get_shader (SehleEngine *engine, unsigned int shader_type)
{
	char c[256];
	sprintf (c, "Sehle::Particles::%s",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment");
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const char *sources[1];
		sprintf (c, "#version 140\n#define %s\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS");
		sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "particles-vertex.txt" : "particles-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

SehleProgram *
sehle_program_particle_get_reference (SehleEngine *engine)
{
	char c[256];
	sprintf (c, "Sehle::Particles");
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 1, SEHLE_PARTICLE_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, particle_get_shader (engine, SEHLE_SHADER_VERTEX));
		sehle_program_add_shader (prog, particle_get_shader (engine, SEHLE_SHADER_FRAGMENT));
		sehle_program_set_uniform_names (prog, 0, SEHLE_NUM_UNIFORMS, ( const unsigned char **) sehle_uniforms);
		sehle_program_set_uniform_names (prog, SEHLE_NUM_UNIFORMS, SEHLE_PARTICLE_NUM_UNIFORMS - SEHLE_NUM_UNIFORMS, (const unsigned char **) uniform_names);
	}
	return prog;
}

static void particle_class_init (SehleParticleRendererClass *klass);
static void particle_init (SehleParticleRendererClass *klass, SehleParticleRenderer *particle);
/* SehleMaterial implementation */
static void particle_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);

unsigned int sehle_particle_renderer_type = 0;
SehleParticleRendererClass *sehle_particle_renderer_class;

unsigned int
sehle_particle_renderer_get_type (void)
{
	if (!sehle_particle_renderer_type) {
		az_register_type (&sehle_particle_renderer_type, (const unsigned char *) "SehleParticleRenderer", AZ_TYPE_BLOCK, sizeof (SehleParticleRendererClass), sizeof (SehleParticleRenderer), AZ_CLASS_ZERO_MEMORY,
			(void (*) (AZClass *)) particle_class_init,
			(void (*) (const AZImplementation *, void *)) particle_init,
			NULL);
		sehle_particle_renderer_class = (SehleParticleRendererClass *) az_type_get_class (sehle_particle_renderer_type);
	}
	return sehle_particle_renderer_type;
}

static void
particle_class_init (SehleParticleRendererClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_MATERIAL,
		ARIKKEI_OFFSET(SehleParticleRendererClass, material_implementation),
		ARIKKEI_OFFSET(SehleParticleRenderer, material_instance));
	klass->material_implementation.bind = particle_bind;
}

static void
particle_init (SehleParticleRendererClass *klass, SehleParticleRenderer *particle)
{
	sehle_render_flags_clear (&particle->material_instance.state_flags, SEHLE_DEPTH_WRITE);
	sehle_render_flags_set_depth_test (&particle->material_instance.state_flags, 0, SEHLE_DEPTH_ALWAYS);
	particle->material_instance.render_stages = SEHLE_STAGE_TRANSPARENT;
	particle->material_instance.render_types = SEHLE_RENDER_TRANSPARENT;
	for (int i = 0; i < 4; i++) particle->colors[i] = EleaColor4fWhite;
	particle->zbias = 0;
}

static void
particle_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleParticleRenderer *particle = (SehleParticleRenderer *) inst;
	SehleProgram *prog = inst->programs[0];
	sehle_render_context_set_program (ctx, prog);
	sehle_render_context_set_gbuffer_uniforms (ctx, prog, PARTICLE_VP2T, PARTICLE_MAP_RPROJECTION);
	sehle_program_setUniformMatrix4fv (prog, SEHLE_UNIFORM_PROJECTION, 1, ctx->proj.c);
	// Depth sampler
	sehle_program_setUniform1i (prog, PARTICLE_DEPTH_SAMPLER, ctx->depthchannel);
	sehle_material_instance_bind_texture (inst, ctx, 0, 0, PARTICLE_SAMPLER);
	sehle_material_instance_bind_texture (inst, ctx, 0, 1, PARTICLE_GRADIENT_SAMPLER);
	sehle_program_setUniform4fv (prog, PARTICLE_COLORS, 4, particle->colors[0].c);
}

#if 0
static void
particle_render (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, SehleRenderData *rd, unsigned int render_type)
{
	SehleParticleRenderer *particle = (SehleParticleRenderer *) inst;
	SehleProgram *prog = inst->programs[0];
	const EleaMat3x4f *i2w = (const EleaMat3x4f *) rd->instances;
	EleaMat3x4f o2v;
	elea_mat3x4f_multiply (&o2v, &ctx->w2v, i2w);
	sehle_program_setUniformMatrix4x3fv (prog, SEHLE_UNIFORM_O2V, 1, 1, o2v.c);

	sehle_vertex_array_bind (rd->va);
	SEHLE_CHECK_ERRORS (0);
	glDrawElements (GL_TRIANGLES, rd->n_indices, GL_UNSIGNED_INT, (uint32_t *) NULL + rd->first);
}
#endif

void
sehle_particle_renderer_setup (SehleParticleRenderer *rend, SehleEngine *engine)
{
	sehle_material_setup (&rend->material_instance, 1, 0);
	rend->material_instance.programs[0] = sehle_program_particle_get_reference (engine);
}

void
sehle_particle_renderer_release (SehleParticleRenderer *rend)
{
	sehle_material_release (&rend->material_instance);
}

void
sehle_particle_renderer_set_texture (SehleParticleRenderer *rend, unsigned int tex_idx, SehleTexture2D *tex)
{
	sehle_material_set_texture (&rend->material_instance, tex_idx, (SehleTexture *) tex);
}
