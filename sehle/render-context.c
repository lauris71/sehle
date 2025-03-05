#define __SEHLE_RENDER_CONTEXT_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

static const int debug = 0;

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "GL/glew.h"

#include <elea/vector2.h>

#ifdef _WIN32
#define strdup _strdup
#endif

#include "engine.h"
#include "light.h"
#include "texture-2d.h"
#include <sehle/material-depth.h>
#include "program-overlay.h"
#include "material-control.h"
#include "program.h"
#include <sehle/render-target.h>
#include "vertex-buffer.h"
#include "index-buffer.h"
// fixme:
#include "renderable.h"
#include "light-point.h"
#include "light-spot.h"

#include <sehle/render-context.h>


struct _SehleRenderListEntry {
	SehleRenderableHandle renderable;
	SehleMaterialHandle material;
	void *data;
};

static void
sehle_render_list_ensure_size (SehleRenderList *list, unsigned int size)
{
	if (list->size >= (list->length + size)) return;
	list->size += size;
	if (list->size < 64) list->size = 64;
	if (list->size < (list->length + size)) list->size = list->length + size;
	list->entries = (SehleRenderListEntry *) realloc (list->entries, list->size * sizeof (SehleRenderListEntry));
}

// Quicksort

static int
compare_rdata (const void *a, const void *b)
{
	SehleRenderListEntry *lhs = (SehleRenderListEntry *) a;
	SehleRenderListEntry *rhs = (SehleRenderListEntry *) b;
	// Priority 1 - material
	if ((const char *) lhs->material.impl < (const char *) rhs->material.impl) return -1;
	if ((const char *) lhs->material.impl > (const char *) rhs->material.impl) return 1;
	if ((const char *) lhs->material.inst < (const char *) rhs->material.inst) return -1;
	if ((const char *) lhs->material.inst > (const char *) rhs->material.inst) return 1;
	return 0;
}

static void
quick (float *d, int left, int right, uint32_t *i)
{
	if (right > left) {
		float pivot = d[left + (right - left) / 2];
		int r = right;
		int l = left;
		do {
			while (d[l] > pivot) l++;
			while (d[r] < pivot) r--;
			if (l <= r) {
				/* Swap distances */
				float t = d[l];
				d[l] = d[r];
				d[r] = t;
				/* Swap indices */
				for (int j = 0; j < 3; j++) {
					uint32_t it = i[3 * l + j];
					i[3 * l + j] = i[3 * r + j];
					i[3 * r + j] = it;
				}
				// Advance pointers
				l++;
				r--;
			}
		} while (l <= r);
		quick (d, left, r, i);
		quick (d, l, right, i);
	}
}

static void render_context_render_list (SehleRenderContext *ctx, unsigned int render_stage, unsigned int render_type, unsigned int material_mask, unsigned int value);

void
sehle_render_context_setup (SehleRenderContext *ctx, SehleEngine *engine)
{
	memset (ctx, 0, sizeof (SehleRenderContext));
	ctx->engine = engine;
	sehle_render_context_reset (ctx);
}

void
sehle_render_context_release (SehleRenderContext* ctx)
{
	if (ctx->render_list.entries) free (ctx->render_list.entries);
	sehle_render_state_finalize (&ctx->render_state);
}

void
sehle_render_context_reset (SehleRenderContext *ctx)
{
	ctx->v2w = EleaMat3x4fIdentity;
	ctx->w2v = EleaMat3x4fIdentity;
	ctx->proj = EleaMat4x4fIdentity;
	ctx->rproj = EleaMat4x4fIdentity;
	ctx->vspace.n = 6;
	elea_mat4x4f_get_sides_of_projection (ctx->vspace.p, &EleaMat4x4fIdentity);
	elea_plane3fp_set_xyzd (&ctx->clip_plane, 0, 0, 0, 0);

	sehle_render_state_init (&ctx->render_state);
	ctx->locked_state = 0;

	ctx->lodBias = 1;
	ctx->used_textures = 0;
	ctx->locked_textures = 0;

	ctx->depthchannel = -1;
	ctx->normalchannel = -1;
	ctx->albedochannel = -1;
	ctx->specularchannel = -1;
	elea_vec4fp_set_xyzw (&ctx->gbuf_area, 0, 0, 1, 1);

	ctx->global_ambient = EleaColor4fBlack;

	ctx->numlights = 0;
	ctx->nreflections = 0;
	ctx->n_refl_frags = 0;

	ctx->render_list.length = 0;
}

