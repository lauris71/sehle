#ifndef __SEHLE_MATERIAL_OVERLAY_H__
#define __SEHLE_MATERIAL_OVERLAY_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2012
//

#include <sehle/material.h>

/*
 * If colors are present, color channel determines primary color
 *
 * If mask is enabled, each texture channel (RGBA) is interpreted
 * as the weigth of respective solid color (primary, secondary[3]
 * Otherwise primary is multiplied with texture color
 */

#define SEHLE_PROGRAM_OVERLAY_HAS_TEXTURE 1
#define SEHLE_PROGRAM_OVERLAY_HAS_COLORS 2
#define SEHLE_PROGRAM_OVERLAY_HAS_MASK 4
#define SEHLE_PROGRAM_OVERLAY_HAS_DEPTH 8
#define SEHLE_PROGRAM_OVERLAY_HAS_EXPOSURE 32

/* Overlay program uniforms */

enum {
	SEHLE_PROGRAM_OVERLAY_VERTEX_TRANSFORM, SEHLE_PROGRAM_OVERLAY_TEX_TRANSFORM,
	SEHLE_PROGRAM_OVERLAY_DEPTH, SEHLE_PROGRAM_OVERLAY_PRIMARY, SEHLE_PROGRAM_OVERLAY_SECONDARY,
	SEHLE_PROGRAM_OVERLAY_TEXTURE,
	SEHLE_PROGRAM_OVERLAY_LWMAX, SEHLE_PROGRAM_OVERLAY_GAMMA,
	SEHLE_PROGRAM_OVERLAY_NUM_UNIFORMS
};

/* Overlay program attributes */

enum {
	SEHLE_PROGRAM_OVERLAY_VERTEX,
	SEHLE_PROGRAM_OVERLAY_TEXCOORD,
	SEHLE_PROGRAM_OVERLAY_COLOR,
	SEHLE_PROGRAM_OVERLAY_NUM_ATTRIBUTES
};

#ifdef __cplusplus
extern "C" {
#endif

SehleProgram *sehle_program_overlay_get_reference (SehleEngine *engine, unsigned int flags, float min_alpha);

#ifdef __cplusplus
};
#endif

#endif
