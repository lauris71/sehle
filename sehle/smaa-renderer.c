#define __SEHLE_SMAA_RENDERER_CPP__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2014
 *
 */

#include <stdlib.h>

#include <nr/pixblock.h>

#include "GL/glew.h"

#include <az/extend.h>

#include "engine.h"
#include "program.h"
#include "render-target-texture.h"
#include <sehle/render-context.h>
#include "shader.h"
#include "shaders-dict.h"
#include "smaa-dict.h"
#include "texture-2d.h"

#include "smaa-renderer.h"

// ArikkeiType implementation
static void smaa_renderer_class_init (SehleSMAARendererClass *klass);
// AZObject implementation
static void smaa_renderer_shutdown (AZObject *object);

unsigned int
sehle_smaa_renderer_get_type (void)
{
	static unsigned int type = 0;
	if (!type) {
		az_register_type (&type, (const unsigned char *) "SehleSMAARenderer", AZ_TYPE_OBJECT, sizeof (SehleSMAARendererClass), sizeof (SehleSMAARenderer), 0,
			(void (*) (AZClass *)) smaa_renderer_class_init,
			NULL, NULL);
	}
	return type;
}

static void
smaa_renderer_class_init (SehleSMAARendererClass *klass)
{
	((AZObjectClass *) klass)->shutdown = smaa_renderer_shutdown;
}

static void
smaa_renderer_shutdown (AZObject *object)
{
	SehleSMAARenderer *rend = SEHLE_SMAA_RENDERER(object);
	rend->engine = NULL;
	for (unsigned int i = 0; i < SEHLE_SMAA_NUM_TARGETS; i++) {
		if (rend->targets[i]) {
			az_object_unref (AZ_OBJECT(rend->targets[i]));
			rend->targets[i] = NULL;
		}
	}
	for (unsigned int i = 0; i < SEHLE_SMAA_NUM_TEXTURES; i++) {
		if (rend->textures[i]) {
			az_object_unref (AZ_OBJECT(rend->textures[i]));
			rend->textures[i] = NULL;
		}
	}
	for (unsigned int i = 0; i < SEHLE_SMAA_NUM_PROGRAMS; i++) {
		if (rend->programs[i]) {
			az_object_unref (AZ_OBJECT(rend->programs[i]));
			rend->programs[i] = NULL;
		}
	}
	sehle_render_context_release (&rend->ctx);
}

static const char *vs_header = ""
"#version 400\n"
"#define SMAA_RT_METRICS metrics\n"
"#define SMAA_GLSL_4\n"
"#define SMAA_PRESET_MEDIUM\n"
"#define SMAA_INCLUDE_VS 1\n"
"#define SMAA_INCLUDE_PS 0\n"
"\n"
"uniform vec4 metrics;\n";

static const char *fs_header = ""
"#version 400\n"
"#define SMAA_RT_METRICS metrics\n"
"#define SMAA_GLSL_4\n"
"#define SMAA_PRESET_MEDIUM\n"
"#define SMAA_INCLUDE_VS 0\n"
"#define SMAA_INCLUDE_PS 1\n"
"\n"
"uniform vec4 metrics;\n";

static const char *edges_vs_main = ""
"in vec2 position;\n"
"in vec2 texcoord;\n"
"\n"
"out vec2 i_texcoord;\n"
"out vec4 i_offset[3];\n"
"\n"
"void main ()\n"
"{\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"	 vec4 offset[3];\n"
"    SMAAEdgeDetectionVS (texcoord, offset);\n"
"	 i_offset[0] = offset[0];\n"
"	 i_offset[1] = offset[1];\n"
"	 i_offset[2] = offset[2];\n"
"    i_texcoord = texcoord;\n"
"}\n";

static const char *edges_fs_main = ""
"in vec2 i_texcoord;\n"
"in vec4 i_offset[3];\n"
"\n"
"uniform sampler2D colorTexGamma;\n"
"\n"
"out vec4 color_fragment;\n"
"void main ()\n"
"{\n"
"    vec2 edges = SMAAColorEdgeDetectionPS (i_texcoord, i_offset, colorTexGamma);\n"
"    color_fragment = vec4(edges, 0.0, 1.0);\n"
"}\n";