void
sehle_render_context_end (SehleRenderContext* ctx)
{
}

void
sehle_render_context_set_view (SehleRenderContext *ctx, const EleaMat3x4f *v2w, const EleaMat4x4f *proj)
{
	ctx->v2w = *v2w;
	elea_mat3x4f_invert_normalized (&ctx->w2v, v2w);
	ctx->proj = *proj;
	elea_mat4x4f_invert (&ctx->rproj, &ctx->proj);
	ctx->vspace.n = 6;
	elea_mat4x4f_get_sides_of_projection (ctx->vspace.p, &ctx->proj);
	for (unsigned int i = 0; i < 6; i++) {
		elea_mat3x4f_transform_plane (&ctx->vspace.p[i], v2w, &ctx->vspace.p[i]);
	}
}

float
sehle_render_context_get_ppu (SehleRenderContext *ctx, float z_eye)
{
	/* In front of camera */
	EleaVec4f v0, v;
	EleaVec2f d0, d;
	float width = (float) (ctx->render_state.viewport.x1 - ctx->render_state.viewport.x0);
	float height = (float) (ctx->render_state.viewport.y1 - ctx->render_state.viewport.y0);
	elea_vec4fp_set_xyzw (&v0, 0, 0, z_eye, 1);
	elea_mat4x4f_transform_vec (&v0, &ctx->proj, &v0);
	elea_vec4fp_set_xyzw (&v, 0.7071067811f, 0.7071067811f, z_eye, 1);
	elea_mat4x4f_transform_vec (&v, &ctx->proj, &v);
	d0.x = v0.x / v0.w;
	d0.y = v0.y / v0.w;
	d0.x = width * 0.5f * (d0.x - 1.0f);
	d0.y = height * 0.5f * (d0.y - 1.0f);
	d.x = v.x / v.w;
	d.y = v.y / v.w;
	d.x = width * 0.5f * (d.x - 1.0f);
	d.y = height * 0.5f * (d.y - 1.0f);
	d = elea_vec2f_sub (d, d0);
	return elea_vec2f_len (d);
}

void
sehle_render_context_schedule_render (SehleRenderContext *ctx,
	SehleRenderableImplementation *rend_impl, SehleRenderableInstance *rend_inst,
	SehleMaterialImplementation *mat_impl, SehleMaterialInstance *mat_inst, void *data)
{
	sehle_render_list_ensure_size (&ctx->render_list, 1);
	SehleRenderListEntry *entry = &ctx->render_list.entries[ctx->render_list.length];
	entry->renderable.impl = rend_impl;
	entry->renderable.inst = rend_inst;
	entry->material.impl = mat_impl;
	entry->material.inst = mat_inst;
	entry->data = data;
	ctx->render_list.length += 1;
}

void
sehle_render_context_schedule_render_sorted_triangles (SehleRenderContext* ctx,
	SehleRenderableImplementation *rend_impl, SehleRenderableInstance *rend_inst, void *data,
	SehleVertexArray* va, unsigned int first, unsigned int nindices,
	SehleMaterialImplementation* mat_impl, SehleMaterialInstance* mat_inst, const EleaMat3x4f *o2w)
{
	if (!nindices) return;
	sehle_render_list_ensure_size (&ctx->render_list, 1);
	SehleRenderListEntry *entry = &ctx->render_list.entries[ctx->render_list.length];
	entry->renderable.impl = rend_impl;
	entry->renderable.inst = rend_inst;
	entry->material.impl = mat_impl;
	entry->material.inst = mat_inst;
	entry->data = data;
	ctx->render_list.length += 1;
	// Reallocate distance buffer if needed
	static unsigned int sizedistances = 0;
	static float *distances = NULL;
	unsigned int ndistances = nindices / 3;
	if (sizedistances < ndistances) {
		sizedistances = sizedistances << 1;
		if (sizedistances < 16384) sizedistances = 16384;
		if (sizedistances < ndistances) sizedistances = ndistances;
		distances = (float *) realloc (distances, sizedistances * sizeof (float));
	}
	// fixme: Should we reorder indices in indexbuffer or copy these?
	SehleVertexBuffer* vb = va->vbufs[SEHLE_ATTRIBUTE_VERTEX];
	uint32_t *idxs = sehle_index_buffer_map (va->ibuf, SEHLE_BUFFER_READ_WRITE);
	const float *values = (const float *) sehle_vertex_buffer_map (vb, SEHLE_BUFFER_READ);
	EleaMat3x4f o2v;
	elea_mat3x4f_multiply (&o2v, &ctx->w2v, o2w);
	for (unsigned int i = 0; i < ndistances; i++) {
		distances[i] = 0;
		for (unsigned int j = 0; j < 3; j++) {
			EleaVec3f vp;
			elea_mat3x4f_transform_point (&vp, &o2v, (EleaVec3f *) &values[idxs[first + 3 * i] * vb->buffer.stride + vb->offsets[SEHLE_ATTRIBUTE_VERTEX]]);
			distances[i] -= vp.z;
		}
	}
	sehle_vertex_buffer_unmap (vb);
	quick (distances, 0, (int) ndistances - 1, idxs + first);
	sehle_index_buffer_unmap (va->ibuf);
#ifdef SEHLE_PERFORMANCE_MONITOR
	ctx->engine->counter.sortedtriangles += ndistances;
#endif
}

