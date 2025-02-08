#ifndef __SEHLE_LIGHT_H__
#define __SEHLE_LIGHT_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2016
//

/*
 * Implementations can implement render_shadow to get shadowing support
 */

typedef struct _SehleLightInstance SehleLightInstance;
typedef struct _SehleLightImplementation SehleLightImplementation;
typedef struct _SehleLightClass SehleLightClass;
typedef struct _SehleLightInfo SehleLightInfo;

#define SEHLE_TYPE_LIGHT sehle_light_get_type ()

#define SEHLE_LIGHT_FROM_RENDERABLE_INSTANCE(i) (SehleLightInstance *) ARIKKEI_BASE_ADDRESS (SehleLightInstance, renderable_inst, i)
#define SEHLE_LIGHT_FROM_RENDERABLE_IMPLEMENTATION(i) (SehleLightImplementation *) ARIKKEI_BASE_ADDRESS (SehleLightImplementation, renderable_impl, i)
#define SEHLE_LIGHT_FROM_MATERIAL_INSTANCE(i) (SehleLightInstance *) ARIKKEI_BASE_ADDRESS (SehleLightInstance, material_inst, i)
#define SEHLE_LIGHT_FROM_MATERIAL_IMPLEMENTATION(i) (SehleLightImplementation *) ARIKKEI_BASE_ADDRESS (SehleLightImplementation, material_impl, i)

#include <elea/color.h>
#include <sehle/material.h>
#include <sehle/renderable.h>

/* Light program flags */

#define SEHLE_PROGRAM_LIGHT_HAS_SHADOW 1
#define SEHLE_PROGRAM_LIGHT_HAS_DENSITY 2
#define SEHLE_PROGRAM_LIGHT_DIRECTIONAL 4
#define SEHLE_PROGRAM_LIGHT_POINT 8
#define SEHLE_PROGRAM_LIGHT_SPOT 16

/* Light program uniforms */

enum SehleLightUniforms {
	SEHLE_LIGHT_L2W_W2V_PROJECTION,
	// From GBuffer to (denormalized) view
	SEHLE_LIGHT_G2D_RPROJECTION,
	// From window to gbuffer packed
	SEHLE_LIGHT_W2G,
	SEHLE_LIGHT_DEPTH_SAMPLER, SEHLE_LIGHT_NORMAL_SAMPLER, SEHLE_LIGHT_ALBEDO_SAMPLER, SEHLE_LIGHT_SPECULAR_SAMPLER,
	SEHLE_LIGHT_LIGHTPOS, SEHLE_LIGHT_LIGHTDIR,
	SEHLE_LIGHT_AMBIENT, SEHLE_LIGHT_DIFFUSE, SEHLE_LIGHT_DIRECT,
	/* Min distance, Max distance, delta, power*/
	SEHLE_LIGHT_POINT_ATTENUATION,
	/* Max cos, delta, power */
	SEHLE_LIGHT_SPOT_ATTENUATION,
	SEHLE_LIGHT_SPLITS, E2S, SEHLE_LIGHT_SHADOW_SAMPLER, SEHLE_LIGHT_DENSITY_SAMPLER,
	SEHLE_LIGHT_NUM_UNIFORMS
};

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_light_get_reference (SehleEngine *engine, unsigned int flags, unsigned int num_splits);

struct _SehleLightInfo {
	EleaVec4f pos;
	EleaVec3f dir;
	/* Min, max, power */
	float point_attn[3];
	float spot_attn[3];
};

struct _SehleLightInstance {
	SehleRenderableInstance renderable_inst;
	SehleMaterialInstance material_inst;

	/* Current program configuration */
	unsigned int program_flags;
	unsigned int num_splits;

	/* Whether to invoke render_shadow from implementation render method */
	/* fixme: Should light shedule separate shadow pass? */
	unsigned int render_shadow : 1;
	/* Implementation-dependent priority if only some lights should be rendered */
	float priority;

	/* Common parameters for material implementation */
	/* Position and direction */
	EleaMat3x4f l2w;
	/* Colors (only RGB is used) */
	EleaColor4f ambient;
	EleaColor4f diffuse;
	EleaColor4f direct;
	/* Point attenuation
	   min_distance, outer_radius, delta, power
	   Idenorm = ((outer - distance) / (outer - inner)) ** power */
	float point_attenuation[4];
	/* Spot attenuation */
	/* outer_cos, cos_delta, power */
	/* Idenorm = ((cos_angle - outer_cos) / (inner_cos - outer_cos)) ** power */
	float spot_attenuation[3];

	SehleLightInfo info;

	/* Geometry */
	SehleVertexArray* va;

	/* Shadow textures */
	SehleTexture *shadow;
	SehleTexture *density;
};

struct _SehleLightImplementation {
	SehleRenderableImplementation renderable_impl;
	SehleMaterialImplementation material_impl;
	/* Set up Lightinfo for forward passes */
	void (* setup_forward) (SehleLightImplementation *impl, SehleLightInstance *inst, SehleRenderContext *ctx);
	/* Implementation should set relevant textures in light instance */
	void (*render_shadow) (SehleLightImplementation *impl, SehleLightInstance *inst, SehleRenderContext *ctx);
};

struct _SehleLightClass {
	SehleRenderableClass renderable_klass;
};

unsigned int sehle_light_get_type (void);

/* Frees programs and buffers */
void sehle_light_release (SehleLightInstance *inst);

/* Set shadow textures and resets program if needed */
void sehle_light_instance_set_shadow_texture (SehleLightInstance *inst, SehleTexture *shadow, SehleTexture *density);

/* For implementations */
void sehle_light_bind_common (SehleLightInstance *inst, SehleRenderContext *ctx);

#ifdef __cplusplus
};
#endif

#endif

