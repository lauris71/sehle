#define __SEHLE_LIGHT_VOLUME_CPP__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2012-2016
//

#include <stdio.h>

#include <az/extend.h>

#include <GL/glew.h>

#include <sehle/engine.h>
#include <sehle/index-buffer.h>
#include <sehle/render-context.h>
#include <sehle/program.h>
#include <sehle/shader.h>
#include <sehle/vertex-buffer.h>

#include <sehle/light-volume.h>

enum SehleLightVolumeUniforms {
	L2W_W2V_PROJECTION,
	// Transformation from window to map coordinates
	W2G,
	// Transformation from map to eye coordinates
	G2E_RPROJECTION,
	// GBuffer samplers
	DEPTH_SAMPLER, NORMAL_SAMPLER, ALBEDO_SAMPLER, SPECULAR_SAMPLER,
	// Ambient color
	LV_PLANES, AMBIENT,
	SEHLE_LIGHT_VOLUME_NUM_UNIFORMS
};

static const char *uniform_names[] = {
	"l2w_w2v_projection", "w2g", "g2e_rprojection",
	"depthSampler", "normalSampler", "albedoSampler", "specularSampler",
	"planes", "ambient"
};

static SehleShader *
light_volume_get_shader (SehleEngine *engine, unsigned int shader_type, unsigned int program)
{
	char c[256];
	sprintf (c, "Sehle::LightVolume::%s",
		(shader_type == SEHLE_SHADER_VERTEX) ? "Vertex" : "Fragment");
	SehleShader *shader = sehle_engine_get_shader (engine, c, shader_type);
	if (sehle_resource_get_sate (&shader->resource) == SEHLE_RESOURCE_STATE_CREATED) {
		const char *sources[1];
		sprintf (c, "#version 140\n#define %s\n",
			(shader_type == SEHLE_SHADER_VERTEX) ? "VS" : "FS");
		if (shader_type == SEHLE_SHADER_VERTEX) {
			sources[0] = (program == SEHLE_LIGHT_VOLUME_PROGRAM_STENCIL) ? "light-vertex.txt" : "gbuf-ssao-vertex.txt";
		} else {
			sources[0] = "lightvolume-fragment.txt";
		}
		sehle_shader_build_from_header_files (shader, (const unsigned char *) c, -1, (const unsigned char **) sources, 1);
	}
	return shader;
}

SehleProgram *
sehle_program_light_volume_get_reference (SehleEngine *engine, unsigned int program)
{
	char c[256];
	sprintf (c, "Sehle::LightVolume");
	SehleProgram *prog = sehle_engine_get_program (engine, c, 1, 2, SEHLE_LIGHT_VOLUME_NUM_UNIFORMS);
	if (sehle_resource_get_sate (&prog->resource) == SEHLE_RESOURCE_STATE_CREATED) {
		sehle_program_add_shader (prog, light_volume_get_shader (engine, SEHLE_SHADER_VERTEX, program));
		sehle_program_add_shader (prog, light_volume_get_shader (engine, SEHLE_SHADER_FRAGMENT, program));
		SehleShader *shader = sehle_engine_get_shader (engine, "decodeGBuffer.txt", SEHLE_SHADER_FRAGMENT);
		if (sehle_resource_get_sate (&shader->resource) == SEHLE_RESOURCE_STATE_CREATED) {
			sehle_shader_build_from_file (shader, (const unsigned char *) "decodeGBuffer.txt");
		}
		sehle_program_add_shader (prog, shader);
		sehle_program_set_uniform_names (prog, 0, SEHLE_LIGHT_VOLUME_NUM_UNIFORMS, (const unsigned char **) uniform_names);
	}
	return prog;
}

static void light_volume_class_init (SehleLightVolumeClass *klass);
static void light_volume_init (SehleLightVolumeClass *klass, SehleLightVolume *lvol);
/* SehleMaterial implementation */
static void light_volume_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);
//static void light_volume_render (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, SehleRenderData *rd, unsigned int render_type);

unsigned int sehle_light_volume_type = 0;
SehleLightVolumeClass *sehle_light_volume_class;

unsigned int
sehle_light_volume_get_type (void)
{
	if (!sehle_light_volume_type) {
		az_register_type (&sehle_light_volume_type, (const unsigned char *) "SehleLightVolume", AZ_TYPE_STRUCT, sizeof (SehleLightVolumeClass), sizeof (SehleLightVolume), AZ_FLAG_ZERO_MEMORY,
			(void (*) (AZClass *)) light_volume_class_init,
			(void (*) (const AZImplementation *, void *)) light_volume_init,
			NULL);
		sehle_light_volume_class = (SehleLightVolumeClass *) az_type_get_class (sehle_light_volume_type);
	}
	return sehle_light_volume_type;
}

static void
light_volume_class_init (SehleLightVolumeClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_LIGHT,
		ARIKKEI_OFFSET(SehleLightVolumeClass, light_implementation),
		ARIKKEI_OFFSET(SehleLightVolume, light_instance));
	klass->light_implementation.material_impl.bind = light_volume_bind;
	//klass->light_implementation.material_impl.render = light_volume_render;
}