void
sehle_render_context_add_reflection (SehleRenderContext *ctx, const EleaPlane3f *plane, SehleVertexArray *va, unsigned int first, unsigned int nindices,
	const EleaMat3x4f *o2w, SehleMaterialReflectingImplementation *mat_impl, SehleMaterialReflectingInstance *mat_inst)
{
	if (!nindices) return;
	if (ctx->n_refl_frags >= SEHLE_RENDER_CONTEXT_MAX_REFLECTING_FRAGS) return;
	// Search for existing reflection or create new
	unsigned int ridx, i;
	for (ridx = 0; ridx < ctx->nreflections; ridx++) {
		for (i = 0; i < 4; i++) {
			if (ctx->reflections[ridx].plane.c[i] == plane->c[i]) break;
		}
		if (i < 4) break;
	}
	if (ridx >= ctx->nreflections) {
		if (ctx->nreflections >= SEHLE_RENDER_CONTEXT_MAX_REFLECTIONS) return;
		ctx->reflections[ctx->nreflections].plane = *plane;
		ctx->nreflections += 1;
	}
	// Create new reflectingfrag
	SehleReflectingFragment *rd = &ctx->refl_frags[ctx->n_refl_frags++];
	rd->reflection = ridx;
	rd->va = va;
	rd->first = first;
	rd->nindices = nindices;
	rd->r2w = *o2w;
	rd->mat_impl = mat_impl;
	rd->mat_inst = mat_inst;
}

void
sehle_render_context_clear (SehleRenderContext* ctx, unsigned int depth, unsigned int color, const EleaColor4f* bg)
{
	GLbitfield mask = 0;
	/* fixme: This is suboptimal - bind only target and viewport? */
	sehle_render_context_bind (ctx);
 	if (depth) {
		glClearDepth (1.0f);
		mask |= GL_DEPTH_BUFFER_BIT;
	}
	if (color) {
		glClearColor (bg->r, bg->g, bg->b, bg->a);
		mask |= GL_COLOR_BUFFER_BIT;
	}
	glClear (mask);
}

void
sehle_render_context_render_depth (SehleRenderContext *ctx, unsigned int solid, unsigned int transparent)
{
	ctx->render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
	sehle_render_flags_clear (&ctx->render_state.flags, SEHLE_COLOR_WRITE);
	ctx->locked_state = SEHLE_COLOR_WRITE;
	//sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
	if (solid) render_context_render_list (ctx, SEHLE_STAGE_SOLID, SEHLE_RENDER_DEPTH, 0, 0);
	if (transparent) render_context_render_list (ctx, SEHLE_STAGE_TRANSPARENT, SEHLE_RENDER_DEPTH, 0, 0);
}

void
sehle_render_context_render_shadow (SehleRenderContext *ctx)
{
	// fixme: Put this in render state
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1, 4);
	ctx->render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
	sehle_render_flags_clear (&ctx->render_state.flags, SEHLE_COLOR_WRITE);
	ctx->locked_state = SEHLE_COLOR_WRITE;
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
	render_context_render_list (ctx, SEHLE_STAGE_SOLID, SEHLE_RENDER_DEPTH, 0, 0);
	/* Semitransparent objects should cast density, not shadow */
	//sehle_render_context_render (ctx, SEHLE_STAGE_TRANSPARENT, SEHLE_RENDER_DEPTH, 0, 0);
	glPolygonOffset (0, 0);
	glDisable (GL_POLYGON_OFFSET_FILL);
}

