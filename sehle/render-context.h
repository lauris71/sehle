#ifndef __SEHLE_RENDER_CONTEXT_H__
#define __SEHLE_RENDER_CONTEXT_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

typedef struct _SehleRenderListEntry SehleRenderListEntry;
typedef struct _SehleRenderList SehleRenderList;
typedef struct _SehleReflection SehleReflection;
typedef struct _SehleReflectingFragment SehleReflectingFragment;
typedef struct _SehleOcclusionQuery SehleOcclusionQuery;

#include <elea/aabox.h>
#include <elea/color.h>
#include <elea/matrix3x4.h>
#include <elea/matrix4x4.h>
#include <elea/plane.h>
#include <elea/polyhedron.h>

#include <sehle/sehle.h>
#include <sehle/render-state.h>

#ifdef WIN32
#undef TRANSPARENT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Display Context
 */

struct _SehleDisplayContext {
	/* Culling mask for renderables */
	unsigned int mask;
	/* Render stage mask */
	unsigned int stages;
	/* Display level for occlusion sorting */
	unsigned int level;
};

/*
 * Render data
 *
 * This contains everything needed for invoking ::render method of materials
 */

struct _SehleRenderList {
	unsigned int size;
	unsigned int length;
	SehleRenderListEntry *entries;
};

struct _SehleReflection {
	// Reflection plane
	EleaPlane3f plane;
	// Rendered texture
	SehleTexture2D* texture;
	// Reflection transforms
	EleaMat3x4f w2v;
	EleaMat4x4f proj;
};

struct _SehleReflectingFragment {
	/* Associated reflection */
	unsigned int reflection;
	/* Render data for occlusion query */
	SehleVertexArray *va;
	unsigned int first;
	unsigned int nindices;
	EleaMat3x4f r2w;
	/* Associated material */
	SehleMaterialReflectingImplementation *mat_impl;
	SehleMaterialReflectingInstance *mat_inst;
};

/*
 * Occlusion query
 *
 * Renderables can submit these in ::display implementation
 * If query passes ::display is invoked again with level value + 1
 */

struct _SehleOcclusionQuery {
	unsigned int level;
	SehleVertexArray *va;
	unsigned int first;
	unsigned int n_indices;
	SehleRenderableImplementation *rend_impl;
	SehleRenderableInstance *rend_inst;
};

#define SEHLE_RENDER_CONTEXT_MAX_LIGHTS 4
#define SEHLE_RENDER_CONTEXT_MAX_REFLECTIONS 2
#define SEHLE_RENDER_CONTEXT_MAX_REFLECTING_FRAGS 32

struct _SehleRenderContext {
	SehleEngine *engine;
	/* View data */
	EleaMat3x4f v2w;
	EleaMat3x4f w2v;
	EleaMat4x4f proj;
	EleaMat4x4f rproj;
	EleaPolyhedron3f vspace;
	EleaPlane3f clip_plane;
	/* State */
	SehleRenderState render_state;
	unsigned int locked_state;

	// LOD bias
	float lodBias;

	/* Mask of texture handles in use */
	/* Used by RenderContext (GBuffer textures) */
	unsigned int locked_textures;
	/* Currently in use, will be reset to locked_textures for each new render call */
	unsigned int used_textures;

	// GBuffer data
	int depthchannel;
	int normalchannel;
	int albedochannel;
	int specularchannel;
	/* Viewport-relative area of gbuffer */
	EleaVec4f gbuf_area;

	/* Lighting */
	EleaColor4f global_ambient;

	unsigned int numlights;
	SehleLightInstance *lights[SEHLE_RENDER_CONTEXT_MAX_LIGHTS];

	// Reflections
	// List of reflected images
	unsigned int nreflections;
	SehleReflection reflections[SEHLE_RENDER_CONTEXT_MAX_REFLECTIONS];
	unsigned int n_refl_frags;
	SehleReflectingFragment refl_frags[SEHLE_RENDER_CONTEXT_MAX_REFLECTING_FRAGS];

	/* Display */
	SehleRenderList render_list;
};

void sehle_render_context_schedule_render (SehleRenderContext *ctx,
	SehleRenderableImplementation *rend_impl, SehleRenderableInstance *rend_inst,
	SehleMaterialImplementation *mat_impl, SehleMaterialInstance *mat_inst, void *data);
void sehle_render_context_schedule_render_sorted_triangles (SehleRenderContext* ctx,
	SehleRenderableImplementation *rend_impl, SehleRenderableInstance *rend_inst, void *data,
	SehleVertexArray* va, unsigned int first, unsigned int nindices,
	SehleMaterialImplementation* mat_impl, SehleMaterialInstance* mat_inst, const EleaMat3x4f* o2w);
void sehle_render_context_add_reflection (SehleRenderContext *ctx, const EleaPlane3f *plane, SehleVertexArray *va, unsigned int first, unsigned int nindices,
	const EleaMat3x4f *o2w, SehleMaterialReflectingImplementation *mat_impl, SehleMaterialReflectingInstance *mat_inst);

