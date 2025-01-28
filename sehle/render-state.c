#define __SEHLE_RENDER_STATE_C__

/*
* Libsehle
*
* Copyright (C) Lauris Kaplinski 2015-2018
*
*/

#include <string.h>

#include "GL/glew.h"

#include <az/object.h>

#include <sehle/engine.h>
#include <sehle/program.h>
#include <sehle/render-target.h>
#include <sehle/texture.h>

#include <sehle/render-state.h>

void
sehle_render_flags_set (unsigned int *flags, unsigned int value)
{
	*flags |= value;
}

void
sehle_render_flags_clear (unsigned int *flags, unsigned int value)
{
	*flags &= ~value;
}

void
sehle_render_flags_set_depth_test (unsigned int *flags, unsigned int test, unsigned int function)
{
	if (test) {
		*flags |= SEHLE_DEPTH_TEST;
	} else {
		*flags &= ~SEHLE_DEPTH_TEST;
	}
	*flags = (*flags & ~SEHLE_DEPTH_MASK) | (function << SEHLE_DEPTH_SHIFT);
}

void
sehle_render_flags_set_blend (unsigned int *flags, unsigned int blend, unsigned int equation, unsigned int src_function, unsigned int dst_function)
{
	if (blend) {
		*flags |= SEHLE_BLEND;
	} else {
		*flags &= ~SEHLE_BLEND;
	}
	*flags = (*flags & ~SEHLE_BLEND_MASK) | (equation << SEHLE_BLEND_EQ_SHIFT) | (src_function << SEHLE_BLEND_SRC_SHIFT) | (dst_function << SEHLE_BLEND_DST_SHIFT);
}

static const NRRectl default_vp = { 0, 0, 1, 1 };
void
sehle_render_state_init (SehleRenderState *rstate)
{
	memset (rstate, 0, sizeof (SehleRenderState));
	rstate->flags = SEHLE_RENDER_STATE_DEFAULT;
	rstate->viewport = default_vp;
}

void
sehle_render_state_finalize (SehleRenderState *rstate)
{
	unsigned int i;
	if (rstate->program) az_object_unref (AZ_OBJECT (rstate->program));
	if (rstate->target) az_object_unref (AZ_OBJECT (rstate->target));
	for (i = 0; i < SEHLE_RENDER_STATE_NUM_TEXTURES; i++) {
		if (rstate->textures[i]) az_object_unref (AZ_OBJECT (rstate->textures[i]));
	}
}

void
sehle_render_state_set_default (SehleRenderState *rstate)
{
	unsigned int i;
	rstate->flags = SEHLE_RENDER_STATE_DEFAULT;
	if (rstate->program) az_object_unref (AZ_OBJECT (rstate->program));
	rstate->program = NULL;
	if (rstate->target) az_object_unref (AZ_OBJECT (rstate->target));
	rstate->target = NULL;
	for (i = 0; i < SEHLE_RENDER_STATE_NUM_TEXTURES; i++) {
		if (rstate->textures[i]) az_object_unref (AZ_OBJECT (rstate->textures[i]));
		rstate->textures[i] = NULL;
	}
	rstate->viewport = default_vp;
}

void
sehle_render_state_set_program (SehleRenderState *rstate, SehleProgram *prog)
{
	if (rstate->program == prog) return;
	if (rstate->program) az_object_unref (AZ_OBJECT (rstate->program));
	rstate->program = prog;
	if (rstate->program) az_object_ref (AZ_OBJECT (rstate->program));
}

void
sehle_render_state_set_target (SehleRenderState *rstate, SehleRenderTarget *tgt)
{
	if (rstate->target == tgt) return;
	if (rstate->target) az_object_unref ((AZObject *) rstate->target);
	rstate->target = tgt;
	if (rstate->target) az_object_ref ((AZObject *) rstate->target);
}

void
sehle_render_state_set_texture (SehleRenderState *rstate, unsigned int idx, SehleTexture *tex)
{
	if (rstate->textures[idx]) az_object_unref (AZ_OBJECT (rstate->textures[idx]));
	rstate->textures[idx] = tex;
	if (rstate->textures[idx]) az_object_ref (AZ_OBJECT (rstate->textures[idx]));
}