static const char *weights_vs_main = ""
"in vec2 position;\n"
"in vec2 texcoord;\n"
"\n"
"out vec2 i_texcoord;\n"
"out vec2 i_pixcoord;\n"
"out vec4 i_offset[3];\n"
"\n"
"void main ()\n"
"{\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"    vec2 pixcoord;\n"
"	 vec4 offset[3];\n"
"    SMAABlendingWeightCalculationVS (texcoord, pixcoord, offset);\n"
"    i_pixcoord = pixcoord;\n"
"	 i_offset[0] = offset[0];\n"
"	 i_offset[1] = offset[1];\n"
"	 i_offset[2] = offset[2];\n"
"    i_texcoord = texcoord;\n"
"}\n";

static const char *weights_fs_main = ""
"in vec2 i_texcoord;\n"
"in vec2 i_pixcoord;\n"
"in vec4 i_offset[3];\n"
"\n"
"uniform sampler2D edgesTex;\n"
"uniform sampler2D areaTex;\n"
"uniform sampler2D searchTex;\n"
"\n"
"out vec4 color_fragment;\n"
"void main ()\n"
"{\n"
"    vec4 subsampleIndices = vec4(0.0, 0.0, 0.0, 0.0);\n"
"    vec4 blends = SMAABlendingWeightCalculationPS (i_texcoord, i_pixcoord, i_offset, edgesTex, areaTex, searchTex, subsampleIndices);\n"
"    color_fragment = blends;\n"
"}\n";

static const char *blend_vs_main = ""
"in vec2 position;\n"
"in vec2 texcoord;\n"
"\n"
"out vec2 i_texcoord;\n"
"out vec4 i_offset;\n"
"\n"
"void main ()\n"
"{\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"    vec4 offset;\n"
"    SMAANeighborhoodBlendingVS(texcoord, offset);\n"
"    i_texcoord = texcoord;\n"
"    i_offset = offset;\n"
"}\n";

static const char *blend_fs_main = ""
"in vec2 i_texcoord;\n"
"in vec4 i_offset;\n"
"\n"
"uniform sampler2D colorTex;\n"
"uniform sampler2D blendTex;\n"
"\n"
"out vec4 color_fragment;\n"
"void main ()\n"
"{\n"
"    vec4 blends = SMAANeighborhoodBlendingPS (i_texcoord, i_offset, colorTex, blendTex);\n"
"    color_fragment = blends;\n"
"}\n";