static void
light_volume_init (SehleLightVolumeClass *klass, SehleLightVolume *lvol)
{
	lvol->light_instance.material_inst.properties |= SEHLE_MATERIAL_STENCIL;
	lvol->light_instance.material_inst.render_stages = SEHLE_STAGE_LIGHTS;
	lvol->light_instance.material_inst.render_types = SEHLE_RENDER_STENCIL | SEHLE_RENDER_LIGHTMAP;
	lvol->factor = 0.5f;
	for (unsigned int i = 0; i < 8; i++) {
		elea_plane3fp_set_nd (&lvol->planes[i], &EleaVec3fZ, -1000000);
	}
}

static void
light_volume_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type)
{
	SehleLightVolume *lvol = SEHLE_LIGHT_VOLUME_FROM_MATERIAL_INSTANCE (inst);
	if (render_type == SEHLE_RENDER_STENCIL) {
		SehleProgram *prog = inst->programs[SEHLE_LIGHT_VOLUME_PROGRAM_STENCIL];
		sehle_render_context_set_gbuffer_uniforms (ctx, prog, W2G, G2E_RPROJECTION);
	} else {
		SehleProgram *prog = inst->programs[SEHLE_LIGHT_VOLUME_PROGRAM_LIGHT];
		sehle_render_context_set_gbuffer_uniforms (ctx, prog, W2G, G2E_RPROJECTION);
		if (ctx->depthchannel >= 0) sehle_program_setUniform1i (prog, DEPTH_SAMPLER, ctx->depthchannel);
		if (ctx->normalchannel >= 0) sehle_program_setUniform1i (prog, NORMAL_SAMPLER, ctx->normalchannel);
		if (ctx->albedochannel >= 0) sehle_program_setUniform1i (prog, ALBEDO_SAMPLER, ctx->albedochannel);
		if (ctx->specularchannel >= 0) sehle_program_setUniform1i (prog, SPECULAR_SAMPLER, ctx->specularchannel);
		EleaPlane3f p[8];
		for (int i = 0; i < 8; i++) {
			elea_mat3x4f_transform_plane (&p[i], &ctx->w2v, &lvol->planes[i]);
			//p[i] = ctx->w2v.transform (lvol->planes[i]);
		}
		sehle_program_setUniform4fv (prog, LV_PLANES, 8, p[0].c);
		EleaColor4f ambient = elea_color4f_mul (ctx->global_ambient, lvol->factor);
		sehle_program_setUniform3fv (prog, AMBIENT, 1, ambient.c);
	}
}

#if 0
static void
light_volume_render (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, SehleRenderData *rd, unsigned int render_type)
{
	if (!rd->n_indices) return;
	if (render_type == SEHLE_RENDER_STENCIL) {
		/* Stencil pass marks lightmap area */
		SehleProgram* prog = inst->programs[SEHLE_LIGHT_VOLUME_PROGRAM_STENCIL];
		const EleaMat3x4f* i2w = (const EleaMat3x4f*) rd->instances;
		EleaMat3x4f o2v;
		EleaMat4x4f o2v_proj;
		elea_mat3x4f_multiply (&o2v, &ctx->w2v, &i2w[0]);
		elea_mat4x4f_multiply_mat3x4 (&o2v_proj, &ctx->proj, &o2v);
		sehle_program_setUniformMatrix4fv (prog, L2W_W2V_PROJECTION, rd->n_instances, o2v_proj.c);
		sehle_vertex_array_bind (rd->va);
		SEHLE_CHECK_ERRORS (0);
		glDrawElementsInstanced (GL_TRIANGLES, rd->n_indices, GL_UNSIGNED_INT, (uint32_t *) NULL + rd->first, rd->n_instances);
		SEHLE_CHECK_ERRORS (0);
	} else if (render_type == SEHLE_RENDER_LIGHTMAP) {
		/* Draw actual light */
		sehle_vertex_array_render_triangles (rd->va, rd->_first, rd->_n_indices);
	}
}
#endif

void
sehle_light_volume_setup (SehleLightVolume *lvol, SehleEngine *engine)
{
	sehle_material_setup (&lvol->light_instance.material_inst, 2, 0);
	lvol->light_instance.material_inst.programs[SEHLE_LIGHT_VOLUME_PROGRAM_STENCIL] = sehle_program_light_volume_get_reference (engine, SEHLE_LIGHT_VOLUME_PROGRAM_STENCIL);
	lvol->light_instance.material_inst.programs[SEHLE_LIGHT_VOLUME_PROGRAM_LIGHT] = sehle_program_light_volume_get_reference (engine, SEHLE_LIGHT_VOLUME_PROGRAM_LIGHT);
}

void
sehle_light_volume_release (SehleLightVolume *lvol)
{
	sehle_material_release (&lvol->light_instance.material_inst);
}