void
sehle_render_state_set_viewport (SehleRenderState *rstate, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
	rstate->viewport.x0 = x0;
	rstate->viewport.y0 = y0;
	rstate->viewport.x1 = x1;
	rstate->viewport.y1 = y1;
}

void
sehle_set_render_state (SehleRenderState *rstate)
{
	unsigned int i;
	if (rstate->flags & SEHLE_CULL) {
		glEnable (GL_CULL_FACE);
	} else {
		glDisable (GL_CULL_FACE);
	}
	if (rstate->flags & SEHLE_FILL) {
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	} else {
		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	}
	if (rstate->flags & SEHLE_DEPTH_WRITE) {
		glDepthMask (1);
	} else {
		glDepthMask (0);
	}
	if (rstate->flags & SEHLE_COLOR_WRITE) {
		glColorMask (1, 1, 1, 1);
	} else {
		glColorMask (0, 0, 0, 0);
	}
	if (rstate->flags & SEHLE_DEPTH_TEST) {
		static unsigned int functions[] = { GL_LESS, GL_LEQUAL, GL_EQUAL, GL_GEQUAL, GL_GREATER, GL_NEVER, GL_ALWAYS };
		glEnable (GL_DEPTH_TEST);
		glDepthFunc (functions[(rstate->flags & SEHLE_DEPTH_MASK) >> SEHLE_DEPTH_SHIFT]);
	} else {
		glDisable (GL_DEPTH_TEST);
	}
	if (rstate->flags & SEHLE_BLEND) {
		static unsigned int equations[] = { SEHLE_BLEND_ADD, SEHLE_BLEND_SUBTRACT, SEHLE_BLEND_REVERSE_SUBTRACT, SEHLE_BLEND_MIN, SEHLE_BLEND_MAX };
		static unsigned int functions[] = { GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };
		glEnable (GL_BLEND);
		glBlendEquation (equations[(rstate->flags & SEHLE_BLEND_EQ_MASK) >> SEHLE_BLEND_EQ_SHIFT]);
		glBlendFunc (functions[(rstate->flags & SEHLE_BLEND_SRC_MASK) >> SEHLE_BLEND_SRC_SHIFT], functions[(rstate->flags & SEHLE_BLEND_DST_MASK) >> SEHLE_BLEND_DST_SHIFT]);
	} else {
		glDisable (GL_BLEND);
	}
	if (rstate->program) {
		glUseProgram (sehle_resource_get_handle (&rstate->program->resource));
#ifdef SEHLE_PERFORMANCE_MONITOR
		rstate->program->resource.engine->counter.program_switches += 1;
#endif
	} else {
		glUseProgram (0);
	}
	if (rstate->target) {
		glBindFramebuffer (GL_FRAMEBUFFER, sehle_resource_get_handle (&rstate->target->resource));
#ifdef SEHLE_PERFORMANCE_MONITOR
		rstate->target->resource.engine->counter.rendertarget_binds += 1;
#endif
	} else {
		glBindFramebuffer (GL_FRAMEBUFFER, 0);
	}
	for (i = 0; i < SEHLE_RENDER_STATE_NUM_TEXTURES; i++) {
		if (rstate->textures[i]) {
			sehle_texture_bind (rstate->textures[i], i);
		}
	}
	glViewport (rstate->viewport.x0, rstate->viewport.y0, rstate->viewport.x1 - rstate->viewport.x0, rstate->viewport.y1 - rstate->viewport.y0);
}

