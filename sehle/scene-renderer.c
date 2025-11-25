#define __SEHLE_SCENE_RENDERER_CPP__

/*
* Libsehle
*
* Copyright (C) Lauris Kaplinski 2016
*
*/

#include "GL/glew.h"

#include <arikkei/arikkei-iolib.h>
#include <arikkei/arikkei-utils.h>

#include <az/extend.h>

#include <sehle/light-directional.h>
#include <sehle/render-target.h>
#include <sehle/render-target-deferred.h>
#include <sehle/render-target-texture.h>
#include <sehle/texture-2d.h>
#include <sehle/scene-renderer.h>

/* AZType implementation */
static void scene_renderer_class_init (SehleSceneRendererClass *klass);
/* ArikkeiObject implementation */
static void scene_renderer_shutdown (AZObject *object);

static unsigned int scene_renderer_type = 0;
static SehleSceneRendererClass *scene_renderer_class = NULL;

unsigned int
sehle_scene_renderer_get_type (void)
{
	if (!scene_renderer_type) {
		az_register_type (&scene_renderer_type, (const unsigned char *) "SehleSceneRenderer", AZ_TYPE_OBJECT, sizeof (SehleSceneRendererClass), sizeof (SehleSceneRenderer), 0,
			(void (*) (AZClass *)) scene_renderer_class_init,
			NULL, NULL);
		scene_renderer_class = (SehleSceneRendererClass *) az_type_get_class (scene_renderer_type);
	}
	return scene_renderer_type;
}

static void
scene_renderer_class_init (SehleSceneRendererClass *klass)
{
	((AZObjectClass *) klass)->shutdown = scene_renderer_shutdown;
}

static void
scene_renderer_shutdown (AZObject *object)
{
	SehleSceneRenderer *rend = SEHLE_SCENE_RENDERER (object);
	for (unsigned int i = 0; i < SEHLE_SCENE_RENDERER_MAX_SHADOWS; i++) {
		if (rend->shadows[i]) {
			az_object_unref (AZ_OBJECT (rend->shadows[i]));
			rend->shadows[i] = NULL;
		}
	}
	if (rend->density) {
		az_object_unref (AZ_OBJECT (rend->density));
		rend->density = NULL;
	}
	for (unsigned int i = 0; i < SEHLE_SCENE_RENDERER_MAX_REFLECTIONS; i++) {
		if (rend->mirror[i]) {
			az_object_unref (AZ_OBJECT (rend->mirror[i]));
			rend->mirror[i] = NULL;
		}
	}
	for (unsigned int i = 0; i < SEHLE_SCENE_RENDERER_NUM_TARGETS; i++) {
		if (rend->targets[i]) {
			az_object_shutdown (AZ_OBJECT (rend->targets[i]));
			rend->targets[i] = NULL;
		}
	}
	for (unsigned int i = 0; i < SEHLE_SCENE_RENDERER_NUM_TEXTURES; i++) {
		if (rend->textures[i]) {
			az_object_unref (AZ_OBJECT (rend->textures[i]));
			rend->textures[i] = NULL;
		}
	}
	sehle_render_context_release (&rend->ctx_fb);
	sehle_render_context_release (&rend->ctx_sb);
	rend->engine = NULL;
}

