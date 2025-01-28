#ifndef __SEHLE_COMMONMATERIALS_H__
#define __SEHLE_COMMONMATERIALS_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

#include <elea/matrix4x4.h>

#include <sehle/material.h>

#define SEHLE_PROGRAM_DEPTH_HAS_COLORS 1
#define SEHLE_PROGRAM_DEPTH_HAS_TEXTURE 2
#define SEHLE_PROGRAM_DEPTH_HAS_BONES 4

enum {
	SEHLE_PROGRAM_DEPTH_DIFFUSE = SEHLE_NUM_UNIFORMS,
	SEHLE_PROGRAM_DEPTH_OPACITY,
	SEHLE_PROGRAM_DEPTH_COLOR_SAMPLER,
	SEHLE_PROGRAM_DEPTH_NUM_UNIFORMS
};

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_depth_program_get_reference (SehleEngine *engine, unsigned int flags, unsigned int max_instances);

void sehle_program_depth_bind_instance (SehleProgram *prog, SehleRenderContext *ctx, unsigned int render_type);
void sehle_program_depth_render (SehleProgram *prog, SehleVertexArray *va, unsigned int first_index, unsigned int n_indices, const EleaMat3x4f *o2v, const EleaMat4x4f *proj);

#ifdef __cplusplus
};
#endif

#endif
