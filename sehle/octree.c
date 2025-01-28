#define __SEHLE_OCTREE_C__

//
// Libsehle
//
// Copyright (C) Lauris Kaplinski 2011
//

#include <stdlib.h>
#include <stdio.h>

#include "GL/glew.h"

#include <az/class.h>

#include <sehle/program.h>
#include <sehle/render-context.h>
#include <sehle/material.h>
#include "vertex-buffer.h"
#include "index-buffer.h"

#include <sehle/octree.h>

typedef struct _SehleOctreeFragment SehleOctreeFragment;

struct _SehleOctreeFragment {
	SehleOctreeFragment *next;
	unsigned int matidx;
	unsigned int first;
	unsigned int nindices;
	EleaAABox3f bbox;
	// Data for sorted rendering
	uint32_t *sortedindices;
};

SehleOctreeFragment *
octree_fragment_new (void)
{
	SehleOctreeFragment *frag = (SehleOctreeFragment *) malloc (sizeof (SehleOctreeFragment));
	memset (frag, 0, sizeof (SehleOctreeFragment));
	return frag;
}

static void
octree_fragment_delete (SehleOctreeFragment *frag)
{
	if (frag->sortedindices) free (frag->sortedindices);
	free (frag);
}

static void
octree_fragment_display (SehleOctreeFragment *frag, SehleRenderContext *ctx, unsigned int stages, SehleStaticOctree *tree)
{
	SehleMaterialImplementation *impl = tree->materials[frag->matidx].impl;
	SehleMaterialInstance *inst = tree->materials[frag->matidx].inst;
	if (!inst) return;
	if (!frag->nindices) return;
	unsigned int fragstage = stages & inst->render_stages;
	if (!fragstage) return;
	if (fragstage & SEHLE_STAGE_SOLID) {
		sehle_render_context_schedule_render (ctx, SEHLE_STATIC_OCTREE_RENDERABLE_IMPLEMENTATION, &tree->renderable_inst,
			impl, inst, frag);
	}
	if (fragstage & SEHLE_STAGE_LIGHTS) {
		sehle_render_context_schedule_render (ctx, SEHLE_STATIC_OCTREE_RENDERABLE_IMPLEMENTATION, &tree->renderable_inst,
			impl, inst, frag);
	}
	if (fragstage & SEHLE_STAGE_TRANSPARENT) {
		// Sort and distance buffers are needed
		// fixme: Have to save bbox midpoint for frag sorting
		// fixme: Ibuffer should be mappend on-demand
		sehle_render_context_schedule_render_sorted_triangles (ctx,
			SEHLE_STATIC_OCTREE_RENDERABLE_IMPLEMENTATION, &tree->renderable_inst, frag,
			tree->va, frag->first, frag->nindices, impl, inst, &tree->r2w);
	}
	if (fragstage & SEHLE_STAGE_FORWARD) {
		sehle_render_context_schedule_render (ctx,
			SEHLE_STATIC_OCTREE_RENDERABLE_IMPLEMENTATION, &tree->renderable_inst,
			impl, inst, frag);
	}
}

static void
octree_fragment_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data)
{
	SehleStaticOctree *octree = SEHLE_STATIC_OCTREE_FROM_RENDERABLE_INSTANCE(inst);
	SehleOctreeFragment *frag = (SehleOctreeFragment *) data;
	SEHLE_CHECK_ERRORS (0);
	EleaMat3x4f o2v;
	elea_mat3x4f_multiply (&o2v, &ctx->w2v, &octree->r2w);
	sehle_program_setUniformMatrix4fv (prog, SEHLE_UNIFORM_PROJECTION, 1, ctx->proj.c);
	EleaMat4x4f o2v_proj;
	elea_mat4x4f_multiply_mat3x4 (&o2v_proj, &ctx->proj, &o2v);
	sehle_program_setUniformMatrix4fv (prog, SEHLE_UNIFORM_O2V_PROJECTION, 1, o2v_proj.c);
	SEHLE_CHECK_ERRORS (0);
	sehle_program_setUniformMatrix4x3fv (prog, SEHLE_UNIFORM_O2V, 1, 1, o2v.c);
	SEHLE_CHECK_ERRORS (0);
	sehle_vertex_array_render_triangles (octree->va, 1, frag->first, frag->nindices);
}

