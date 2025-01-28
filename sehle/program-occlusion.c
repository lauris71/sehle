#define __SEHLE_PROGRAM_OCCLUSION_CPP__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

#include <stdio.h>

#include "engine.h"
#include "index-buffer.h"
#include <sehle/render-context.h>
#include "shader.h"
#include "vertex-buffer.h"

#include "program-occlusion.h"

static SehleShader *
occlusion_get_shader (SehleEngine *engine, unsigned int shader_type, unsigned int flags, unsigned int max_instances)
{
	char c[256];
	sprintf (c, "Sehle::OcclusionProgram::%s__I%u",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment",
		max_instances);
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const char *sources[1];
		sprintf (c, "#version 140\n"
			"#define %s\n"
			"#define MAX_INSTANCES %u\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS",
			max_instances);
		sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "occlusion-vertex.txt" : "occlusion-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

static const char *uniforms[] = {
	"o2v", "projection",
	"map_rprojection", "viewportinv",
	"depthSampler", 
	"v2o", "radius", "inner_radius", "strength"
};

SehleProgram *
sehle_program_occlusion_get_reference (SehleEngine *engine, unsigned int flags, unsigned int max_instances)
{
	char c[256];
	sprintf (c, "Sehle::OcclusionProgram__I%u",
		max_instances);
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 2, SEHLE_OCCLUSION_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, occlusion_get_shader (engine, SEHLE_SHADER_VERTEX, flags, max_instances));
		sehle_program_add_shader (prog, occlusion_get_shader (engine, SEHLE_SHADER_FRAGMENT, flags, max_instances));
		SehleShader *shader = sehle_shader_fetch_from_file (engine, "encodeGBuffer.txt", SEHLE_SHADER_FRAGMENT);
		sehle_program_add_shader (prog, shader);
		sehle_program_set_uniform_names (prog, 0, SEHLE_OCCLUSION_NUM_UNIFORMS, (const unsigned char **) uniforms);
	}
	return prog;
}

void
sehle_program_occlusion_bind_instance (SehleProgram *prog, SehleRenderContext *ctx, float radius, float inner_radius, float strength)
{
	sehle_program_setUniform1f (prog, SEHLE_OCCLUSION_RADIUS, radius);
	sehle_program_setUniform1f (prog, SEHLE_OCCLUSION_INNER_RADIUS, inner_radius);
	sehle_program_setUniform1f (prog, SEHLE_OCCLUSION_STRENGTH, strength);
}

void
sehle_program_occlusion_render (SehleProgram *prog, SehleRenderContext *ctx, unsigned int n_instances, const EleaMat3x4f *i2w, SehleVertexArray *va)
{
	sehle_render_context_set_gbuffer_uniforms (ctx, prog, SEHLE_OCCLUSION_W2G, SEHLE_OCCLUSION_G2D_RPROJECTION);

	if (ctx->depthchannel >= 0) sehle_program_setUniform1i (prog, SEHLE_OCCLUSION_DEPTH_SAMPLER, ctx->depthchannel);

	sehle_program_setUniformMatrix4fv (prog, SEHLE_OCCLUSION_PROJECTION, 1, ctx->proj.c);

	sehle_vertex_array_bind (va);
	SEHLE_CHECK_ERRORS (0);
	for (unsigned int i = 0; i < n_instances; i += 16) {
		static EleaMat3x4f o2v[16], v2o[16];
		unsigned int batch_size = n_instances - i;
		if (batch_size > 16) batch_size = 16;
		for (unsigned int j = 0; j < batch_size; j++) {
			elea_mat3x4f_multiply (&o2v[j], &ctx->w2v, &i2w[i + j]);
			// fixme: This is wrong because we may have scale
			elea_mat3x4f_invert_normalized (&v2o[j], &o2v[j]);
		}
		sehle_program_setUniformMatrix4x3fv (prog, SEHLE_OCCLUSION_O2V, batch_size, 1, o2v[0].c);
		sehle_program_setUniformMatrix4x3fv (prog, SEHLE_OCCLUSION_V2O, batch_size, 1, v2o[0].c);
		sehle_vertex_array_render_triangles_instanced (va, 0, 0, va->ibuf->buffer.n_elements, batch_size);
	}
}

