#ifndef __SEHLE_PROGRAM_OCCLUSION_H__
#define __SEHLE_PROGRAM_OCCLUSION_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

#include <elea/matrix3x4.h>

#include <sehle/program.h>

/* Occlusion program uniforms */

enum SehleOcclusionUniforms {
	SEHLE_OCCLUSION_O2V,
	SEHLE_OCCLUSION_PROJECTION,
	// From GBuffer to (denormalized) view
	SEHLE_OCCLUSION_G2D_RPROJECTION,
	// From window to gbuffer packed
	SEHLE_OCCLUSION_W2G,
	SEHLE_OCCLUSION_DEPTH_SAMPLER,
	//
	SEHLE_OCCLUSION_V2O,
	SEHLE_OCCLUSION_RADIUS, SEHLE_OCCLUSION_INNER_RADIUS, SEHLE_OCCLUSION_STRENGTH,
	SEHLE_OCCLUSION_NUM_UNIFORMS
};

enum SehleOcclusionAttributes {
	SEHLE_OCCLUSION_VERTEX,
	SEHLE_OCCLUSION_NUM_ATTRIBUTES
};

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_occlusion_get_reference (SehleEngine *engine, unsigned int flags, unsigned int max_instances);

void sehle_program_occlusion_bind_instance (SehleProgram *prog, SehleRenderContext *ctx, float radius, float inner_radius, float strength);
void sehle_program_occlusion_render (SehleProgram *prog, SehleRenderContext *ctx, unsigned int n_instances, const EleaMat3x4f *i2w, SehleVertexArray *va);

#ifdef __cplusplus
};
#endif

#endif

