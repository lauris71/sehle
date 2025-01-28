#define __SEHLE_STARS_RENDERER_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2016-2021
 */

#include <stdio.h>

#include <az/class.h>

#include <GL/glew.h>

#include <sehle/engine.h>
#include <sehle/index-buffer.h>
#include <sehle/program.h>
#include <sehle/render-context.h>
#include <sehle/shader.h>
#include <sehle/vertex-buffer.h>

#include <sehle/stars-renderer.h>

enum SehleStarsUniforms {
	STARS_BRIGHTNESS = SEHLE_NUM_UNIFORMS, STARS_GAMMA,
	SEHLE_STARS_NUM_UNIFORMS
};

static const char *uniform_names[] = {
	"star_brightness", "star_gamma"
};

static SehleShader *
stars_get_shader (SehleEngine *engine, unsigned int shader_type)
{
	char c[256];
	sprintf (c, "Sehle::Stars::%s",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment");
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (!sehle_resource_is_initialized (&shader->resource)) {
		const char *sources[1];
		sprintf (c, "#version 140\n#define %s\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS");
		sources[0] = (shader_type == SEHLE_SHADER_VERTEX) ? "stars-vertex.txt" : "stars-fragment.txt";
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

SehleProgram *
sehle_program_stars_get_reference (SehleEngine *engine)
{
	char c[256];
	sprintf (c, "Sehle::Stars");
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 1, SEHLE_STARS_NUM_UNIFORMS);
	if (!sehle_resource_is_initialized (&prog->resource)) {
		sehle_program_add_shader (prog, stars_get_shader (engine, SEHLE_SHADER_VERTEX));
		sehle_program_add_shader (prog, stars_get_shader (engine, SEHLE_SHADER_FRAGMENT));
		sehle_program_set_uniform_names (prog, 0, SEHLE_NUM_UNIFORMS, ( const unsigned char **) sehle_uniforms);
		sehle_program_set_uniform_names (prog, SEHLE_NUM_UNIFORMS, SEHLE_STARS_NUM_UNIFORMS - SEHLE_NUM_UNIFORMS, (const unsigned char **) uniform_names);
	}
	return prog;
}

static void stars_class_init (SehleStarsRendererClass *klass);
static void stars_init (SehleStarsRendererClass *klass, SehleStarsRenderer *stars);
/* SehleRenderable implementation */
static void stars_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx);
static void stars_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data);
/* SehleMaterial implementation */
static void stars_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);

unsigned int sehle_stars_renderer_type = 0;
SehleStarsRendererClass *sehle_stars_renderer_class;

unsigned int
sehle_stars_renderer_get_type (void)
{
	if (!sehle_stars_renderer_type) {
		az_register_type (&sehle_stars_renderer_type, (const unsigned char *) "SehleStarsRenderer", AZ_TYPE_BLOCK, sizeof (SehleStarsRendererClass), sizeof (SehleStarsRenderer), AZ_CLASS_ZERO_MEMORY,
			(void (*) (AZClass *)) stars_class_init,
			(void (*) (const AZImplementation *, void *)) stars_init,
			NULL);
		sehle_stars_renderer_class = (SehleStarsRendererClass *) az_type_get_class (sehle_stars_renderer_type);
	}
	return sehle_stars_renderer_type;
}

static void
stars_class_init (SehleStarsRendererClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 2);
	az_class_declare_interface (( AZClass *) klass, 0, SEHLE_TYPE_RENDERABLE,
		ARIKKEI_OFFSET (SehleStarsRendererClass, renderable_impl),
		ARIKKEI_OFFSET (SehleStarsRenderer, renderable_inst));
	az_class_declare_interface ((AZClass *) klass, 1, SEHLE_TYPE_MATERIAL,
		ARIKKEI_OFFSET(SehleStarsRendererClass, material_impl),
		ARIKKEI_OFFSET(SehleStarsRenderer, material_inst));
	klass->renderable_impl.display = stars_display;
	klass->renderable_impl.render = stars_render;
	klass->material_impl.bind = stars_bind;
}

