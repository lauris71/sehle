#ifndef __SEHLE_SCENE_RENDERER_H__
#define __SEHLE_SCENE_RENDERER_H__

/*
* Libsehle
*
* Copyright (C) Lauris Kaplinski 2016
*
*/

#define SEHLE_TYPE_SCENE_RENDERER (sehle_scene_renderer_get_type ())
#define SEHLE_SCENE_RENDERER(o) (AZ_CHECK_INSTANCE_CAST ((o), SEHLE_TYPE_SCENE_RENDERER, SehleSceneRenderer))
#define SEHLE_IS_SCENE_RENDERER(o) (AZ_CHECK_INSTANCE_TYPE ((o), SEHLE_TYPE_SCENE_RENDERER))

typedef struct _SehleSceneRenderer SehleSceneRenderer;
typedef struct _SehleSceneRendererClass SehleSceneRendererClass;

#include <az/object.h>
#include <sehle/renderable.h>
#include <sehle/render-context.h>

/* Renderable layers */
/* Rendered as opaque geometry to gbuffer */
#define SEHLE_SCENE_RENDERER_OPAQUE 1
/* Rendered as opaque geometry to depth buffer */
#define SEHLE_SCENE_RENDERER_SHADOW 2
/* Rendered as transparent geometry to color buffer */
#define SEHLE_SCENE_RENDERER_TRANSPARENT 4
/* Rendered as transparent geometry to density buffer */
#define SEHLE_SCENE_RENDERER_DENSITY 8
/* Rendered as occlusion to gbuffer */
#define SEHLE_SCENE_RENDERER_OCCLUSION 16
/* Rendered in lighting stage */
#define SEHLE_SCENE_RENDERER_LIGHTS (1 << 14)
/* Rendered in sky stage (after opaque and lighting passes, before transparent pass) */
#define SEHLE_SCENE_RENDERER_SKY (1 << 15)

#define SEHLE_SCENE_RENDERER_ALL_LAYERS 0xffffffff

#define SEHLE_SCENE_RENDERER_MAX_SHADOWS SEHLE_RENDER_CONTEXT_MAX_LIGHTS
#define SEHLE_SCENE_RENDERER_MAX_REFLECTIONS 2

/* Render targets */
enum {
	SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER,
	SEHLE_SCENE_RENDERER_TARGET_SHADOW,
	SEHLE_SCENE_RENDERER_TARGET_GBUFFER,
	SEHLE_SCENE_RENDERER_NUM_TARGETS
};

/* Textures */
enum {
	SEHLE_SCENE_RENDERER_GBUF_ALBEDO_AMBIENT,
	SEHLE_SCENE_RENDERER_GBUF_NORMAL,
	SEHLE_SCENE_RENDERER_GBUF_SPECULAR_SHININESS,
	SEHLE_SCENE_RENDERER_GBUF_DEPTH,
	SEHLE_SCENE_RENDERER_FBUF_COLOR,
	SEHLE_SCENE_RENDERER_FBUF_DEPTH,
	SEHLE_SCENE_RENDERER_NUM_TEXTURES
};

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleSceneRenderer {
	AZObject object;

	SehleEngine *engine;
	unsigned int viewport_width;
	unsigned int viewport_height;

	EleaColor4f fb_bg;

	/* Main framebuffer context */
	SehleRenderContext ctx_fb;
	/* Shadow context */
	SehleRenderContext ctx_sb;

	/* Root renderable for recursive shadow and reflection rendering */
	SehleRenderableHandle root;

	SehleRenderTarget *targets[SEHLE_SCENE_RENDERER_NUM_TARGETS];

	SehleTexture2D *shadows[SEHLE_SCENE_RENDERER_MAX_SHADOWS];
	SehleTexture2D *density;
	SehleTexture2D *mirror[SEHLE_SCENE_RENDERER_MAX_REFLECTIONS];

	/* Direct access to component textures */
	SehleTexture2D *textures[SEHLE_SCENE_RENDERER_NUM_TEXTURES];

	/* Profiler */
	unsigned int sync;
	float time_opaque;
	float time_occlusion;
	float time_lighting;
	float time_sky;
	float time_transparent;
};

struct _SehleSceneRendererClass {
	AZObjectClass object_class;
};

unsigned int sehle_scene_renderer_get_type (void);

/* If targets or textures are present they are recycled, otherwise new ones are created */
SehleSceneRenderer *sehle_scene_renderer_new (SehleEngine *engine, unsigned int width, unsigned int height);
SehleSceneRenderer *sehle_scene_renderer_new_from_textures (SehleEngine *engine, unsigned int width, unsigned int height,
	SehleTexture2D *textures[], SehleTexture2D *shadows[], SehleTexture2D *density);

/* Resize both viewport and targets */
void sehle_scene_renderer_resize (SehleSceneRenderer *rend, unsigned int width, unsigned int height);
void sehle_scene_renderer_resize_viewport (SehleSceneRenderer *rend, unsigned int width, unsigned int height);
void sehle_scene_renderer_resize_targets (SehleSceneRenderer *rend, unsigned int width, unsigned int height);
void sehle_scene_renderer_resize_viewport_and_grow_targets (SehleSceneRenderer *rend, unsigned int width, unsigned int height);

void sehle_scene_renderer_render_colors (SehleSceneRenderer *rend, const float area[], const EleaMat3x4f *v2w, const EleaMat4x4f *proj, const EleaPlane3f *clip_plane, SehleRenderableImplementation *impl, SehleRenderableInstance *inst);

/* For light implementations */
void sehle_scene_renderer_render_directional_shadow (SehleSceneRenderer *rend, SehleDirectionalLightInstance *dirl, SehleRenderContext *ctx);

#ifdef __cplusplus
};
#endif

#endif

