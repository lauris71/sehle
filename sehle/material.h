#ifndef __SEHLE_MATERIAL_H__
#define __SEHLE_MATERIAL_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2015
//

typedef struct _SehleMaterialInstance SehleMaterialInstance;
typedef struct _SehleMaterialImplementation SehleMaterialImplementation;
typedef struct _SehleMaterialClass SehleMaterialClass;

#define SEHLE_TYPE_MATERIAL (sehle_material_get_type ())
#define SEHLE_MATERIAL(m) ((SehleMaterialInstance *) (m))

#include <az/interface.h>

#include <sehle/resource.h>

/* Material properties */
#define SEHLE_MATERIAL_INVISIBLE 1
/* Material requires triangles to be sorted */
#define SEHLE_MATERIAL_SORT 2
#define SEHLE_MATERIAL_REFLECTION 4
/* Material requires separate stencil pass (RENDER_STENCIL) */
#define SEHLE_MATERIAL_STENCIL 8

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Standard uniforms
 */

enum {
	SEHLE_UNIFORM_O2V, SEHLE_UNIFORM_PROJECTION, SEHLE_UNIFORM_O2V_PROJECTION,
	SEHLE_UNIFORM_CLIP_PLANE,
	SEHLE_UNIFORM_AS2S,
	SEHLE_NUM_UNIFORMS
};
extern const char *sehle_uniforms[];

struct _SehleMaterialInstance {
	/* OpenGL state flags (fill, cull, depth test etc.) */
	unsigned int state_flags;
	/* General properties (sort, reflection) used by geometries, rendercontext etc. */
	unsigned int properties;
	/* Mask of meaningful render stages for this material */
	unsigned int render_stages;
	/* Mask of meaningful render types for this material */
	unsigned int render_types;
	/* True if program is properly set up */
	unsigned int program_initialized : 1;
	/* Textures */
	unsigned int n_textures;
	SehleTexture *textures[16];
	int channels[16];
	/* Programs */
	unsigned int n_programs;
	SehleProgram *programs[8];
};

struct _SehleMaterialImplementation {
	AZImplementation parent_implementation;
	void (* bind) (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);
};

struct _SehleMaterialClass {
	AZInterfaceClass parent_klass;
};

unsigned int sehle_material_get_type (void);

void sehle_material_set_texture (SehleMaterialInstance *inst, unsigned int idx, SehleTexture *tex);

void sehle_material_bind (SehleMaterialImplementation *impl, SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int render_type);

/* For subclasses and implementations */
void sehle_material_setup (SehleMaterialInstance *inst, unsigned int n_programs, unsigned int n_textures);
void sehle_material_release (SehleMaterialInstance *inst);
/* Bind texture to new channel and set sampler uniform */
void sehle_material_instance_bind_texture (SehleMaterialInstance *inst, SehleRenderContext *ctx, unsigned int prog_idx, unsigned int tex_idx, int sampler_loc);

#ifdef __cplusplus
};
#endif

#endif