struct _SehleStaticOctreeNode {
	unsigned int level;
	EleaAABox3f partition;
	EleaAABox3f bbox;
	EleaVec3f splitPoint;
	SehleStaticOctreeNode *children[8];
	unsigned int nfrags;
	SehleOctreeFragment *frags;
	unsigned int nguests;
	SehleOctreeFragment *guests;
};

static SehleStaticOctreeNode *
octree_node_new (unsigned int level, const EleaAABox3f *partition)
{
	SehleStaticOctreeNode *node = (SehleStaticOctreeNode *) malloc (sizeof (SehleStaticOctreeNode));
	memset (node, 0, sizeof (SehleStaticOctreeNode));
	node->level = level;
	node->partition = *partition;
	node->bbox = EleaAABox3fEmpty;
	node->splitPoint = elea_vec3f_add (partition->min, partition->max);
	node->splitPoint = elea_vec3f_div (node->splitPoint, 2);
	return node;
}

static void
octree_node_delete (SehleStaticOctreeNode *node)
{
	for (int i = 0; i < 8; i++) if (node->children[i]) octree_node_delete (node->children[i]);
	while (node->frags) {
		SehleOctreeFragment *child = node->frags;
		node->frags = node->frags->next;
		octree_fragment_delete (child);
	}
	while (node->guests) {
		SehleOctreeFragment *child = node->guests;
		node->guests = node->guests->next;
		octree_fragment_delete (child);
	}
}

void
octree_node_display (SehleStaticOctreeNode *node, SehleRenderContext *ctx, unsigned int stages, const EleaPolyhedron3f *viewspace, SehleStaticOctree *tree)
{
	if (elea_polyhedron3f_test_aabox (viewspace, &node->bbox) == ELEA_POSITION_OUT) return;
	for (int i = 0; i < 8; i++) {
		if (node->children[i]) octree_node_display (node->children[i], ctx, stages, viewspace, tree);
	}
	for (SehleOctreeFragment *f = node->frags; f; f = f->next) octree_fragment_display (f, ctx, stages, tree);
	for (SehleOctreeFragment *f = node->guests; f; f = f->next) octree_fragment_display (f, ctx, stages, tree);
}

void
octree_node_insert_fragment (SehleStaticOctreeNode *node, SehleOctreeFragment *frag)
{
	int x = -1;
	int y = -1;
	int z = -1;
	if (frag->bbox.max.x <= node->splitPoint.x) {
		x = 0;
	} else if (frag->bbox.min.x >= node->splitPoint.x) {
		x = 1;
	}
	if (x >= 0) {
		if (frag->bbox.max.y <= node->splitPoint.y) {
			y = 0;
		} else if (frag->bbox.min.y >= node->splitPoint.y) {
			y = 1;
		}
	}
	if (y >= 0) {
		if (frag->bbox.max.z <= node->splitPoint.z) {
			z = 0;
		} else if (frag->bbox.min.z >= node->splitPoint.z) {
			z = 1;
		}
	}
	if ((z >= 0) && (node->level < 10)) {
		int pos = (z << 2) | (y << 1) | x;
		if (!node->children[pos]) {
			EleaAABox3f cpartition = node->partition;
			if (x == 0) {
				cpartition.c[3] = node->splitPoint.c[0];
			} else {
				cpartition.c[0] = node->splitPoint.c[0];
			}
			if (y == 0) {
				cpartition.c[4] = node->splitPoint.c[1];
			} else {
				cpartition.c[1] = node->splitPoint.c[1];
			}
			if (z == 0) {
				cpartition.c[5] = node->splitPoint.c[2];
			} else {
				cpartition.c[2] = node->splitPoint.c[2];
			}
			node->children[pos] = octree_node_new (node->level + 1, &cpartition);
		}
		octree_node_insert_fragment (node->children[pos], frag);
	} else {
		frag->next = node->frags;
		node->frags = frag;
		node->nfrags += 1;
	}
	elea_aabox3f_union (&node->bbox, &node->bbox, &frag->bbox);
}

void static_octree_class_init (SehleStaticOctreeClass *klass);
void static_octree_finalize (SehleStaticOctreeClass *klass, SehleStaticOctree *octree);

/* Renderable implementation */
static void static_octree_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx);

unsigned int sehle_static_octree_type = 0;
SehleStaticOctreeClass *sehle_static_octree_class = NULL;

