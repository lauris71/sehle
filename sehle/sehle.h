#ifndef __SEHLE_H__
#define __SEHLE_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2007-2015
//

/*
 * Predefined display stages
 *
 * Used to notify renderables in display phase about the intended rendering operation(s)
 * Multiple stages may be specified, renderables can submit for these as one or multiple entries
 * Both renderables and materials can specify stage mask for quick culling of non-applicable stages
 */

/*
 * STAGE_SOLID - opaque geometry
 * "Normal" opaque geometry, intended render types are RENDER_GBUF and RENDER_DEPTH (both global depth and shadow)
 */
#define SEHLE_STAGE_SOLID 1

 /*
 * STAGE_TRANSPARENT - transparent geometry
 * Semitransparent geometry, particles etc., intended render types RENDER_TRANSPARENT and RENDER_DENSITY
 * Lights should add themselves to the light list for forward lighting
 */
#define SEHLE_STAGE_TRANSPARENT 2

 /* STAGE_AMBIENT
*   Notify occluding/transmitting objects that they should request rendering into ambient buffer
*   Intended render type is RENDER_AMBIENT
*/
#define SEHLE_STAGE_AMBIENT 4

/*
* STAGE_LIGHTS - lights and lightmaps
*   Notify light-emitting or lightmap objects to request rendering of light geometry
*   Intended render type is RENDER_STENCIL and RENDER_LIGHTMAP
*   fixme: Currently there is non-stencil RENDER_LIGHTMAP also
*/
#define SEHLE_STAGE_LIGHTS 8

/*
 * STAGE_REFLECTIONS - Notify reflective materials that they have to request the rendering of reflection images
 *                     NOT that they should request rendering themselves (this should happen in some other stage)
 */
#define SEHLE_STAGE_REFLECTIONS 16

 /*
 * STAGE_FORWARD - gizmos, controls...
 * Intended render types RENDER_DEPTH and RENDER_FORWARD
 */
#define SEHLE_STAGE_FORWARD 32

#define SEHLE_RENDER_STAGES_ALL 0x3f;

/*
 * Predefined render types
 *
 * Used to tell multi-program materials about the role and setup of the current pass
 * Only single type can be rendered in one pass
 */

/*
 * RENDER_DEPTH - write depth to framebuffer (color may or may not be written, should be disabled by caller)
 */
#define SEHLE_RENDER_DEPTH 1

/*
 * RENDER_DENSITY - compose transparent objects into framebuffer (with additive blending) forming the density for crossing light (depth may or may not be written)
 */
#define SEHLE_RENDER_DENSITY 2

/*
 * RENDER_TRANSPARENT - write color and depth to framebuffer (with standard blending) using up to 4 lights + shadow + density textures
 */
#define SEHLE_RENDER_TRANSPARENT 4

/*
 * RENDER_GBUFFER - write depth, albedo, normal and specular textures to special framebufer
 */
#define SEHLE_RENDER_GBUFFER 8

/*
 * RENDER_FORWARD - write color and/or depth to framebuffer (with standard blending) using up to 4 lights and special scheme
 */
#define SEHLE_RENDER_FORWARD 16

/*
 * RENDER_STENCIL - render stencil stage of light volume material
 */
#define SEHLE_RENDER_STENCIL 32

/*
 * RENDER_AMBIENT - write ambient term to gbuffer using gbuffer depth, normal and albedo textures
 */
#define SEHLE_RENDER_AMBIENT 64

/*
 * RENDER_LIGHTMAP - modify framebuffer by lightmap value and ambient light (with additive blending)
 */
#define SEHLE_RENDER_LIGHTMAP 128

#define SEHLE_RENDER_TYPES_ALL 0xffffffff;

/* Standard vertex bindings */

#define SEHLE_NUM_VERTEX_BINDINGS 8

#define SEHLE_ATTRIBUTE_VERTEX 0

#define SEHLE_ATTRIBUTE_NORMAL 1

#define SEHLE_ATTRIBUTE_TEXCOORD 2
#define SEHLE_ATTRIBUTE_TEXCOORD0 2