SehleSMAARenderer *sehle_smaa_renderer_new (SehleEngine *engine, unsigned int width, unsigned int height,
	SehleRenderTargetTexture *edges_tgt, SehleRenderTargetTexture *blend_tgt, SehleTexture2D *edges_tex, SehleTexture2D *blend_tex)
{
	arikkei_return_val_if_fail (engine != NULL, NULL);
	arikkei_return_val_if_fail (!edges_tgt || SEHLE_IS_RENDER_TARGET_TEXTURE(edges_tgt), NULL);
	arikkei_return_val_if_fail (!blend_tgt || SEHLE_IS_RENDER_TARGET_TEXTURE(blend_tgt), NULL);
	SehleSMAARenderer *rend = (SehleSMAARenderer *) az_object_new (SEHLE_TYPE_SMAA_RENDERER);
	rend->engine = engine;
	rend->target_width = width;
	rend->target_height = height;
	sehle_render_context_setup (&rend->ctx, engine);
	/* Targets */
	SEHLE_CHECK_ERRORS (0);
	if (!edges_tgt) {
		SehleTexture2D *tex = edges_tex;
		if (!edges_tex) tex = sehle_texture_2d_new (engine, NULL);
		sehle_texture_2d_set_size (tex, width, height);
		sehle_texture_2d_set_format (tex, SEHLE_TEXTURE_RGB, SEHLE_TEXTURE_U8, SEHLE_TEXTURE_COLOR_SPACE_LINEAR);
		sehle_texture_2d_set_filter (tex, SEHLE_TEXTURE_FILTER_LINEAR, SEHLE_TEXTURE_FILTER_LINEAR);
		sehle_texture_2d_set_mapping (tex, 1);
		rend->targets[SEHLE_SMAA_EDGES_TARGET] = sehle_render_target_texture_new_tex (engine, tex, 0, 0);
		if (!edges_tex) az_object_unref (AZ_OBJECT (tex));
	} else {
		az_object_ref (AZ_OBJECT(edges_tgt));
		rend->targets[SEHLE_SMAA_EDGES_TARGET] = edges_tgt;
	}
	SEHLE_CHECK_ERRORS (0);
	if (!blend_tgt) {
		SehleTexture2D *tex = blend_tex;
		if (!blend_tex) tex = sehle_texture_2d_new (engine, NULL);
		sehle_texture_2d_set_size (tex, width, height);
		sehle_texture_2d_set_format (tex, SEHLE_TEXTURE_RGBA, SEHLE_TEXTURE_U8, SEHLE_TEXTURE_COLOR_SPACE_LINEAR);
		sehle_texture_2d_set_filter (tex, SEHLE_TEXTURE_FILTER_LINEAR, SEHLE_TEXTURE_FILTER_LINEAR);
		sehle_texture_2d_set_mapping (tex, 1);
		rend->targets[SEHLE_SMAA_WEIGHTS_TARGET] = sehle_render_target_texture_new_tex (engine, tex, 0, 0);
		if (!blend_tex) az_object_unref (AZ_OBJECT(tex));
	} else {
		az_object_ref (AZ_OBJECT(blend_tgt));
		rend->targets[SEHLE_SMAA_WEIGHTS_TARGET] = blend_tgt;
	}
	SEHLE_CHECK_ERRORS (0);
	/* Textures */
	rend->textures[SEHLE_SMAA_AREA_TEXTURE] = sehle_texture_2d_new (engine, (const unsigned char *) "SehleSMAAAreaTexture");
	if (!sehle_resource_is_initialized (&rend->textures[SEHLE_SMAA_AREA_TEXTURE]->texture.resource)) {
		size_t csize;
		/* fixme: Can be 2-channel texture */
		const unsigned char *cdata = smaa_mmap ("area_texture", &csize);
		NRPixBlock px;
		nr_pixblock_setup_extern_full (&px, NR_PIXBLOCK_U8, 3, NR_PIXBLOCK_LINEAR, 0, 0, 0, 160, 560, (unsigned char *) cdata, 160 * 3, 0);
		sehle_texture_2d_set_filter (rend->textures[SEHLE_SMAA_AREA_TEXTURE], SEHLE_TEXTURE_FILTER_LINEAR, SEHLE_TEXTURE_FILTER_LINEAR);
		sehle_texture_2d_set_mapping (rend->textures[SEHLE_SMAA_AREA_TEXTURE], 1);
		sehle_texture_2d_set_pixels_from_pixblock (rend->textures[SEHLE_SMAA_AREA_TEXTURE], &px);
		nr_pixblock_release (&px);
		smaa_unmap (cdata);
	}
	rend->textures[SEHLE_SMAA_SEARCH_TEXTURE] = sehle_texture_2d_new (engine, (const unsigned char *) "SehleSMAASearchTexture");
	if (!sehle_resource_is_initialized (&rend->textures[SEHLE_SMAA_SEARCH_TEXTURE]->texture.resource)) {
		sehle_texture_2d_set_filter (rend->textures[SEHLE_SMAA_SEARCH_TEXTURE], SEHLE_TEXTURE_FILTER_LINEAR, SEHLE_TEXTURE_FILTER_LINEAR);
		sehle_texture_2d_set_mapping (rend->textures[SEHLE_SMAA_SEARCH_TEXTURE], 1);
		size_t csize;
		/* fixme: Can be 1-channel texture */
		const unsigned char *cdata = smaa_mmap ("search_texture", &csize);
		NRPixBlock px;
		nr_pixblock_setup_extern_full (&px, NR_PIXBLOCK_U8, 3, NR_PIXBLOCK_LINEAR, 0, 0, 0, 64, 16, (unsigned char *) cdata, 64 * 3, 0);
		sehle_texture_2d_set_filter (rend->textures[SEHLE_SMAA_SEARCH_TEXTURE], SEHLE_TEXTURE_FILTER_NEAREST, SEHLE_TEXTURE_FILTER_NEAREST);
		sehle_texture_2d_set_pixels_from_pixblock (rend->textures[SEHLE_SMAA_SEARCH_TEXTURE], &px);
		nr_pixblock_release (&px);
		smaa_unmap (cdata);
	}
	rend->textures[SEHLE_SMAA_EDGES_TEXTURE] = sehle_render_target_texture_get_color (rend->targets[SEHLE_SMAA_EDGES_TARGET]);
	rend->textures[SEHLE_SMAA_WEIGHTS_TEXTURE] = sehle_render_target_texture_get_color (rend->targets[SEHLE_SMAA_WEIGHTS_TARGET]);
	/* Programs */
	unsigned int smaa_length;
	const char *smaa_code = (const char *) sehle_get_map ((const unsigned char *) "SMAA.txt", &smaa_length);
	const unsigned char *sources[] = { NULL, (const unsigned char *) smaa_code, NULL };
	int64_t slens[] = { -1, (int) smaa_length, -1 };
	rend->programs[SEHLE_SMAA_EDGES_PROGRAM] = sehle_engine_get_program (engine, "SehleSMAAEdgesProgram", 1, 1, 2);
	if (!sehle_resource_is_initialized (&rend->programs[SEHLE_SMAA_EDGES_PROGRAM]->resource)) {
		sources[0] = (const unsigned char *) vs_header;
		sources[2] = (const unsigned char *) edges_vs_main;
		SehleShader *vs = sehle_engine_get_shader (engine, "SehleSMAAEdgesVertex", SEHLE_SHADER_VERTEX);
		sehle_shader_build (vs, sources, 3, slens);
		SehleShader *fs = sehle_engine_get_shader (engine, "SehleSMAAEdgesFragment", SEHLE_SHADER_FRAGMENT);
		sources[0] = (const unsigned char *) fs_header;
		sources[2] = (const unsigned char *) edges_fs_main;
		sehle_shader_build (fs, sources, 3, slens);
		sehle_program_add_shader (rend->programs[SEHLE_SMAA_EDGES_PROGRAM], vs);
		sehle_program_add_shader (rend->programs[SEHLE_SMAA_EDGES_PROGRAM], fs);
		const char *uniform_names[] = { "metrics", "colorTexGamma" };
		sehle_program_set_uniform_names (rend->programs[SEHLE_SMAA_EDGES_PROGRAM], 0, 2, (const unsigned char **) uniform_names);
	}
	rend->programs[SEHLE_SMAA_WEIGHTS_PROGRAM] = sehle_engine_get_program (engine, "SehleSMAAWeightsProgram", 1, 1, 4);
	if (!sehle_resource_is_initialized (&rend->programs[SEHLE_SMAA_WEIGHTS_PROGRAM]->resource)) {
		sources[0] = (const unsigned char *) vs_header;
		sources[2] = (const unsigned char *) weights_vs_main;
		SehleShader *vs = sehle_engine_get_shader (engine, "SehleSMAAWeightsVertex", SEHLE_SHADER_VERTEX);
		sehle_shader_build (vs, sources, 3, slens);
		SehleShader *fs = sehle_engine_get_shader (engine, "SehleSMAAWeightsFragment", SEHLE_SHADER_FRAGMENT);
		sources[0] = (const unsigned char *) fs_header;
		sources[2] = (const unsigned char *) weights_fs_main;
		sehle_shader_build (fs, sources, 3, slens);
		sehle_program_add_shader (rend->programs[SEHLE_SMAA_WEIGHTS_PROGRAM], vs);
		sehle_program_add_shader (rend->programs[SEHLE_SMAA_WEIGHTS_PROGRAM], fs);
		const char *uniform_names[] = { "metrics", "edgesTex", "areaTex", "searchTex" };
		sehle_program_set_uniform_names (rend->programs[SEHLE_SMAA_WEIGHTS_PROGRAM], 0, 4, (const unsigned char **) uniform_names);
	}
	rend->programs[SEHLE_SMAA_BLEND_PROGRAM] = sehle_engine_get_program (engine, "SehleSMAABlendProgram", 1, 1, 3);
	if (!sehle_resource_is_initialized (&rend->programs[SEHLE_SMAA_BLEND_PROGRAM]->resource)) {
		sources[0] = (const unsigned char *) vs_header;
		sources[2] = (const unsigned char *) blend_vs_main;
		SehleShader *vs = sehle_engine_get_shader (engine, "SehleSMAABlendVertex", SEHLE_SHADER_VERTEX);
		sehle_shader_build (vs, sources, 3, slens);
		SehleShader *fs = sehle_engine_get_shader (engine, "SehleSMAABlendFragment", SEHLE_SHADER_FRAGMENT);
		sources[0] = (const unsigned char *) fs_header;
		sources[2] = (const unsigned char *) blend_fs_main;
		sehle_shader_build (fs, sources, 3, slens);
		sehle_program_add_shader (rend->programs[SEHLE_SMAA_BLEND_PROGRAM], vs);
		sehle_program_add_shader (rend->programs[SEHLE_SMAA_BLEND_PROGRAM], fs);
		const char *uniform_names[] = { "metrics", "colorTex", "blendTex" };
		sehle_program_set_uniform_names (rend->programs[SEHLE_SMAA_BLEND_PROGRAM], 0, 3, (const unsigned char **) uniform_names);
	}

	sehle_free_map ((const unsigned char *) smaa_code);
	return rend;
}