SehleSceneRenderer *
sehle_scene_renderer_new (SehleEngine *engine, unsigned int width, unsigned int height)
{
	arikkei_return_val_if_fail (engine != NULL, NULL);
	SehleSceneRenderer *rend = (SehleSceneRenderer *) az_object_new (SEHLE_TYPE_SCENE_RENDERER);
	rend->engine = engine;
	rend->viewport_width = width;
	rend->viewport_height = height;

	sehle_render_context_setup (&rend->ctx_fb, engine);
	sehle_render_context_setup (&rend->ctx_sb, engine);
	/* GBuffer */
	SEHLE_CHECK_ERRORS (0);
	rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER] = (SehleRenderTarget *) sehle_render_target_deferred_new (engine, width, height);
	SEHLE_CHECK_ERRORS (0);
	rend->textures[SEHLE_SCENE_RENDERER_GBUF_ALBEDO_AMBIENT] = ((SehleRenderTargetDeferred *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER])->tex[SEHLE_RENDER_TARGET_TEXTURE_ALBEDO_AMBIENT];
	az_object_ref (AZ_OBJECT (rend->textures[SEHLE_SCENE_RENDERER_GBUF_ALBEDO_AMBIENT]));
	rend->textures[SEHLE_SCENE_RENDERER_GBUF_NORMAL] = ((SehleRenderTargetDeferred *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER])->tex[SEHLE_RENDER_TARGET_TEXTURE_NORMAL];
	az_object_ref (AZ_OBJECT (rend->textures[SEHLE_SCENE_RENDERER_GBUF_NORMAL]));
	rend->textures[SEHLE_SCENE_RENDERER_GBUF_SPECULAR_SHININESS] = ((SehleRenderTargetDeferred *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER])->tex[SEHLE_RENDER_TARGET_TEXTURE_SPECULAR_SHININESS];
	az_object_ref (AZ_OBJECT (rend->textures[SEHLE_SCENE_RENDERER_GBUF_SPECULAR_SHININESS]));
	rend->textures[SEHLE_SCENE_RENDERER_GBUF_DEPTH] = ((SehleRenderTargetDeferred *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER])->tex[SEHLE_RENDER_TARGET_TEXTURE_DEPTH];
	az_object_ref (AZ_OBJECT (rend->textures[SEHLE_SCENE_RENDERER_GBUF_DEPTH]));
	//rend->textures[SEHLE_SCENE_RENDERER_GBUF_AMBIENT] = ((SehleRenderTargetDeferred *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER])->tex[SehleRenderTargetDeferred::AMBIENT];
	//az_object_ref (AZ_OBJECT (rend->textures[SEHLE_SCENE_RENDERER_GBUF_AMBIENT]));

	/* Framebuffer */
	SEHLE_CHECK_ERRORS (0);
	rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER] = (SehleRenderTarget *) sehle_render_target_texture_new (engine, width, height, SEHLE_TEXTURE_RGBA, SEHLE_TEXTURE_F16, 1, 1);
	SEHLE_CHECK_ERRORS (0);
	/* Share single depth texture between framebuffer and gbuffer */
	sehle_render_target_texture_set_depth ((SehleRenderTargetTexture *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER], 1, rend->textures[SEHLE_SCENE_RENDERER_GBUF_DEPTH]);
	SEHLE_CHECK_ERRORS (0);
	rend->textures[SEHLE_SCENE_RENDERER_FBUF_COLOR] = sehle_render_target_texture_get_color ((SehleRenderTargetTexture *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER]);
	SEHLE_CHECK_ERRORS (0);
	rend->textures[SEHLE_SCENE_RENDERER_FBUF_DEPTH] = sehle_render_target_texture_get_depth ((SehleRenderTargetTexture *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER]);
	SEHLE_CHECK_ERRORS (0);

	/* Shadow and density */
	rend->targets[SEHLE_SCENE_RENDERER_TARGET_SHADOW] = (SehleRenderTarget *) sehle_render_target_texture_new (engine, 2048, 2048, SEHLE_TEXTURE_RGBA, SEHLE_TEXTURE_U8, 1, 1);
	SEHLE_CHECK_ERRORS (0);
	for (int i = 0; i < SEHLE_SCENE_RENDERER_MAX_SHADOWS - 1; i++) {
		rend->shadows[i] = sehle_render_target_texture_replace_depth ((SehleRenderTargetTexture *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_SHADOW], NULL);
		SEHLE_CHECK_ERRORS (0);
	}
	rend->shadows[SEHLE_SCENE_RENDERER_MAX_SHADOWS - 1] = sehle_render_target_texture_get_depth ((SehleRenderTargetTexture *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_SHADOW]);
	SEHLE_CHECK_ERRORS (0);
	rend->density = sehle_render_target_texture_get_color ((SehleRenderTargetTexture *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_SHADOW]);
	SEHLE_CHECK_ERRORS (0);

	return rend;
}

SehleSceneRenderer *
sehle_scene_renderer_new_from_textures (SehleEngine *engine, unsigned int width, unsigned int height, SehleTexture2D *textures[], SehleTexture2D *shadows[], SehleTexture2D *density)
{
	SehleSceneRenderer *rend = sehle_scene_renderer_new (engine, width, height);
	return rend;
}

void
sehle_scene_renderer_resize (SehleSceneRenderer *rend, unsigned int width, unsigned int height)
{
	sehle_scene_renderer_resize_viewport (rend, width, height);
	sehle_scene_renderer_resize_targets (rend, width, height);
}

void
sehle_scene_renderer_resize_viewport (SehleSceneRenderer *rend, unsigned int width, unsigned int height)
{
	if ((width != rend->viewport_width) || (height != rend->viewport_height)) {
		rend->viewport_width = width;
		rend->viewport_height = height;
	}
}