static void
stars_init (SehleStarsRendererClass *klass, SehleStarsRenderer *stars)
{
	stars->renderable_inst.render_stages = SEHLE_STAGE_TRANSPARENT;
	stars->renderable_inst.bbox = EleaAABox3fInfinite;
	sehle_render_flags_clear (&stars->material_inst.state_flags, SEHLE_CULL | SEHLE_DEPTH_WRITE);
	sehle_render_flags_set_depth_test (&stars->material_inst.state_flags, 1, SEHLE_DEPTH_LEQUAL);
	stars->material_inst.render_stages = SEHLE_STAGE_TRANSPARENT;
	stars->material_inst.render_types = SEHLE_RENDER_TRANSPARENT;
	stars->w2s = EleaMat3x4fIdentity;
	stars->starBrightness = 1;
	stars->starGamma = 1;
}

static void
stars_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx)
{
	SehleStarsRenderer *stars = SEHLE_STARS_RENDERER_FROM_RENDERABLE_INSTANCE (inst);
	sehle_render_context_schedule_render (ctx, impl, inst, SEHLE_STARS_RENDERER_MATERIAL_IMPLEMENTATION, &stars->material_inst, NULL);
}

static void
stars_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data)
{
	SehleStarsRenderer *stars = SEHLE_STARS_RENDERER_FROM_RENDERABLE_INSTANCE (inst);
	sehle_vertex_array_render_points (stars->va, 1, 0, stars->va->ibuf->buffer.n_elements);
}

static void
stars_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleStarsRenderer *stars = SEHLE_STARS_RENDERER_FROM_MATERIAL_INSTANCE (inst);
	// fixme: Move to proper place
	SEHLE_CHECK_ERRORS (0);
	glPointParameterf (GL_POINT_SIZE_MIN, 0.5f);
	SEHLE_CHECK_ERRORS (0);
	glPointParameterf (GL_POINT_SIZE_MAX, 5.0f);
	SEHLE_CHECK_ERRORS (0);
	glEnable (GL_VERTEX_PROGRAM_POINT_SIZE);
	SEHLE_CHECK_ERRORS (0);
	sehle_render_context_set_program (ctx, inst->programs[0]);
	EleaMat3x4f s2w, s2v;
	elea_mat3x4f_invert_normalized (&s2w, &stars->w2s);
	elea_mat3x4f_multiply (&s2v, &ctx->w2v, &s2w);
	elea_mat3x4f_set_translation_xyz (&s2v, 0, 0, 0);
	EleaMat4x4f s2v_proj;
	elea_mat4x4f_multiply_mat3x4 (&s2v_proj, &ctx->proj, &s2v);
	sehle_program_setUniformMatrix4fv (inst->programs[0], SEHLE_UNIFORM_O2V_PROJECTION, 1, s2v_proj.c);
	sehle_program_setUniform1f (inst->programs[0], STARS_BRIGHTNESS, stars->starBrightness);
	sehle_program_setUniform1f (inst->programs[0], STARS_GAMMA, stars->starGamma);
}

#define NCOORDS 4

void
sehle_stars_renderer_setup (SehleStarsRenderer *stars, SehleEngine *engine, unsigned int n_stars, const float *star_data)
{
	stars->va = sehle_vertex_array_new_from_attrs (engine, NULL, n_stars, n_stars, SEHLE_ATTRIBUTE_VERTEX, 4, -1);
	float *attributes = sehle_vertex_buffer_map (stars->va->vbufs[SEHLE_ATTRIBUTE_VERTEX], SEHLE_BUFFER_WRITE);
	uint32_t *indices = sehle_index_buffer_map (stars->va->ibuf, SEHLE_BUFFER_WRITE);
	for (unsigned int i = 0; i < n_stars; i++) {
		sehle_vertex_buffer_set_values (stars->va->vbufs[SEHLE_ATTRIBUTE_VERTEX], i, SEHLE_ATTRIBUTE_VERTEX, star_data + i * NCOORDS, 4);
		indices[i] = i;
	}
	sehle_vertex_buffer_unmap (stars->va->vbufs[SEHLE_ATTRIBUTE_VERTEX]);
	sehle_index_buffer_unmap (stars->va->ibuf);
	sehle_material_setup (&stars->material_inst, 1, 0);
	stars->material_inst.programs[0] = sehle_program_stars_get_reference (engine);
}

void
sehle_stars_renderer_release (SehleStarsRenderer *stars)
{
	az_object_unref (( AZObject *) stars->va);
	sehle_material_release (&stars->material_inst);
}

