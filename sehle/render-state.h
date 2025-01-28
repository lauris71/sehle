#ifndef __SEHLE_RENDER_STATE_H__
#define __SEHLE_RENDER_STATE_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2015
 *
 */

#include <nr/types.h>

#include "sehle.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Bitfield that describes the most common pipeline state
 *
 * Default values:
 * Culling enabled
 * Filled polygons
 * Depth write ON
 * Color write ALL
 * Depth test ON
 * Blend OFF
 * Depth function LESS
 * Blend equation ADD
 * Blend functions SOURCE_ALPHA ONE_MINUS_SOURCE_ALPHA
 */

#define SEHLE_CULL 1
#define SEHLE_FILL 2
#define SEHLE_DEPTH_WRITE 4
#define SEHLE_COLOR_WRITE 8
#define SEHLE_DEPTH_TEST 16
#define SEHLE_BLEND 32

/* Depth functions */
/* We force state zero to be default */
#define SEHLE_DEPTH_SHIFT 6
#define SEHLE_DEPTH_LESS 0
#define SEHLE_DEPTH_LEQUAL 1
#define SEHLE_DEPTH_EQUAL 2
#define SEHLE_DEPTH_GEQUAL 3
#define SEHLE_DEPTH_GREATER 4
#define SEHLE_DEPTH_NEVER 5
#define SEHLE_DEPTH_ALWAYS 6
#define SEHLE_DEPTH_MASK (7 << SEHLE_DEPTH_SHIFT)

/* Blend equations */
#define SEHLE_BLEND_EQ_SHIFT 9
#define SEHLE_BLEND_ADD 0
#define SEHLE_BLEND_SUBTRACT 1
#define SEHLE_BLEND_REVERSE_SUBTRACT 2
#define SEHLE_BLEND_MIN 3
#define SEHLE_BLEND_MAX 4
#define SEHLE_BLEND_EQ_MASK (7 << SEHLE_BLEND_EQ_SHIFT)

/* Blend functions */
#define SEHLE_BLEND_SRC_SHIFT 14
#define SEHLE_BLEND_DST_SHIFT 18
#define SEHLE_BLEND_ZERO 0
#define SEHLE_BLEND_ONE 1
#define SEHLE_BLEND_SRC_COLOR 2
#define SEHLE_BLEND_ONE_MINUS_SRC_COLOR 3
#define SEHLE_BLEND_DST_COLOR 4
#define SEHLE_BLEND_ONE_MINUS_DST_COLOR 5
#define SEHLE_BLEND_SRC_ALPHA 6
#define SEHLE_BLEND_ONE_MINUS_SRC_ALPHA 7
#define SEHLE_BLEND_DST_ALPHA 8
#define SEHLE_BLEND_ONE_MINUS_DST_ALPHA 9
#define SEHLE_BLEND_SRC_MASK (15 << SEHLE_BLEND_SRC_SHIFT)
#define SEHLE_BLEND_DST_MASK (15 << SEHLE_BLEND_DST_SHIFT)

#define SEHLE_BLEND_MASK (SEHLE_BLEND_EQ_MASK | SEHLE_BLEND_SRC_MASK | SEHLE_BLEND_DST_MASK)

#define SEHLE_RENDER_STATE_DEFAULT (SEHLE_CULL | SEHLE_FILL | SEHLE_DEPTH_WRITE | SEHLE_COLOR_WRITE | SEHLE_DEPTH_TEST | (SEHLE_DEPTH_LESS << SEHLE_DEPTH_SHIFT) | (SEHLE_BLEND_ADD << SEHLE_BLEND_EQ_SHIFT)| (SEHLE_BLEND_SRC_ALPHA << SEHLE_BLEND_SRC_SHIFT) | (SEHLE_BLEND_ONE_MINUS_SRC_ALPHA << SEHLE_BLEND_DST_SHIFT))

/* Convenience helpers, do not change engine state */
void sehle_render_flags_set (unsigned int *flags, unsigned int value);
void sehle_render_flags_clear (unsigned int *flags, unsigned int value);
void sehle_render_flags_set_depth_test (unsigned int *flags, unsigned int test, unsigned int function);
void sehle_render_flags_set_blend (unsigned int *flags, unsigned int blend, unsigned int equation, unsigned int src_function, unsigned int dst_function);

#define SEHLE_RENDER_STATE_NUM_TEXTURES 16

struct _SehleRenderState {
	unsigned int flags;
	SehleProgram *program;
	SehleRenderTarget *target;
	SehleTexture *textures[SEHLE_RENDER_STATE_NUM_TEXTURES];
	NRRectl viewport;
};

void sehle_render_state_init (SehleRenderState *rstate);
void sehle_render_state_finalize (SehleRenderState *rstate);

void sehle_render_state_set_default (SehleRenderState *rstate);
void sehle_render_state_set_program (SehleRenderState *rstate, SehleProgram *prog);
void sehle_render_state_set_target (SehleRenderState *rstate, SehleRenderTarget *tgt);
void sehle_render_state_set_texture (SehleRenderState *rstate, unsigned int idx, SehleTexture *tex);
void sehle_render_state_set_viewport (SehleRenderState *rstate, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1);

/* Update openGL state */

/* Set OpenGL state unconditionally */
void sehle_set_render_state (SehleRenderState *rstate);
/* Update current OpenGL state */
void sehle_update_render_state (SehleRenderState *current_state, const SehleRenderState *new_state);
void sehle_update_render_flags (SehleRenderState *current_state, unsigned int flags);
void sehle_update_render_target (SehleRenderState *current_state, SehleRenderTarget *tgt);
void sehle_update_program (SehleRenderState *current_state, SehleProgram *prog);
void sehle_update_texture (SehleRenderState *current_state, unsigned int idx, SehleTexture *tex);
void sehle_update_viewport (SehleRenderState *current_state, const NRRectl *vport);

#ifdef __cplusplus
};
#endif

#endif

