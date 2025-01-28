#define __SEHLE_COMMONMATERIALS_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2015
//

#include <stdio.h>

#include <arikkei/arikkei-iolib.h>

#include "GL/glew.h"

#include "engine.h"
#include "texture.h"
#include <sehle/render-context.h>
#include "shader.h"
#include "program.h"
#include "vertex-buffer.h"

#include "material-depth.h"

SehleShader *
depth_get_vertex_shader (SehleEngine *engine, unsigned int has_texture)
{
	char c[256];
	sprintf (c, "Sehle::DepthProgram::Vertex_T%d", has_texture != 0);
	SehleShader *shader = sehle_engine_get_shader (engine, c, SEHLE_SHADER_VERTEX);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const unsigned char *sources[2];
		unsigned int size, is_file;
		sprintf (c, "#version 140\n#define VS\n#define HAS_TEXTURE %d\n", has_texture);
		sources[0] = (const unsigned char *) c;
		sources[1] = sehle_shader_map ((const unsigned char *) "depth-vertex.txt", &size, &is_file);
		sehle_shader_build (shader, sources, 2, NULL);
		if (is_file) arikkei_munmap (sources[1], size);
	}
	return shader;
}

SehleShader *
depth_get_fragment_shader (SehleEngine *engine, unsigned int has_texture)
{
	char c[256];
	sprintf (c, "Sehle::DepthProgram::Fragment_T%d", has_texture != 0);
	SehleShader *shader = sehle_engine_get_shader (engine, c, SEHLE_SHADER_FRAGMENT);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const unsigned char *sources[2];
		unsigned int size, is_file;
		sprintf (c, "#version 140\n#define FS\n#define HAS_TEXTURE %d\n", has_texture);
		sources[0] = (const unsigned char *) c;
		sources[1] = sehle_shader_map ((const unsigned char *) "depth-fragment.txt", &size, &is_file);
		sehle_shader_build (shader, sources, 2, NULL);
		if (is_file) arikkei_munmap (sources[1], size);
	}
	return shader;
}

static SehleShader *
depth_get_shader (SehleEngine *engine, unsigned int shader_type, unsigned int flags, unsigned int max_instances)
{
	char c[256];
	sprintf (c, "Sehle::DepthProgram::%s_C%uT%uB%u_I%u",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment",
		(flags & SEHLE_PROGRAM_DEPTH_HAS_COLORS) != 0,
		(flags & SEHLE_PROGRAM_DEPTH_HAS_TEXTURE) != 0,
		(flags & SEHLE_PROGRAM_DEPTH_HAS_BONES) != 0,
		max_instances);
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const char *sources[1];
		sprintf (c, "#version 140\n"
			"#define %s\n"
			"#define HAS_COLORS %u\n"
			"#define HAS_TEXTURE %u\n"
			"#define HAS_SKIN %d\n"
			"#define MAX_INSTANCES %u\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS",
			(flags & SEHLE_PROGRAM_DEPTH_HAS_COLORS) != 0,
			(flags & SEHLE_PROGRAM_DEPTH_HAS_TEXTURE) != 0,
			(flags & SEHLE_PROGRAM_DEPTH_HAS_BONES) != 0,
			max_instances);
		sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "depth-vertex.txt" : "depth-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

SehleProgram *
sehle_depth_program_get_reference (SehleEngine *engine, unsigned int flags, unsigned int max_instances)
{
	char c[256];
	sprintf (c, "Sehle::DepthProgram_C%uT%uB%u_I%u",
		(flags & SEHLE_PROGRAM_DEPTH_HAS_COLORS) != 0,
		(flags & SEHLE_PROGRAM_DEPTH_HAS_TEXTURE) != 0,
		(flags & SEHLE_PROGRAM_DEPTH_HAS_BONES) != 0,
		max_instances);
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 1, SEHLE_PROGRAM_DEPTH_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		const char *uniforms[] = { "diffuse", "opacity", "color_sampler" };
		sehle_program_add_shader (prog, depth_get_shader (engine, SEHLE_SHADER_VERTEX, flags, max_instances));
		sehle_program_add_shader (prog, depth_get_shader (engine, SEHLE_SHADER_FRAGMENT, flags, max_instances));
		sehle_program_set_uniform_names (prog, 0, SEHLE_NUM_UNIFORMS, (const unsigned char **) sehle_uniforms);
		sehle_program_set_uniform_names (prog, SEHLE_NUM_UNIFORMS, SEHLE_PROGRAM_DEPTH_NUM_UNIFORMS - SEHLE_NUM_UNIFORMS, (const unsigned char **) uniforms);
	}
	return prog;
}

void
sehle_program_depth_bind_instance (SehleProgram *prog, SehleRenderContext *ctx, unsigned int render_type)
{
	EleaPlane3f clip_view;
	elea_mat3x4f_transform_plane (&clip_view, &ctx->w2v, &ctx->clip_plane);
	sehle_program_setUniform4fv (prog, SEHLE_UNIFORM_CLIP_PLANE, 1, clip_view.c);
}

void
sehle_program_depth_render (SehleProgram *prog, SehleVertexArray *va, unsigned int first_index, unsigned int n_indices, const EleaMat3x4f *o2v, const EleaMat4x4f *proj)
{
	sehle_program_setUniformMatrix4fv (prog, SEHLE_UNIFORM_PROJECTION, 1, proj->c);
	sehle_program_setUniformMatrix4x3fv (prog, SEHLE_UNIFORM_O2V, 1, 1, o2v->c);
	SEHLE_CHECK_ERRORS (0);
	sehle_vertex_array_render_triangles (va, 1, first_index, n_indices);
}