void
sehle_render_context_render_density (SehleRenderContext* ctx)
{
	ctx->render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
	sehle_render_flags_clear (&ctx->render_state.flags, SEHLE_DEPTH_WRITE);
	sehle_render_flags_set_depth_test (&ctx->render_state.flags, 1, SEHLE_DEPTH_LEQUAL);
	ctx->locked_state = SEHLE_DEPTH_WRITE | SEHLE_DEPTH_TEST | SEHLE_DEPTH_MASK;
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
	render_context_render_list (ctx, SEHLE_STAGE_TRANSPARENT, SEHLE_RENDER_DENSITY, 0, 0);
}

void
sehle_render_context_render_transparent (SehleRenderContext* ctx)
{
	// fixme:
	// Probably we should lock depth test, but in that case we have to move sky to separate stage (that should be done anyways)
	ctx->render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
	sehle_render_flags_set (&ctx->render_state.flags, SEHLE_BLEND);
	ctx->locked_state = SEHLE_BLEND;
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
	render_context_render_list (ctx, SEHLE_STAGE_TRANSPARENT, SEHLE_RENDER_TRANSPARENT, 0, 0);
}

void
sehle_render_context_render_forward (SehleRenderContext* ctx)
{
	ctx->render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
	sehle_render_flags_set_depth_test (&ctx->render_state.flags, 1, SEHLE_DEPTH_LEQUAL);
	sehle_render_flags_set (&ctx->render_state.flags, SEHLE_BLEND);
	ctx->locked_state = SEHLE_DEPTH_TEST | SEHLE_BLEND | SEHLE_DEPTH_MASK | SEHLE_BLEND_MASK;
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
	render_context_render_list (ctx, SEHLE_STAGE_FORWARD, SEHLE_RENDER_FORWARD, 0, 0);
}

void
sehle_render_context_render (SehleRenderContext *ctx, unsigned int render_stage, unsigned int render_type, unsigned int mat_mask, unsigned int mat_val)
{
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
	render_context_render_list (ctx, render_stage, render_type, mat_mask, mat_val);
}

void
sehle_render_context_render_gbuffer (SehleRenderContext *ctx)
{
	ctx->render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
	ctx->locked_state = 0;
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
	render_context_render_list (ctx, SEHLE_STAGE_SOLID, SEHLE_RENDER_GBUFFER, 0, 0);
}

void
sehle_render_context_render_occlusion (SehleRenderContext *ctx)
{
	ctx->render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
	sehle_render_flags_set_depth_test (&ctx->render_state.flags, 1, SEHLE_DEPTH_GREATER);
	sehle_render_flags_set_blend (&ctx->render_state.flags, 1, SEHLE_BLEND_ADD, SEHLE_BLEND_DST_COLOR, SEHLE_BLEND_ZERO);
	sehle_render_flags_clear (&ctx->render_state.flags, SEHLE_DEPTH_WRITE);
	ctx->locked_state = (SEHLE_DEPTH_WRITE | SEHLE_DEPTH_MASK | SEHLE_BLEND | SEHLE_BLEND_MASK);
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
	// fixme: This should be integrated into render state somehow
	// fixme: Only apply to albedo/occlusion buffer?
	glColorMask (0, 0, 0, 1);
	SEHLE_CHECK_ERRORS (0);
	render_context_render_list (ctx, SEHLE_STAGE_AMBIENT, SEHLE_RENDER_AMBIENT, 0, 0);
	glColorMask (1, 1, 1, 1);
	SEHLE_CHECK_ERRORS (0);
}