void
sehle_update_render_state (SehleRenderState *current_state, const SehleRenderState *new_state)
{
	unsigned int i;
	SEHLE_CHECK_ERRORS (0);
	if (current_state->flags != new_state->flags) {
		/* Cull faces */
		if ((current_state->flags ^ new_state->flags) & SEHLE_CULL) {
			if (new_state->flags & SEHLE_CULL) {
				glEnable (GL_CULL_FACE);
			} else {
				glDisable (GL_CULL_FACE);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		// Fill triangles
		if ((current_state->flags ^ new_state->flags) & SEHLE_FILL) {
			if (new_state->flags & SEHLE_FILL) {
				glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
			} else {
				glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		// Write depth
		if ((current_state->flags ^ new_state->flags) & SEHLE_DEPTH_WRITE) {
			if (new_state->flags & SEHLE_DEPTH_WRITE) {
				glDepthMask (1);
			} else {
				glDepthMask (0);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		// Write color
		if ((current_state->flags ^ new_state->flags) & SEHLE_COLOR_WRITE) {
			if (new_state->flags & SEHLE_COLOR_WRITE) {
				glColorMask (1, 1, 1, 1);
			} else {
				glColorMask (0, 0, 0, 0);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		// Test depth
		if ((current_state->flags ^ new_state->flags) & (SEHLE_DEPTH_TEST | SEHLE_DEPTH_MASK)) {
			if (new_state->flags & SEHLE_DEPTH_TEST) {
				glEnable (GL_DEPTH_TEST);
				static unsigned int functions[] = { GL_LESS, GL_LEQUAL, GL_EQUAL, GL_GEQUAL, GL_GREATER, GL_NEVER, GL_ALWAYS };
				glEnable (GL_DEPTH_TEST);
				glDepthFunc (functions[(new_state->flags & SEHLE_DEPTH_MASK) >> SEHLE_DEPTH_SHIFT]);
			} else {
				glDisable (GL_DEPTH_TEST);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		// Blend
		if ((current_state->flags ^ new_state->flags) & (SEHLE_BLEND | SEHLE_BLEND_MASK)) {
			if (new_state->flags & SEHLE_BLEND) {
				static unsigned int equations[] = { GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX };
				static unsigned int functions[] = { GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };
				glEnable (GL_BLEND);
				unsigned int idx = (new_state->flags & SEHLE_BLEND_EQ_MASK) >> SEHLE_BLEND_EQ_SHIFT;
				glBlendEquation (equations[idx]);
				SEHLE_CHECK_ERRORS (0);
				unsigned int sidx = (new_state->flags & SEHLE_BLEND_SRC_MASK) >> SEHLE_BLEND_SRC_SHIFT;
				unsigned int didx = (new_state->flags & SEHLE_BLEND_DST_MASK) >> SEHLE_BLEND_DST_SHIFT;
				glBlendFunc (functions[sidx], functions[didx]);
				SEHLE_CHECK_ERRORS (0);
			} else {
				glDisable (GL_BLEND);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		current_state->flags = new_state->flags;
	}
	sehle_update_program (current_state, new_state->program);
	SEHLE_CHECK_ERRORS (0);
	sehle_update_render_target (current_state, new_state->target);
	SEHLE_CHECK_ERRORS (0);
	for (i = 0; i < SEHLE_RENDER_STATE_NUM_TEXTURES; i++) {
		sehle_update_texture (current_state, i, new_state->textures[i]);
		SEHLE_CHECK_ERRORS (0);
	}
	sehle_update_viewport (current_state, &new_state->viewport);
	SEHLE_CHECK_ERRORS (0);
}

void
sehle_update_render_flags (SehleRenderState *current_state, unsigned int flags)
{
	if (current_state->flags != flags) {
		/* Cull faces */
		if ((current_state->flags ^ flags) & SEHLE_CULL) {
			if (flags & SEHLE_CULL) {
				glEnable (GL_CULL_FACE);
			} else {
				glDisable (GL_CULL_FACE);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		// Fill triangles
		if ((current_state->flags ^ flags) & SEHLE_FILL) {
			if (flags & SEHLE_FILL) {
				glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
			} else {
				glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		// Write depth
		if ((current_state->flags ^ flags) & SEHLE_DEPTH_WRITE) {
			if (flags & SEHLE_DEPTH_WRITE) {
				glDepthMask (1);
			} else {
				glDepthMask (0);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		// Write color
		if ((current_state->flags ^ flags) & SEHLE_COLOR_WRITE) {
			if (flags & SEHLE_COLOR_WRITE) {
				glColorMask (1, 1, 1, 1);
			} else {
				glColorMask (0, 0, 0, 0);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		// Test depth
		if ((current_state->flags ^ flags) & (SEHLE_DEPTH_TEST | SEHLE_DEPTH_MASK)) {
			if (flags & SEHLE_DEPTH_TEST) {
				glEnable (GL_DEPTH_TEST);
				static unsigned int functions[] = { GL_LESS, GL_LEQUAL, GL_EQUAL, GL_GEQUAL, GL_GREATER, GL_NEVER, GL_ALWAYS };
				glEnable (GL_DEPTH_TEST);
				glDepthFunc (functions[(flags & SEHLE_DEPTH_MASK) >> SEHLE_DEPTH_SHIFT]);
			} else {
				glDisable (GL_DEPTH_TEST);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		// Blend
		if ((current_state->flags ^ flags) & (SEHLE_BLEND | SEHLE_BLEND_MASK)) {
			if (flags & SEHLE_BLEND) {
				static unsigned int equations[] = { GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX };
				static unsigned int functions[] = { GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };
				glEnable (GL_BLEND);
				glBlendEquation (equations[(flags & SEHLE_BLEND_EQ_MASK) >> SEHLE_BLEND_EQ_SHIFT]);
				glBlendFunc (functions[(flags & SEHLE_BLEND_SRC_MASK) >> SEHLE_BLEND_SRC_SHIFT], functions[(flags & SEHLE_BLEND_DST_MASK) >> SEHLE_BLEND_DST_SHIFT]);
			} else {
				glDisable (GL_BLEND);
			}
			SEHLE_CHECK_ERRORS (0);
		}
		current_state->flags = flags;
	}
}

void
sehle_update_program (SehleRenderState *current_state, SehleProgram *prog)
{
	if (current_state->program != prog) {
		if (current_state->program) az_object_unref (AZ_OBJECT (current_state->program));
		current_state->program = prog;
		if (current_state->program) {
			az_object_ref (AZ_OBJECT (current_state->program));
			glUseProgram (sehle_resource_get_handle (&current_state->program->resource));
#ifdef SEHLE_PERFORMANCE_MONITOR
			current_state->program->resource.engine->counter.program_switches += 1;
#endif
		} else {
			glUseProgram (0);
		}
	}
}

void
sehle_update_render_target (SehleRenderState *current_state, SehleRenderTarget *tgt)
{
	if (current_state->target != tgt) {
		if (current_state->target) az_object_unref (AZ_OBJECT (current_state->target));
		current_state->target = tgt;
		if (current_state->target) {
			az_object_ref (AZ_OBJECT (current_state->target));
			glBindFramebuffer (GL_FRAMEBUFFER, sehle_resource_get_handle (&current_state->target->resource));
#ifdef SEHLE_PERFORMANCE_MONITOR
			current_state->target->resource.engine->counter.rendertarget_binds += 1;
#endif
		} else {
			glBindFramebuffer (GL_FRAMEBUFFER, 0);
		}
	}
}

void
sehle_update_texture (SehleRenderState *current_state, unsigned int idx, SehleTexture *tex)
{
	if (current_state->textures[idx] != tex) {
		if (current_state->textures[idx]) az_object_unref (AZ_OBJECT (current_state->textures[idx]));
		current_state->textures[idx] = tex;
		if (current_state->textures[idx]) {
			az_object_ref (AZ_OBJECT (current_state->textures[idx]));
			/* fixme: use get_handle method and put OpenGL here? */
			sehle_texture_bind (current_state->textures[idx], idx);
		}
	}
}

void
sehle_update_viewport (SehleRenderState *current_state, const NRRectl *vport)
{
	if ((current_state->viewport.x0 != vport->x0) || (current_state->viewport.y0 != vport->y0) || (current_state->viewport.x1 != vport->x1) || (current_state->viewport.y1 != vport->y1)) {
		current_state->viewport = *vport;
		glViewport (current_state->viewport.x0, current_state->viewport.y0, current_state->viewport.x1 - current_state->viewport.x0, current_state->viewport.y1 - current_state->viewport.y0);
	}
}