unsigned int
sehle_static_octree_get_type ()
{
	if (!sehle_static_octree_type) {
		sehle_static_octree_class = (SehleStaticOctreeClass *) az_register_type (&sehle_static_octree_type, (const unsigned char *) "SehleStaticOctree", AZ_TYPE_STRUCT, sizeof (SehleStaticOctreeClass), sizeof (SehleStaticOctree), AZ_CLASS_ZERO_MEMORY,
			(void (*) (AZClass *)) static_octree_class_init,
			NULL,
			(void (*) (const AZImplementation *, void *)) static_octree_finalize);
	}
	return sehle_static_octree_type;
}

void
static_octree_class_init (SehleStaticOctreeClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_RENDERABLE,
		ARIKKEI_OFFSET (SehleStaticOctreeClass, renderable_impl),
		ARIKKEI_OFFSET (SehleStaticOctree, renderable_inst));
	klass->renderable_impl.display = static_octree_display;
}

void
static_octree_finalize (SehleStaticOctreeClass *klass, SehleStaticOctree *octree)
{
	octree_node_delete (octree->root);
	if (octree->materials) free (octree->materials);
	if (octree->ibuffer) az_object_unref (AZ_OBJECT(octree->ibuffer));
	if (octree->vbuffer) az_object_unref (AZ_OBJECT (octree->vbuffer));
}

static void
static_octree_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx)
{
	SehleStaticOctree *octree = (SehleStaticOctree *) inst;
	EleaMat3x4f w2o;
	elea_mat3x4f_invert_normalized (&w2o, &octree->r2w);
	EleaPolyhedron3f vsobject;
	vsobject.n = ctx->vspace.n;
	for (unsigned int i = 0; i < ctx->vspace.n; i++) {
		elea_mat3x4f_transform_plane (&vsobject.p[i], &w2o, &ctx->vspace.p[i]);
	}
	octree_node_display (octree->root, ctx, displayctx->stages, &vsobject, octree);
}

SehleStaticOctree *
sehle_static_octree_new (SehleEngine *engine, unsigned int layer_mask, const EleaAABox3f *bbox_object)
{
	SehleStaticOctree *octree = (SehleStaticOctree *) az_instance_new (SEHLE_TYPE_STATIC_OCTREE);
	octree->renderable_inst.layer_mask = layer_mask;
	octree->root = octree_node_new (0, bbox_object);
	return octree;
}

void
sehle_static_octree_delete (SehleStaticOctree *octree)
{
	az_instance_delete (SEHLE_TYPE_STATIC_OCTREE, octree);
}

void
sehle_static_octree_set_r2w (SehleStaticOctree *octree, const EleaMat3x4f *r2w)
{
	octree->r2w = *r2w;
}

void
sehle_static_octree_set_material (SehleStaticOctree *octree, unsigned int  idx, SehleMaterialImplementation *mat_impl, SehleMaterialInstance *mat_inst)
{
	octree->materials[idx].impl = mat_impl;
	octree->materials[idx].inst = mat_inst;
}

void
sehle_static_octree_resize_materials (SehleStaticOctree *octree, unsigned int length)
{
	octree->materials = (SehleMaterialHandle *) realloc (octree->materials, length * sizeof (SehleMaterialHandle));
	// Clear new pointers
	for (unsigned int i = octree->nmaterials; i < length; i++) {
		octree->materials[i].impl = NULL;
		octree->materials[i].inst = NULL;
	}
	octree->nmaterials = length;
}

void
sehle_static_octree_clear (SehleStaticOctree *octree, const EleaAABox3f *bbox_object)
{
	octree_node_delete (octree->root);
	sehle_static_octree_resize_materials (octree, 0);
	octree->root = octree_node_new (0, bbox_object);
}

void
sehle_static_octree_add_fragment (SehleStaticOctree *octree, unsigned int matidx, unsigned int first, unsigned int nindices, const EleaAABox3f *bbox_object)
{
	SehleOctreeFragment *frag = octree_fragment_new ();
	frag->next = NULL;
	frag->matidx = matidx;
	frag->first = first;
	frag->nindices = nindices;
	frag->bbox = *bbox_object;
	frag->sortedindices = NULL;
	octree_node_insert_fragment (octree->root, frag);
}