unsigned int sehle_render_context_query_reflection_visibility (SehleRenderContext *ctx, unsigned int refl_idx);

void sehle_render_context_setup (SehleRenderContext* ctx, SehleEngine* engine);
void sehle_render_context_release (SehleRenderContext* ctx);
void sehle_render_context_reset (SehleRenderContext* ctx);
void sehle_render_context_end (SehleRenderContext* ctx);
void sehle_render_context_finish_frame (SehleRenderContext* ctx);
void sehle_render_context_set_view (SehleRenderContext* ctx, const EleaMat3x4f* v2w, const EleaMat4x4f* proj);

/* Get pixels per unit at certain z in diagonal direction */
float sehle_render_context_get_ppu (SehleRenderContext *ctx, float z_eye);

/* Update all engine states to values in given RenderContext */
void sehle_render_context_bind (SehleRenderContext *ctx);

/* Schedule rendering */
void sehle_render_context_display_frame (SehleRenderContext *ctx, SehleRenderableImplementation *impl, SehleRenderableInstance *inst, unsigned int display_mask, unsigned int render_stages);

/* Rendering */
/* Applies render state first */
void sehle_render_context_render (SehleRenderContext *ctx, unsigned int render_stage, unsigned int render_type, unsigned int mat_mask, unsigned int mat_val);
/* All methods modify and apply render state first */
void sehle_render_context_clear (SehleRenderContext* ctx, unsigned int depth, unsigned int color, const EleaColor4f *bg);
/* Stages SOLID and/or TRANSPARENT, render type DEPTH, color write disabled */
void sehle_render_context_render_depth (SehleRenderContext *ctx, unsigned int solid, unsigned int transparent);
/* Stage LIGHT, render types LIGHTMAP and STENCIL, additive blending, depth write disabled */
void sehle_render_context_render_lights (SehleRenderContext *ctx);
/* Stages SOLID and TRANSPARENT, render type DEPTH, color write disabled, polygon offset on */
void sehle_render_context_render_shadow (SehleRenderContext* ctx);
/* Stage TRANSPARENT, render type DENSITY, depth LE, depth write off */
void sehle_render_context_render_density (SehleRenderContext* ctx);
/* Stage TRANSPARENT, render type TRANSLARENT, blending on */
void sehle_render_context_render_transparent (SehleRenderContext* ctx);
/* Stage FORWARD, render type FORWARD, depth LE, blending on */
void sehle_render_context_render_forward (SehleRenderContext* ctx);
/* Stage SOLID, render type GBUFFER */
void sehle_render_context_render_gbuffer (SehleRenderContext *ctx);
/* Stage OCCLUSION, render type OCCLUSION, depth gt, depth write off, blending on, color mask alpha */
void sehle_render_context_render_occlusion (SehleRenderContext *ctx);

/* Set render state obeying locking */
void sehle_render_context_set_render_state (SehleRenderContext *ctx, unsigned int state);
void sehle_render_context_set_program (SehleRenderContext *ctx, SehleProgram *prog);
void sehle_render_context_set_target (SehleRenderContext *ctx, SehleRenderTarget *tgt);
void sehle_render_context_set_viewport (SehleRenderContext *ctx, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1);

/* Allocate free texture channel and set render state */
int sehle_render_context_set_texture (SehleRenderContext *ctx, SehleTexture *tex);
/* Unconditionally set texture to channel and update render state */
void sehle_render_context_set_texture_to_channel (SehleRenderContext *ctx, SehleTexture *tex, unsigned int channel);
/* Release texture channel */
void sehle_render_context_release_texture (SehleRenderContext* ctx, unsigned int channel);

/* Set up light data for transparent pass lighting */
void sehle_render_context_add_light (SehleRenderContext *ctx, SehleLightInstance *light);

void sehle_render_context_set_reflection_texture (SehleRenderContext *ctx, unsigned int idx, SehleTexture2D* texture, const EleaMat3x4f *refl_w2v, const EleaMat4x4f *refl_proj);
/* Set GBuffer textures and area */
void sehle_render_context_bind_gbuf_textures (SehleRenderContext *ctx, SehleTexture2D* depth, SehleTexture2D* normal, SehleTexture2D* albedo, SehleTexture2D* specular, float x0, float y0, float x1, float y1);

/* Get viewport->gbuffer packed transform */
void sehle_render_context_get_vp2gbuf_transform (SehleRenderContext *ctx, float v[]);
/* Calculate and set W2E and G2E uniforms */
void sehle_render_context_set_gbuffer_uniforms (SehleRenderContext *ctx, SehleProgram *prog, int w2g, int g2e);

/* Render rectangle with 2D vertex and texture coordinates, uniforms have to be set up by caller */
/* fixme: Move render state setup to caller */
void sehle_render_context_draw_overlay_rect_2d (SehleRenderContext *ctx, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1,
	float tex_x0, float tex_y0, float tex_x1, float tex_y1);

#ifdef __cplusplus
};
#endif

#endif