void
sehle_scene_renderer_resize_targets (SehleSceneRenderer *rend, unsigned int width, unsigned int height)
{
	if ((width != rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER]->width) || (height != rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER]->height)) {
		sehle_render_target_resize (rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER], width, height);
	}
	if ((width != rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER]->width) || (height != rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER]->height)) {
		sehle_render_target_resize (rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER], width, height);
	}
}

void
sehle_scene_renderer_resize_viewport_and_grow_targets (SehleSceneRenderer *rend, unsigned int width, unsigned int height)
{
	sehle_scene_renderer_resize_viewport (rend, width, height);
	unsigned int twidth = width;
	unsigned int theight = height;
	if (twidth < rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER]->width) twidth = rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER]->width;
	if (theight < rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER]->height) theight = rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER]->height;
	sehle_render_target_resize (rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER], twidth, theight);
	if (twidth < rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER]->width) twidth = rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER]->width;
	if (theight < rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER]->height) theight = rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER]->height;
	sehle_render_target_resize (rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER], twidth, theight);
}

void
sehle_scene_renderer_render_colors (SehleSceneRenderer *rend, const float area[], const EleaMat3x4f *v2w, const EleaMat4x4f *proj, const EleaPlane3f *clip_plane, SehleRenderableImplementation *impl, SehleRenderableInstance *inst)
{
	double start_time;
	rend->root.impl = impl;
	rend->root.inst = inst;

	unsigned int gi[4];
	gi[0] = (unsigned int) (area[0] * rend->viewport_width);
	gi[1] = (unsigned int) (area[1] * rend->viewport_height);
	gi[2] = (unsigned int) (area[2] * rend->viewport_width);
	gi[3] = (unsigned int) (area[3] * rend->viewport_height);

	start_time = arikkei_get_time ();
	sehle_render_context_reset (&rend->ctx_fb);
	sehle_render_context_set_view (&rend->ctx_fb, v2w, proj);
	if (clip_plane) rend->ctx_fb.clip_plane = *clip_plane;
	sehle_render_context_set_target (&rend->ctx_fb, rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER]);
	sehle_render_context_set_viewport (&rend->ctx_fb, gi[0], gi[1], gi[2], gi[3]);
	/* Clear everything */
	sehle_render_context_clear (&rend->ctx_fb, 1, 1, &EleaColor4fTransparent);
	/* Opaque layer objects to GBuffer */
	sehle_render_context_display_frame (&rend->ctx_fb, impl, inst, SEHLE_SCENE_RENDERER_OPAQUE, SEHLE_STAGE_SOLID);
	sehle_render_context_render_gbuffer (&rend->ctx_fb);
	sehle_render_context_finish_frame (&rend->ctx_fb);
	if (rend->sync) glFinish ();
	rend->time_opaque = (float) (arikkei_get_time () - start_time);
	/* Ambient occlusion */
	float gf[4];
	gf[0] = (float) gi[0] / rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER]->width;
	gf[1] = (float) gi[1] / rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER]->height;
	gf[2] = (float) gi[2] / rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER]->width;
	gf[3] = (float) gi[3] / rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER]->height;
	// fixme: This a bit dangerous because we bind all textures and render to albedo (although do not read from it)
	start_time = arikkei_get_time ();
	sehle_render_context_bind_gbuf_textures (&rend->ctx_fb, rend->textures[SEHLE_SCENE_RENDERER_GBUF_DEPTH],
		NULL, NULL, NULL, gf[0], gf[1], gf[2], gf[3]);
	sehle_render_context_display_frame (&rend->ctx_fb, impl, inst, SEHLE_SCENE_RENDERER_OCCLUSION, SEHLE_STAGE_AMBIENT);
	sehle_render_context_render_occlusion (&rend->ctx_fb);
	sehle_render_context_finish_frame (&rend->ctx_fb);
	if (rend->sync) glFinish ();
	rend->time_occlusion = (float) (arikkei_get_time () - start_time);

	start_time = arikkei_get_time ();
	/* HDR framebuffer */
	sehle_render_context_set_target (&rend->ctx_fb, rend->targets[SEHLE_SCENE_RENDERER_TARGET_FRAMEBUFFER]);
	sehle_render_context_set_viewport (&rend->ctx_fb, gi[0], gi[1], gi[2], gi[3]);
	/* Clear colors, keep depth */
	sehle_render_context_clear (&rend->ctx_fb, 0, 1, &rend->fb_bg);

	//SehleRenderTargetDeferred *gtarget = (SehleRenderTargetDeferred *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_GBUFFER];
	sehle_render_context_bind_gbuf_textures (&rend->ctx_fb, rend->textures[SEHLE_SCENE_RENDERER_GBUF_DEPTH],
		rend->textures[SEHLE_SCENE_RENDERER_GBUF_NORMAL],
		rend->textures[SEHLE_SCENE_RENDERER_GBUF_ALBEDO_AMBIENT],
		rend->textures[SEHLE_SCENE_RENDERER_GBUF_SPECULAR_SHININESS], gf[0], gf[1], gf[2], gf[3]);

	// Lightmaps
	// The problem is, that UINT8 buffers do not support subtractive blend
	// Thus we better render lightmap here so we do not have to create FP16 SSAO buffers
	sehle_render_context_display_frame (&rend->ctx_fb, impl, inst, SEHLE_SCENE_RENDERER_LIGHTS, SEHLE_STAGE_LIGHTS);
	// Lighting for opaque geometry
	sehle_render_context_render_lights (&rend->ctx_fb);
	sehle_render_context_finish_frame (&rend->ctx_fb);
	if (rend->sync) glFinish ();
	rend->time_lighting = (float) (arikkei_get_time () - start_time);

	// fixme: Sort out sky
	start_time = arikkei_get_time ();
	sehle_render_context_display_frame (&rend->ctx_fb, impl, inst, SEHLE_SCENE_RENDERER_SKY, SEHLE_STAGE_TRANSPARENT);
	sehle_render_context_render_transparent (&rend->ctx_fb);
	sehle_render_context_finish_frame (&rend->ctx_fb);
	if (rend->sync) glFinish ();
	rend->time_sky = (float) (arikkei_get_time () - start_time);

	start_time = arikkei_get_time ();
	sehle_render_context_display_frame (&rend->ctx_fb, impl, inst, SEHLE_SCENE_RENDERER_TRANSPARENT, SEHLE_STAGE_TRANSPARENT);
	sehle_render_context_render_transparent (&rend->ctx_fb);
	sehle_render_context_finish_frame (&rend->ctx_fb);
	if (rend->sync) glFinish ();
	rend->time_transparent = (float) (arikkei_get_time () - start_time);

	sehle_render_context_end (&rend->ctx_fb);

	rend->root.impl = NULL;
	rend->root.inst = NULL;
}