void
sehle_render_context_render_lights (SehleRenderContext* ctx)
{
	/* Light geometry without stencils */
	ctx->render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
	sehle_render_flags_clear (&ctx->render_state.flags, SEHLE_DEPTH_WRITE);
	sehle_render_flags_set_blend (&ctx->render_state.flags, 1, SEHLE_BLEND_ADD, SEHLE_BLEND_ONE, SEHLE_BLEND_ONE);
	ctx->locked_state = SEHLE_DEPTH_WRITE | SEHLE_BLEND | SEHLE_BLEND_MASK;
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
	/* Skip those lights that have stencil flag set at moment */
	render_context_render_list (ctx, SEHLE_STAGE_LIGHTS, SEHLE_RENDER_LIGHTMAP, SEHLE_MATERIAL_STENCIL, 0);

	// Render stenciled lightmaps
	// Set up stencil
	// We initialize it to 127 because we render back and front faces in the same pass
	glStencilMask (255);
	glClearStencil (127);
	glEnable (GL_STENCIL_TEST);

	/* fixme: Create two-pass render function */
	// Iterate
	for (unsigned int i = 0; i < ctx->render_list.length; i++) {
		SehleRenderListEntry *entry = &ctx->render_list.entries[i];
		if (!(entry->material.inst->render_types & SEHLE_RENDER_STENCIL)) continue;
		if (!(entry->material.inst->properties & SEHLE_MATERIAL_STENCIL)) continue;

		/* First stencil pass */
		ctx->render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
		sehle_render_flags_clear (&ctx->render_state.flags, SEHLE_CULL | SEHLE_DEPTH_WRITE | SEHLE_COLOR_WRITE);
		sehle_render_flags_set_depth_test (&ctx->render_state.flags, 1, SEHLE_DEPTH_GREATER);
		ctx->locked_state = SEHLE_CULL | SEHLE_DEPTH_WRITE | SEHLE_COLOR_WRITE | SEHLE_DEPTH_MASK;
		sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
		glClear (GL_STENCIL_BUFFER_BIT);
		glStencilFuncSeparate (GL_FRONT, GL_ALWAYS, 0, 255);
		glStencilFuncSeparate (GL_BACK, GL_ALWAYS, 0, 255);
		glStencilOpSeparate (GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR);
		glStencilOpSeparate (GL_BACK, GL_KEEP, GL_KEEP, GL_INCR);
		sehle_material_bind (entry->material.impl, entry->material.inst, ctx, SEHLE_RENDER_STENCIL);
		sehle_renderable_render (entry->renderable.impl, entry->renderable.inst, ctx, ctx->render_state.program, SEHLE_RENDER_STENCIL, entry->data);

		// Second stencil pass
		ctx->render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
		sehle_render_flags_clear (&ctx->render_state.flags, SEHLE_DEPTH_WRITE);
		sehle_render_flags_set_blend (&ctx->render_state.flags, 1, SEHLE_BLEND_ADD, SEHLE_BLEND_ONE, SEHLE_BLEND_ONE);
		ctx->locked_state = SEHLE_DEPTH_WRITE | SEHLE_BLEND | SEHLE_BLEND_MASK;
		sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
		// fixme: Use CW culling instead
		glStencilFuncSeparate (GL_FRONT, GL_LESS, 127, 255);
		glStencilFuncSeparate (GL_BACK, GL_NEVER, 127, 255);
		glEnable (GL_STENCIL_TEST);
		sehle_material_bind (entry->material.impl, entry->material.inst, ctx, SEHLE_RENDER_LIGHTMAP);
		sehle_renderable_render (entry->renderable.impl, entry->renderable.inst, ctx, ctx->render_state.program, SEHLE_RENDER_LIGHTMAP, entry->data);
	}
	// Disable stencil
	glStencilMask (0);
	glDisable (GL_STENCIL_TEST);

	sehle_render_flags_set_blend (&ctx->render_state.flags, 1, SEHLE_BLEND_ADD, SEHLE_BLEND_SRC_ALPHA, SEHLE_BLEND_ONE_MINUS_SRC_ALPHA);
}

void
sehle_render_context_bind_gbuf_textures (SehleRenderContext* ctx, SehleTexture2D* depth, SehleTexture2D* normal, SehleTexture2D* albedo, SehleTexture2D* specular, float x0, float y0, float x1, float y1)
{
	elea_vec4fp_set_xyzw (&ctx->gbuf_area, x0, y0, x1, y1);
	if (depth) {
		if (ctx->depthchannel < 0) ctx->depthchannel = sehle_render_context_set_texture (ctx, &depth->texture);
		ctx->locked_textures |= (1 << ctx->depthchannel);
		sehle_texture_bind (&depth->texture, ctx->depthchannel);
	} else {
		ctx->depthchannel = -1;
	}
	if (normal) {
		if (ctx->normalchannel < 0) ctx->normalchannel = sehle_render_context_set_texture (ctx, &normal->texture);
		ctx->locked_textures |= (1 << ctx->normalchannel);
		sehle_texture_bind (&normal->texture, ctx->normalchannel);
	} else {
		ctx->normalchannel = -1;
	}
	if (albedo) {
		if (ctx->albedochannel < 0) ctx->albedochannel = sehle_render_context_set_texture (ctx, &albedo->texture);
		ctx->locked_textures |= (1 << ctx->albedochannel);
		sehle_texture_bind (&albedo->texture, ctx->albedochannel);
	} else {
		ctx->albedochannel = -1;
	}
	if (specular) {
		if (ctx->specularchannel < 0) ctx->specularchannel = sehle_render_context_set_texture (ctx, &specular->texture);
		ctx->locked_textures |= (1 << ctx->specularchannel);
		sehle_texture_bind (&specular->texture, ctx->specularchannel);
	} else {
		ctx->specularchannel = -1;
	}
}

