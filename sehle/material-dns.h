#ifndef __SEHLE_MATERIAL_DNS_H__
#define __SEHLE_MATERIAL_DNS_H__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

#include <elea/color.h>
#include <sehle/material.h>

/* DNS program flags */

#define SEHLE_PROGRAM_DNS_HAS_COLORS 1
#define SEHLE_PROGRAM_DNS_HAS_DIFFUSE 2
#define SEHLE_PROGRAM_DNS_HAS_NORMAL 4
#define SEHLE_PROGRAM_DNS_HAS_SPECULAR 8
#define SEHLE_PROGRAM_DNS_HAS_AMBIENT 16
#define SEHLE_PROGRAM_DNS_HAS_BONES 32

#define SEHLE_PROGRAM_DNS_OUTPUT_GBUFFER 0
#define SEHLE_PROGRAM_DNS_OUTPUT_BLEND 1
#define SEHLE_PROGRAM_DNS_OUTPUT_DENSITY 2

/* DNS program uniforms */

enum {
	SEHLE_PROGRAM_DNS_COLOR_SAMPLER = SEHLE_NUM_UNIFORMS, SEHLE_PROGRAM_DNS_NORMAL_SAMPLER, SEHLE_PROGRAM_DNS_SPECULAR_SAMPLER, SEHLE_PROGRAM_DNS_AMBIENT_SAMPLER,
	SEHLE_PROGRAM_DNS_DIFFUSE, SEHLE_PROGRAM_DNS_SPECULAR, SEHLE_PROGRAM_DNS_SHININESS, SEHLE_PROGRAM_DNS_AMBIENT,
	SEHLE_PROGRAM_DNS_LIGHT_AMBIENT, SEHLE_PROGRAM_DNS_LIGHT_DIFFUSE, SEHLE_PROGRAM_DNS_LIGHT_DIRECT,
	SEHLE_PROGRAM_DNS_LIGHT_POS, SEHLE_PROGRAM_DNS_LIGHT_DIR, SEHLE_PROGRAM_DNS_POINT_ATTENUATION, SEHLE_PROGRAM_DNS_SPOT_ATTENUATION,
	SEHLE_PROGRAM_DNS_AS2S,
	SEHLE_PROGRAM_DNS_NUM_UNIFORMS
};

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_dns_get_reference (SehleEngine *engine, unsigned int flags, unsigned int output);

/* DNS material textures */

enum {
	SEHLE_MATERIAL_DNS_MAP_DIFFUSE,
	SEHLE_MATERIAL_DNS_MAP_NORMAL,
	SEHLE_MATERIAL_DNS_MAP_SPECULAR,
	SEHLE_MATERIAL_DNS_MAP_AMBIENT,
	SEHLE_MATERIAL_DNS_NUM_MAPS
};

typedef struct _SehleMaterialDNS SehleMaterialDNS;
typedef struct _SehleMaterialDNSClass SehleMaterialDNSClass;

#define SEHLE_TYPE_MATERIAL_DNS sehle_material_dns_get_type ()

#define SEHLE_MATERIAL_DNS_FROM_MATERIAL_INSTANCE(i) (SehleMaterialDNS *) ARIKKEI_BASE_ADDRESS (SehleMaterialDNS, material_inst, i)
#define SEHLE_MATERIAL_DNS_MATERIAL_IMPLEMENTATION (&sehle_material_dns_class->material_impl)

struct _SehleMaterialDNS {
	SehleMaterialInstance material_inst;
	EleaColor4f diffuse;
	EleaColor4f specular;
	float shininess;
	float opacity;
	/* Current program flags */
	unsigned int program_flags;
	/* Material uses vertex colors */
	unsigned int has_colors : 1;
	/* Skinning */
	unsigned int skinned : 1;
};

struct _SehleMaterialDNSClass {
	AZClass klass;
	SehleMaterialImplementation material_impl;
};

unsigned int sehle_material_dns_get_type (void);

#ifndef __SEHLE_MATERIAL_DNS_CPP__
extern unsigned int sehle_material_dns_type;
extern SehleMaterialDNSClass *sehle_material_dns_class;
#endif

SehleMaterialDNS *sehle_material_dns_new (SehleEngine *engine);
void sehle_material_dns_delete (SehleMaterialDNS *dns);

/* Marks that given material uses vertex color attribute */
void sehle_material_dns_set_has_colors (SehleMaterialDNS *mdns, unsigned int has_colors);
void sehle_material_dns_set_skinned (SehleMaterialDNS *mdns, unsigned int skinned);
/* Set texture (grab reference) */
void sehle_material_dns_set_texture (SehleMaterialDNS *mdns, unsigned int map_type, SehleTexture2D *tex2d);
/* Set material to use proper rendering pipeline */
void sehle_material_dns_set_transparent (SehleMaterialDNS *mdns, unsigned int transparent, unsigned int sort);

#ifdef __cplusplus
};
#endif

#endif
