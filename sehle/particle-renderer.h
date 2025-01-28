#ifndef __SEHLE_PARTICLE_RENDERER_H__
#define __SEHLE_PARTICLE_RENDERER_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2012-2016
//

typedef struct _SehleParticleRenderer SehleParticleRenderer;
typedef struct _SehleParticleRendererClass SehleParticleRendererClass;

#define SEHLE_TYPE_PARTICLE_RENDERER sehle_particle_renderer_get_type ()

#define SEHLE_PARTICLE_RENDERER_FROM_MATERIAL_INSTANCE(i) (SehleParticleRenderer *) AZ_BASE_ADDRESS (SehleParticleRenderer, material_instance, i)
#define SEHLE_PARTICLE_RENDERER_MATERIAL_IMPLEMENTATION (&sehle_particle_renderer_class->material_implementation)

#define SEHLE_PARTICLE_RENDERER_MAP_TEXTURE 0
#define SEHLE_PARTICLE_RENDERER_GRADIENT_TEXTURE 1

#include <elea/color.h>
#include <sehle/material.h>

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_particle_get_reference (SehleEngine *engine);

struct _SehleParticleRenderer {
	SehleMaterialInstance material_instance;
	EleaColor4f colors[4];
	float zbias;
};

struct _SehleParticleRendererClass {
	AZClass klass;
	SehleMaterialImplementation material_implementation;
};

#ifndef __SEHLE_PARTICLE_RENDERER_CPP__
extern unsigned int sehle_particle_renderer_type;
extern SehleParticleRendererClass *sehle_particle_renderer_class;
#endif

unsigned int sehle_particle_renderer_get_type (void);

void sehle_particle_renderer_setup (SehleParticleRenderer *rend, SehleEngine *engine);
void sehle_particle_renderer_release (SehleParticleRenderer *rend);

/* Grabs reference */
void sehle_particle_renderer_set_texture (SehleParticleRenderer *rend, unsigned int tex_idx, SehleTexture2D *tex);

#ifdef __cplusplus
};
#endif

#endif
