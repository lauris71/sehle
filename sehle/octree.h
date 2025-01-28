#ifndef __SEHLE_OCTREE_H__
#define __SEHLE_OCTREE_H__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2011-2014
//

//
// Octree implementation
//
// Client is responsible of calculating all bounding boxes
// Bounding boxes have to be specified in mesh coordinate system
//

typedef struct _SehleStaticOctree SehleStaticOctree;
typedef struct _SehleStaticOctreeClass SehleStaticOctreeClass;
typedef struct _SehleStaticOctreeNode SehleStaticOctreeNode;

#define SEHLE_TYPE_STATIC_OCTREE (sehle_static_octree_get_type ())

#define SEHLE_STATIC_OCTREE_FROM_RENDERABLE_INSTANCE(i) (SehleStaticOctree *) ARIKKEI_BASE_ADDRESS (SehleStaticOctree, renderable_inst, i)
#define SEHLE_STATIC_OCTREE_RENDERABLE_IMPLEMENTATION (&sehle_static_octree_class->renderable_impl)

#include <elea/matrix3x4.h>
#include <sehle/renderable.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SehleStaticOctree {
	SehleRenderable renderable_inst;

	EleaMat3x4f r2w;

	SehleVertexArray* va;
	SehleVertexBuffer *vbuffer;
	SehleIndexBuffer *ibuffer;
	uint32_t nmaterials;
	SehleMaterialHandle *materials;
	SehleStaticOctreeNode *root;
};

struct _SehleStaticOctreeClass {
	SehleRenderableClass renderable_klass;
	SehleRenderableImplementation renderable_impl;
};

unsigned int sehle_static_octree_get_type (void);

#ifndef __SEHLE_STATIC_MESH_C__
extern unsigned int sehle_static_octree_type;
extern SehleStaticOctreeClass *sehle_static_octree_class;
#endif

SehleStaticOctree *sehle_static_octree_new (SehleEngine *engine, unsigned int layer_mask, const EleaAABox3f *bbox_object);
void sehle_static_octree_delete (SehleStaticOctree *octree);

void sehle_static_octree_set_r2w (SehleStaticOctree *octree, const EleaMat3x4f *r2w);

#ifdef __cplusplus
};
#endif

#endif