/*
 * Render a (part of) list collected in display stage
 * Fragment will be rendered if ! materialflags || !material->properties || (materialflags & material->properties)
 */

static void
render_context_render_list (SehleRenderContext *ctx, unsigned int render_stage, unsigned int render_type, unsigned int material_mask, unsigned int value)
{
	SehleMaterialImplementation *last_impl = NULL;
	SehleMaterialInstance *last_inst = NULL;
	for (unsigned int i = 0; i < ctx->render_list.length; i++) {
		SehleRenderListEntry *rdata = &ctx->render_list.entries[i];
		//if (!(rdata->stage & render_type)) continue;
		/* If depth and density stages are displayed together we have to separate by stage because transparent materials also have depth rendertype */
		/* if (!(rdata->material.inst->render_stages & render_stage)) continue; */
		if (!(rdata->material.inst->render_types & render_type)) continue;
		if ((material_mask & rdata->material.inst->properties) != value) continue;
		if (rdata->material.inst != last_inst) {
			last_impl = rdata->material.impl;
			last_inst = rdata->material.inst;
			ctx->used_textures = ctx->locked_textures;
			SEHLE_CHECK_ERRORS (0);
			sehle_material_bind (rdata->material.impl, rdata->material.inst, ctx, render_type);
			SEHLE_CHECK_ERRORS (0);
		}
		SEHLE_CHECK_ERRORS (0);
		/* fixme: If program is fetched from ctx, remove it from arguments as well */
		sehle_renderable_render (rdata->renderable.impl, rdata->renderable.inst, ctx, ctx->render_state.program, render_type, rdata->data);
		SEHLE_CHECK_ERRORS (0);
	}
	ctx->used_textures = ctx->locked_textures;
}

unsigned int
sehle_render_context_query_reflection_visibility (SehleRenderContext *ctx, unsigned int refl_idx)
{
	SehleProgram *prog = sehle_depth_program_get_reference (ctx->engine, 0, 1);

	// fixme: Keep query?
	static unsigned int query = 0;
	if (!query) glGenQueries (1, &query);

	/* fixme: Does water render depth for some reason - in that case we need to lock DEPTH_LEQUAL */
	//render_state.flags = (SEHLE_RENDER_STATE_DEFAULT & ~SEHLE_DEPTH_MASK) | (SEHLE_DEPTH_TEST | SEHLE_DEPTH_LEQUAL);
	//sehle_render_flags_clear (&render_state.flags, SEHLE_DEPTH_WRITE | SEHLE_COLOR_WRITE | SEHLE_DEPTH_MASK);
	ctx->render_state.flags = SEHLE_RENDER_STATE_DEFAULT;
	sehle_render_flags_clear (&ctx->render_state.flags, SEHLE_DEPTH_WRITE | SEHLE_COLOR_WRITE);
	ctx->locked_state = SEHLE_DEPTH_WRITE | SEHLE_COLOR_WRITE;
	sehle_render_context_set_program (ctx, prog);
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
	sehle_program_depth_bind_instance (prog, ctx, SEHLE_RENDER_DEPTH);
	glBeginQuery (GL_SAMPLES_PASSED, query);

	EleaMat3x4f o2v;
	elea_mat3x4f_multiply (&o2v, &ctx->w2v, &ctx->refl_frags[refl_idx].r2w);
	sehle_program_depth_render (prog, ctx->refl_frags[refl_idx].va, ctx->refl_frags[refl_idx].first, ctx->refl_frags[refl_idx].nindices, &o2v, &ctx->proj);

	glEndQuery (GL_SAMPLES_PASSED);
	int params;
	glGetQueryObjectiv (query, GL_QUERY_RESULT, &params);

	az_object_unref ((AZObject *) prog);

	return (unsigned int) params;
}