void
sehle_smaa_renderer_resize (SehleSMAARenderer *rend, unsigned int width, unsigned int height)
{
	arikkei_return_if_fail (rend != NULL);
	arikkei_return_if_fail (SEHLE_IS_SMAA_RENDERER (rend));
	for (unsigned int i = 0; i < SEHLE_SMAA_NUM_TARGETS; i++) {
		if (rend->targets[i]) {
			sehle_render_target_resize (SEHLE_RENDER_TARGET(rend->targets[i]), width, height);
		}
	}
	rend->target_width = width;
	rend->target_height = height;
}

void
sehle_smaa_renderer_render (SehleSMAARenderer *rend, SehleRenderTarget *tgt, SehleTexture2D *source)
{
	arikkei_return_if_fail (rend != NULL);
	arikkei_return_if_fail (SEHLE_IS_SMAA_RENDERER (rend));
	arikkei_return_if_fail (tgt != NULL);
	arikkei_return_if_fail (SEHLE_IS_RENDER_TARGET (tgt));
	arikkei_return_if_fail (source != NULL);

	float m[] = { 1.0f / rend->target_width, 1.0f / rend->target_height, (float) rend->target_width, (float) rend->target_height };

	/* Edges */
	SEHLE_CHECK_ERRORS (0);
	sehle_render_context_reset (&rend->ctx);
	sehle_render_context_set_target (&rend->ctx, &rend->targets[SEHLE_SMAA_EDGES_TARGET]->render_target);
	sehle_render_context_set_viewport (&rend->ctx, 0, 0, rend->target_width, rend->target_height);
	sehle_render_context_clear (&rend->ctx, 0, 1, &EleaColor4fTransparent);
	sehle_render_context_set_program (&rend->ctx, rend->programs[SEHLE_SMAA_EDGES_PROGRAM]);
	rend->ctx.render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
	sehle_render_flags_clear (&rend->ctx.render_state.flags, SEHLE_DEPTH_TEST | SEHLE_DEPTH_WRITE);
	SEHLE_CHECK_ERRORS (0);
	sehle_render_context_set_texture_to_channel (&rend->ctx, &source->texture, 0);
	sehle_render_context_bind (&rend->ctx);
	SEHLE_CHECK_ERRORS (0);

	sehle_program_setUniform4fv (rend->programs[SEHLE_SMAA_EDGES_PROGRAM], 0, 1, m);
	sehle_program_setUniform1i (rend->programs[SEHLE_SMAA_EDGES_PROGRAM], 1, 0);
	SEHLE_CHECK_ERRORS (0);

	glStencilMask (255);
	glClearStencil (0);
	glEnable (GL_STENCIL_TEST);
	glClear (GL_STENCIL_BUFFER_BIT);
	glStencilFunc (GL_ALWAYS, 1, 1);
	glStencilOp (GL_KEEP, GL_KEEP, GL_REPLACE);

	sehle_render_context_draw_overlay_rect_2d (&rend->ctx, 0, 0, rend->target_width, rend->target_height, 0, 0, 1, 1);

	/* Blending weights */
	SEHLE_CHECK_ERRORS (0);
	sehle_render_context_set_target (&rend->ctx, SEHLE_RENDER_TARGET (rend->targets[SEHLE_SMAA_WEIGHTS_TARGET]));
	sehle_render_context_clear (&rend->ctx, 0, 1, &EleaColor4fTransparent);
	sehle_render_context_set_program (&rend->ctx, rend->programs[SEHLE_SMAA_WEIGHTS_PROGRAM]);
	sehle_render_context_set_texture_to_channel (&rend->ctx, &rend->textures[SEHLE_SMAA_EDGES_TEXTURE]->texture, 0);
	sehle_render_context_set_texture_to_channel (&rend->ctx, &rend->textures[SEHLE_SMAA_AREA_TEXTURE]->texture, 1);
	sehle_render_context_set_texture_to_channel (&rend->ctx, &rend->textures[SEHLE_SMAA_SEARCH_TEXTURE]->texture, 2);
	sehle_render_context_bind (&rend->ctx);

	sehle_program_setUniform4fv (rend->programs[SEHLE_SMAA_WEIGHTS_PROGRAM], 0, 1, m);
	sehle_program_setUniform1i (rend->programs[SEHLE_SMAA_WEIGHTS_PROGRAM], 1, 0);
	sehle_program_setUniform1i (rend->programs[SEHLE_SMAA_WEIGHTS_PROGRAM], 2, 1);
	sehle_program_setUniform1i (rend->programs[SEHLE_SMAA_WEIGHTS_PROGRAM], 3, 2);

	glStencilFunc (GL_EQUAL, 1, 1);
	glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);

	sehle_render_context_draw_overlay_rect_2d (&rend->ctx, 0, 0, rend->target_width, rend->target_height, 0, 0, 1, 1);

	glStencilMask (0);
	glDisable (GL_STENCIL_TEST);

	/* Blending */
	sehle_render_context_set_target (&rend->ctx, tgt);
	sehle_render_context_set_program (&rend->ctx, rend->programs[SEHLE_SMAA_BLEND_PROGRAM]);
	sehle_render_context_set_texture_to_channel (&rend->ctx, &source->texture, 0);
	sehle_render_context_set_texture_to_channel (&rend->ctx, &rend->textures[SEHLE_SMAA_WEIGHTS_TEXTURE]->texture, 1);
	sehle_render_context_bind (&rend->ctx);

	sehle_program_setUniform4fv (rend->programs[SEHLE_SMAA_BLEND_PROGRAM], 0, 1, m);
	sehle_program_setUniform1i (rend->programs[SEHLE_SMAA_BLEND_PROGRAM], 1, 0);
	sehle_program_setUniform1i (rend->programs[SEHLE_SMAA_BLEND_PROGRAM], 2, 1);

	sehle_render_context_draw_overlay_rect_2d (&rend->ctx, 0, 0, rend->target_width, rend->target_height, 0, 0, 1, 1);

	sehle_render_context_end (&rend->ctx);
}

