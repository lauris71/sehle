#define __SEHLE_COMMONMATERIALS_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2012
//

#include <stdio.h>

#include "GL/glew.h"

#include "engine.h"
#include "texture.h"
#include <sehle/render-context.h>
#include "program.h"
#include "texture-2d.h"

#include <sehle/index-buffer.h>
#include <sehle/shader.h>
#include <sehle/vertex-buffer.h>

#include <sehle/material-overlay.h>

static SehleShader *
overlay_get_shader (SehleEngine *engine, unsigned int shader_type, unsigned int flags, float min_alpha)
{
	char c[256];
	sprintf (c, "Sehle::OverlayProgram::%s_T%uC%uM%uD%uE%uA%.3f",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment",
		(flags & SEHLE_PROGRAM_OVERLAY_HAS_TEXTURE) != 0,
		(flags & SEHLE_PROGRAM_OVERLAY_HAS_COLORS) != 0,
		(flags & SEHLE_PROGRAM_OVERLAY_HAS_MASK) != 0,
		(flags & SEHLE_PROGRAM_OVERLAY_HAS_DEPTH) != 0,
		(flags & SEHLE_PROGRAM_OVERLAY_HAS_EXPOSURE) != 0,
		min_alpha);
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const char *sources[1];
		sprintf (c, "#version 140\n"
			"#define %s\n"
			"#define HAS_TEXTURE %u\n"
			"#define HAS_COLORS %u\n"
			"#define HAS_MASK %u\n"
			"#define HAS_DEPTH %u\n"
			"#define HAS_ALPHA %u\n"
			"#define HAS_EXPOSURE %u\n"
			"#define MIN_ALPHA %.3f\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS",
			(flags & SEHLE_PROGRAM_OVERLAY_HAS_TEXTURE) != 0,
			(flags & SEHLE_PROGRAM_OVERLAY_HAS_COLORS) != 0,
			(flags & SEHLE_PROGRAM_OVERLAY_HAS_MASK) != 0,
			(flags & SEHLE_PROGRAM_OVERLAY_HAS_DEPTH) != 0,
			min_alpha > 0,
			(flags & SEHLE_PROGRAM_OVERLAY_HAS_EXPOSURE) != 0,
			min_alpha);
			sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "overlay-vertex.txt" : "overlay-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

static const char *uniforms[] = {
	"vertex_transform", "tex_transform",
	"depth", "primary", "secondary",
	"tex",
	"lwmax", "gamma"
};

SehleProgram *
sehle_program_overlay_get_reference (SehleEngine *engine, unsigned int flags, float min_alpha)
{
	char c[256];
	sprintf (c, "Sehle::OverlayProgram_T%uC%uM%uD%uE%uA%.3f",
		(flags & SEHLE_PROGRAM_OVERLAY_HAS_TEXTURE) != 0,
		(flags & SEHLE_PROGRAM_OVERLAY_HAS_COLORS) != 0,
		(flags & SEHLE_PROGRAM_OVERLAY_HAS_MASK) != 0,
		(flags & SEHLE_PROGRAM_OVERLAY_HAS_DEPTH) != 0,
		(flags & SEHLE_PROGRAM_OVERLAY_HAS_EXPOSURE) != 0,
		min_alpha);
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 2, SEHLE_PROGRAM_OVERLAY_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, overlay_get_shader (engine, SEHLE_SHADER_VERTEX, flags, min_alpha));
		sehle_program_add_shader (prog, overlay_get_shader (engine, SEHLE_SHADER_FRAGMENT, flags, min_alpha));
		SehleShader *shader = sehle_shader_fetch_from_file (engine, "exposure.txt", SEHLE_SHADER_FRAGMENT);
		sehle_program_add_shader (prog, shader);
		sehle_program_set_uniform_names (prog, 0, SEHLE_PROGRAM_OVERLAY_NUM_UNIFORMS, (const unsigned char **) uniforms);
	}
	return prog;
}