void
sehle_render_context_set_reflection_texture (SehleRenderContext *ctx, unsigned int idx, SehleTexture2D* texture, const EleaMat3x4f *refl_w2v, const EleaMat4x4f *refl_proj)
{
	if (idx >= ctx->nreflections) return;
	if (ctx->reflections[idx].texture) {
		//az_object_unref ((AZObject *) ctx->reflections[idx].texture);
	}
	ctx->reflections[idx].texture = texture;
	if (ctx->reflections[idx].texture) {
		//az_object_ref (( AZObject*) ctx->reflections[idx].texture);
	}
	ctx->reflections[idx].w2v = *refl_w2v;
	ctx->reflections[idx].proj = *refl_proj;
}

void
sehle_render_context_bind (SehleRenderContext *ctx)
{
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
}

void
sehle_render_context_display_frame (SehleRenderContext *ctx, SehleRenderableImplementation *impl, SehleRenderableInstance *inst, unsigned int display_mask, unsigned int render_stages)
{
	SehleDisplayContext display_ctx;
	display_ctx.mask = display_mask;
	display_ctx.stages = render_stages;
	display_ctx.level = 0;
	sehle_renderable_display (impl, inst, ctx, &display_ctx);
	// Sort by state
	qsort (ctx->render_list.entries, ctx->render_list.length, sizeof (SehleRenderListEntry), compare_rdata);
}

void
sehle_render_context_finish_frame (SehleRenderContext *ctx)
{
	// Clear render list
	ctx->render_list.length = 0;
	for (unsigned int i = 0; i < ctx->nreflections; i++) {
		if (ctx->reflections[i].texture) {
			//az_object_unref (AZ_OBJECT (ctx->reflections[i].texture));
		}
		ctx->reflections[i].texture = NULL;
	}
	ctx->nreflections = 0;
	ctx->n_refl_frags = 0;
	ctx->numlights = 0;
}

void
sehle_render_context_set_render_state (SehleRenderContext *ctx, unsigned int state)
{
	ctx->render_state.flags = (ctx->render_state.flags & ctx->locked_state) | (state & ~ctx->locked_state);
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
}

void
sehle_render_context_set_program (SehleRenderContext *ctx, SehleProgram *prog)
{
	sehle_render_state_set_program (&ctx->render_state, prog);
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
}

void
sehle_render_context_set_target (SehleRenderContext *ctx, SehleRenderTarget *tgt)
{
	sehle_render_state_set_target (&ctx->render_state, tgt);
	sehle_engine_set_render_state (ctx->engine, &ctx->render_state);
}

void
sehle_render_context_set_viewport (SehleRenderContext *ctx, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
	sehle_render_state_set_viewport (&ctx->render_state, x0, y0, x1, y1);
}

int
sehle_render_context_set_texture (SehleRenderContext *ctx, SehleTexture *tex)
{
	unsigned int channel;
	for (channel = 0; channel < SEHLE_RENDER_STATE_NUM_TEXTURES; channel++) {
		unsigned int texmask = (1 << channel);
		if (!(ctx->used_textures & texmask)) {
			ctx->used_textures |= texmask;
			sehle_render_state_set_texture (&ctx->render_state, channel, tex);
			return channel;
		}
	}
	return -1;
}

void
sehle_render_context_set_texture_to_channel (SehleRenderContext *ctx, SehleTexture *tex, unsigned int channel)
{
	unsigned int  texmask = (1 << channel);
	ctx->used_textures |= texmask;
	sehle_render_state_set_texture (&ctx->render_state, channel, tex);
}

void
sehle_render_context_release_texture (SehleRenderContext* ctx, unsigned int channel)
{
	unsigned int texmask = (1 << channel);
	ctx->used_textures &= ~texmask;
}

void
sehle_render_context_add_light (SehleRenderContext *ctx, SehleLightInstance *light)
{
	unsigned int light_idx;
	if (ctx->numlights < SEHLE_RENDER_CONTEXT_MAX_LIGHTS) {
		light_idx = ctx->numlights++;
	} else {
		light_idx = 0;
		float min_priority = ctx->lights[0]->priority;
		for (unsigned int i = 1; i < SEHLE_RENDER_CONTEXT_MAX_LIGHTS; i++) {
			if (ctx->lights[i]->priority < min_priority) {
				light_idx = i;
				min_priority = ctx->lights[i]->priority;
			}
		}
		if (min_priority >= light->priority) return;
	}
	ctx->lights[light_idx] = light;
}