void
sehle_scene_renderer_render_directional_shadow (SehleSceneRenderer *rend, SehleDirectionalLightInstance *dirl, SehleRenderContext *ctx)
{
	// Calculate projection matrices
	// Camera hexahedron in view space
	EleaVec3f corners[8];
	elea_mat4x4f_get_corners_of_rev_projection (corners, &ctx->rproj);
	//ctx->rprojection.getCornersofReverseProjection (corners, Elea::Matrix4x4fIdentity);
	// fixme: Split
	// Transform to light space
	// First orient light to view axis
	EleaVec3f p1;
	elea_mat3x4f_get_translation (&p1, &ctx->v2w);
	EleaVec3f p0;
	elea_mat3x4f_get_col_vec (&p0, &dirl->light_inst.l2w, 2);
	p0 = elea_vec3f_add (p1, p0);
	EleaVec3f up;
	elea_mat3x4f_get_col_vec (&up, &ctx->v2w, 2);
	EleaMat3x4f l2w;
	elea_mat3x4f_set_look_at (&l2w, &p0, &p1, &up);
	// EleaMat3x4f l2w = light->l2w;
	EleaMat3x4f w2l;
	elea_mat3x4f_invert_normalized (&w2l, &l2w);
	EleaMat3x4f v2l;
	elea_mat3x4f_multiply (&v2l, &w2l, &ctx->v2w);

	// Determine relative splits
	float near = -corners[0].z;
	float far = -corners[4].z;
	float beta = 0.002f;
	float rsplits[5];
	for (int i = 0; i < 5; i++) {
		float factor = i / 4.0f;
		float s0 = factor;
		float s1 = near * (expf (factor * (logf (far) - logf (near))) - 1) / (far - near);
		rsplits[i] = beta * s0 + (1 - beta) * s1;
		//rsplits[i] = beta * factor + (1 - beta) * near * (expf (factor * (logf (far) - logf (near))) - 1) / (far - near);
	}
	EleaMat4x4f shadow_proj[4];
	/* Split Z in viewspace coordinates */
	float split_z[3];
	for (int i = 0; i < 4; i++) {
		// Calculate subfrustum corners
		EleaVec3f c[8];
		for (int j = 0; j < 4; j++) {
			EleaVec3f d, s;
			d = elea_vec3f_sub (corners[4 + j], corners[j]);
			s = elea_vec3f_mul (d, rsplits[i]);
			c[j] = elea_vec3f_add (corners[j], s);
			s = elea_vec3f_mul (d, rsplits[i + 1]);
			c[4 + j] = elea_vec3f_add (corners[j], s);
		}
		// Absolute (negative) split position
		if (i < 3) split_z[i] = c[4].z;
		// Subfrustum bounding box in light space
		EleaAABox3f bbox = EleaAABox3fEmpty;
		for (unsigned int j = 0; j < 8; j++) {
			elea_aabox3f_grow_p_mat (&bbox, &bbox, &c[j], &v2l);
		}
		// fixme: Should determine light distance from clipped scene bbox
		float lightdistance = 1000;
		elea_mat4x4f_set_ortho (&shadow_proj[i], bbox.c[0], bbox.c[3], bbox.c[1], bbox.c[4], -bbox.c[5] - lightdistance, -bbox.c[2]);
	}

	int shadowidx = 0;
	// fixme: Can we simply clean shadow texture without replacing? (Lauris)
	az_object_ref (AZ_OBJECT (rend->shadows[shadowidx]));
	SehleTexture2D *shadow = sehle_render_target_texture_replace_depth ((SehleRenderTargetTexture *) rend->targets[SEHLE_SCENE_RENDERER_TARGET_SHADOW], rend->shadows[shadowidx]);
	az_object_unref (AZ_OBJECT (shadow));
	// fixme: Update l2w in Miletos::Light
	sehle_render_context_reset (&rend->ctx_sb);
	sehle_render_context_set_view (&rend->ctx_sb, &l2w, &shadow_proj[0]);
	sehle_render_context_set_target (&rend->ctx_sb, rend->targets[SEHLE_SCENE_RENDERER_TARGET_SHADOW]);
	sehle_render_context_set_viewport (&rend->ctx_sb, 0, 0, rend->targets[SEHLE_SCENE_RENDERER_TARGET_SHADOW]->width, rend->targets[SEHLE_SCENE_RENDERER_TARGET_SHADOW]->height);
#if 1
	/* fixme: The light shadow mask belong to AosoraLight once we have nested interfaces (Lauris) */
	sehle_render_context_clear (&rend->ctx_sb, 1, 1, &EleaColor4fWhite);
	rend->ctx_sb.lodBias = 1.0f;
	for (int i = 0; i < 4; i++) {
		int vpx = (i % 2) * rend->shadows[shadowidx]->width / 2;
		int vpy = (i / 2) * rend->shadows[shadowidx]->height / 2;
		sehle_render_context_set_view (&rend->ctx_sb, &l2w, &shadow_proj[i]);
		sehle_render_context_set_viewport (&rend->ctx_sb, vpx, vpy, vpx + rend->shadows[shadowidx]->width / 2, vpy + rend->shadows[shadowidx]->height / 2);
		if (i < 2) {
			sehle_render_context_display_frame (&rend->ctx_sb, rend->root.impl, rend->root.inst, SEHLE_SCENE_RENDERER_SHADOW | SEHLE_SCENE_RENDERER_DENSITY, SEHLE_STAGE_SOLID | SEHLE_STAGE_TRANSPARENT);
			sehle_render_context_render_shadow (&rend->ctx_sb);
			sehle_render_context_render_density (&rend->ctx_sb);
		} else {
			sehle_render_context_display_frame (&rend->ctx_sb, rend->root.impl, rend->root.inst, SEHLE_SCENE_RENDERER_SHADOW, SEHLE_STAGE_SOLID);
			sehle_render_context_render_shadow (&rend->ctx_sb);
			//sehle_render_context_render_density (&rend->ctx_sb);
		}
		sehle_render_context_finish_frame (&rend->ctx_sb);
	}
#endif
	sehle_directional_light_set_shadow_matrices (dirl, &ctx->v2w, &l2w, shadow_proj, split_z);
	/* fixme: Sort this out (Lauris) */
	sehle_light_instance_set_shadow_texture (&dirl->light_inst, &rend->shadows[shadowidx]->texture, &rend->density->texture);
	sehle_render_context_end (&rend->ctx_sb);
}