#define SEHLE_ATTRIBUTE_TEXCOORD1 3

#define SEHLE_ATTRIBUTE_COLOR 4

#define SEHLE_ATTRIBUTE_TANGENT 5
#define SEHLE_ATTRIBUTE_PROPERTIES 5
#define SEHLE_ATTRIBUTE_PROPERTIES0 5
#define SEHLE_ATTRIBUTE_TEXCOORD2 5

#define SEHLE_ATTRIBUTE_BONES 6
#define SEHLE_ATTRIBUTE_TEXCOORD3 6
#define SEHLE_ATTRIBUTE_PROPERTIES1 6

#define SEHLE_ATTRIBUTE_WEIGHTS 7
#define SEHLE_ATTRIBUTE_PROPERTIES2 7

#define SEHLE_NUM_ATTRIBUTE_NAMES 8

/* System */
typedef struct _SehleEngine SehleEngine;
/* Objects */
typedef struct _SehleDirectionalLightInstance SehleDirectionalLightInstance;
typedef struct _SehleGraph SehleGraph;
typedef struct _SehleIndexBuffer SehleIndexBuffer;
typedef struct _SehleLightInstance SehleLightInstance;
typedef struct _SehleStaticMesh SehleStaticMesh;
typedef struct _SehlePointLightInstance SehlePointLightInstance;
typedef struct _SehleProgram SehleProgram;
typedef struct _SehleRenderableInstance SehleRenderableInstance;
typedef struct _SehleRenderableImplementation SehleRenderableImplementation;
typedef struct _SehleRenderState SehleRenderState;
typedef struct _SehleRenderTarget SehleRenderTarget;
typedef struct _SehleResource SehleResource;
typedef struct _SehleResourceManager SehleResourceManager;
typedef struct _SehleShader SehleShader;
typedef struct _SehleSpotLightInstance SehleSpotLightInstance;
typedef struct _SehleTexture SehleTexture;
typedef struct _SehleTexture2D SehleTexture2D;
typedef struct _SehleTextureCubeMap SehleTextureCubeMap;
typedef struct _SehleVertexArray SehleVertexArray;
typedef struct _SehleVertexBuffer SehleVertexBuffer;
/* Renderables */
typedef struct _SehleRenderableList SehleRenderableList;
/* Renderers */
typedef struct _SehleSceneRenderer SehleSceneRenderer;
typedef struct _SehleSMAARenderer SehleSMAARenderer;
typedef struct _SehleUIRenderer SehleUIRenderer;
/* Materials */
typedef struct _SehleMaterialImplementation SehleMaterialImplementation;
typedef struct _SehleMaterialInstance SehleMaterialInstance;
typedef struct _SehleMaterialReflectingImplementation SehleMaterialReflectingImplementation;
typedef struct _SehleMaterialReflectingInstance SehleMaterialReflectingInstance;
typedef struct _SehleMaterialDNS SehlematerialDNS;
typedef struct _SehleMaterialControl SehleMaterialControl;
/* Render context */
typedef struct _SehleDisplayContext SehleDisplayContext;
typedef struct _SehleRenderContext SehleRenderContext;

typedef struct _SehleMaterialHandle SehleMaterialHandle;

struct _SehleMaterialHandle {
	SehleMaterialImplementation *impl;
	SehleMaterialInstance *inst;
};

typedef struct _SehleRenderableHandle SehleRenderableHandle;

struct _SehleRenderableHandle {
	SehleRenderableImplementation *impl;
	SehleRenderableInstance *inst;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Engine lifecycle */
SehleEngine *sehle_engine_new (void);
void sehle_engine_delete (SehleEngine *engine);

#ifndef SEHLE_SILENCE_ERRORS
#define SEHLE_CHECK_ERRORS(s) sehle_check_error (s, __FILE__, __LINE__)
#else
#define SEHLE_CHECK_ERRORS(s)
#endif

void sehle_check_error (unsigned int silent, const char *file, unsigned int line);
const char *sehle_get_error_string (unsigned int gl_error);
const char *sehle_describe_framebuffer_status (unsigned int status);

#ifdef __cplusplus
};
#endif

#endif