void
sehle_render_context_get_vp2gbuf_transform (SehleRenderContext *ctx, float v[])
{
	v[0] = (ctx->gbuf_area.c[2] - ctx->gbuf_area.c[0]) / (ctx->render_state.viewport.x1 - ctx->render_state.viewport.x0);
	v[1] = (ctx->gbuf_area.c[3] - ctx->gbuf_area.c[1]) / (ctx->render_state.viewport.y1 - ctx->render_state.viewport.y0);
	v[2] = ctx->gbuf_area.c[0] - ctx->render_state.viewport.x0 * (ctx->gbuf_area.c[2] - ctx->gbuf_area.c[0]) / (ctx->render_state.viewport.x1 - ctx->render_state.viewport.x0);
	v[3] = ctx->gbuf_area.c[1] - ctx->render_state.viewport.y0 * (ctx->gbuf_area.c[3] - ctx->gbuf_area.c[1]) / (ctx->render_state.viewport.y1 - ctx->render_state.viewport.y0);
}

void
sehle_render_context_set_gbuffer_uniforms (SehleRenderContext *ctx, SehleProgram *prog, int w2g, int g2e)
{
	EleaMat4x4f rproj_map;
	float c[4];
	if (g2e >= 0) {
		/* Reverse transformation */
		EleaMat4x4f map = { 2 / (ctx->gbuf_area.c[2] - ctx->gbuf_area.c[0]), 0, 0, 0,
			0, 2 / (ctx->gbuf_area.c[3] - ctx->gbuf_area.c[1]), 0, 0,
			0, 0, 2, 0,
			(ctx->gbuf_area.c[2] + ctx->gbuf_area.c[0]) / (ctx->gbuf_area.c[0] - ctx->gbuf_area.c[2]), (ctx->gbuf_area.c[3] + ctx->gbuf_area.c[1]) / (ctx->gbuf_area.c[1] - ctx->gbuf_area.c[3]), -1, 1 };
		elea_mat4x4f_multiply (&rproj_map, &ctx->rproj, &map);
		sehle_program_setUniformMatrix4fv (prog, g2e, 1, rproj_map.c);
	}
	if (w2g >= 0) {
		sehle_render_context_get_vp2gbuf_transform (ctx, c);
		if (w2g >= 0) sehle_program_setUniform4fv (prog, w2g, 1, c);
	}
}

void
sehle_render_context_draw_overlay_rect_2d (SehleRenderContext *ctx, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1,
	float tex_x0, float tex_y0, float tex_x1, float tex_y1)
{
	if ((x0 >= x1) || (y0 >= y1)) return;
	SEHLE_CHECK_ERRORS (0);

	float width2 = (ctx->render_state.viewport.x1 - ctx->render_state.viewport.x0) / 2.0f;
	float xscale = 2.0f / (ctx->render_state.viewport.x1 - ctx->render_state.viewport.x0);
	float height2 = (ctx->render_state.viewport.y1 - ctx->render_state.viewport.y0) / 2.0f;
	float yscale = 2.0f / (ctx->render_state.viewport.y1 - ctx->render_state.viewport.y0);

	float nx0 = (x0 - width2) * xscale;
	float ny0 = (y0 - height2) * yscale;
	float nx1 = (x1 - width2) * xscale;
	float ny1 = (y1 - height2) * yscale;

	float tx0 = tex_x0;
	float ty0 = tex_y0;
	float tx1 = tex_x1;
	float ty1 = tex_y1;

	SehleVertexArray *va = sehle_engine_get_standard_geometry (ctx->engine, SEHLE_GEOMETRY_STREAM_256);
	float* d = sehle_vertex_buffer_map (va->vbufs[0], SEHLE_BUFFER_WRITE);
	d[0] = nx0;
	d[1] = ny0;
	d[2] = tx0;
	d[3] = ty0;
	d[8] = nx1;
	d[9] = ny0;
	d[10] = tx1;
	d[11] = ty0;
	d[16] = nx1;
	d[17] = ny1;
	d[18] = tx1;
	d[19] = ty1;
	d[24] = nx0;
	d[25] = ny1;
	d[26] = tx0;
	d[27] = ty1;
	sehle_vertex_buffer_unmap (va->vbufs[0]);
	SEHLE_CHECK_ERRORS (0);
	sehle_vertex_array_render_triangles (va, 1, 0, 6);
	az_object_unref ((AZObject *) va);
}

