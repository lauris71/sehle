#ifndef __SEHLE_ENGINE_H__
#define __SEHLE_ENGINE_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2015
//

#define SEHLE_PERFORMANCE_MONITOR 1

typedef struct _SehleEngine SehleEngine;
typedef struct _SehleEngineClass SehleEngineClass;
#ifdef SEHLE_PERFORMANCE_MONITOR
typedef struct _SehlePerformanceCounter SehlePerformanceCounter;
#endif

#define SEHLE_TYPE_ENGINE sehle_engine_get_type ()
#define SEHLE_ENGINE(v) ((SehleEngine *) (v))
#define SEHLE_IS_ENGINE(v) ((v) != NULL)

#include <sehle/sehle.h>
#include <sehle/render-state.h>

#define SEHLE_GEOMETRY_TEXTURE_RECT 0
#define SEHLE_GEOMETRY_UNIT_CUBE_OUTSIDE 1
#define SEHLE_GEOMETRY_UNIT_CUBE_INSIDE 2
#define SEHLE_GEOMETRY_STREAM_256 3
#define SEHLE_GEOMETRY_GRID_8x8 4
#define SEHLE_NUM_BASIC_GEOMETRIES 5

#define SEHLE_TEXTURE_NONE 0
#define SEHLE_TEXTURE_BLACK 1
#define SEHLE_TEXTURE_WHITE 2
#define SEHLE_TEXTURE_BLUE 3
#define SEHLE_TEXTURE_TRANSPARENT 4
#define SEHLE_TEXTURE_NOISE_16X16 5
#define SEHLE_NUM_BASIC_TEXTURES 6

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SEHLE_PERFORMANCE_MONITOR
struct _SehlePerformanceCounter {
	unsigned int indices;
	unsigned int indexinstances;
	unsigned int sortedtriangles;
	unsigned int geometry_batches;
	unsigned int texture_binds;
	unsigned int program_switches;
	unsigned int rendertarget_binds;
};
#endif

struct _SehleEngine {
	SehleTexture2D *textures[SEHLE_NUM_BASIC_TEXTURES];
	SehleVertexArray *varrays[SEHLE_NUM_BASIC_GEOMETRIES];

	SehleResourceManager *resources;

	// Whether OpenGL context is available
	unsigned int running;

	/* Current render state */
	SehleRenderState render_state;

	/* Current time for all interested materials */
	float time;

#ifdef SEHLE_PERFORMANCE_MONITOR
	SehlePerformanceCounter counter;
#endif
};

unsigned int sehle_engine_get_type (void);

SehleEngine *sehle_engine_new (void);
void sehle_engine_delete (SehleEngine *engine);

/* Signal that all resources can safely build OpenGL objects */
void sehle_engine_ensure_started (SehleEngine *engine);

/*
 * Initialize OpenGL to known state
 * Culling enabled, front face CCW
 * Filled polygons
 * Depth test LESS
 * Depth write ON
 * Color write ALL
 * Blend OFF
 * Blend function SRC_ALPHA, ONE_MINUS_SRC_ALPHA
 */
void sehle_engine_initialize_render_state (SehleEngine *engine);
/* Set render state to specific value */
void sehle_engine_set_render_state (SehleEngine *engine, const SehleRenderState *new_state);
void sehle_engine_set_render_flags (SehleEngine *engine, unsigned int flags);
/* Set render target and invoke bind/release as needed */
/* Engine holds reference of current target */
void sehle_engine_set_render_target (SehleEngine *engine, SehleRenderTarget *tgt);
/* Set program and invoke bind/release as needed */
/* Engine holds reference of current program */
void sehle_engine_set_program (SehleEngine *engine, SehleProgram *prog);
void sehle_engine_set_texture (SehleEngine *engine, unsigned int idx, SehleTexture *tex);
void sehle_engine_set_viewport (SehleEngine *engine, const NRRectl *viewport);

/* Get standard resource */
SehleTexture2D *sehle_engine_get_standard_texture (SehleEngine *engine, unsigned int type);
SehleVertexArray *sehle_engine_get_standard_geometry (SehleEngine *engine, unsigned int type);

/* Get a reference to resource (new instance will be created if not already known) */
SehleVertexBuffer *sehle_engine_get_vertex_buffer (SehleEngine *engine, const char *id, unsigned int usage);
SehleIndexBuffer *sehle_engine_get_index_buffer (SehleEngine *engine, const char *id, unsigned int usage);
SehleShader *sehle_engine_get_shader (SehleEngine *engine, const char *id, unsigned int shader_type);
SehleProgram *sehle_engine_get_program (SehleEngine *engine, const char *id, unsigned int n_vertex_shaders, unsigned int n_fragment_shaders, unsigned int n_uniformss);

/* Registers resource to engine, does NOT aquire new reference */
void sehle_engine_add_resource (SehleEngine *engine, SehleResource *res, unsigned int replace);
/* Unregister resource, does NOT drop reference */
void sehle_engine_remove_resource (SehleEngine *engine, SehleResource *res);

SehleResource *sehle_engine_lookup_resource (SehleEngine *engine, unsigned int type, const unsigned char *id);

#ifdef SEHLE_PERFORMANCE_MONITOR
void sehle_engine_clear_counter (SehleEngine *engine);
#endif

#ifdef __cplusplus
};
#endif

#endif

