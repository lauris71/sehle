#define __SEHLE_STATIC_MESH_C__

/*
 * Libsehle
 *
 * Copyright (C) Lauris Kaplinski 2007-2021
 */

#include <stdlib.h>
#include <stdio.h>

#include "GL/glew.h"

#include <az/class.h>

#include <sehle/vertex-buffer.h>
#include <sehle/index-buffer.h>
#include <sehle/program.h>
#include <sehle/render-context.h>
#include <sehle/material.h>
#include <sehle/vertex-array.h>

#include "static-mesh.h"

static void static_mesh_class_init (SehleStaticMeshClass *klass);
static void static_mesh_init (SehleStaticMeshClass *klass, SehleStaticMesh *mesh);
static void static_mesh_finalize (SehleStaticMeshClass *klass, SehleStaticMesh *mesh);
/* Renderable implementation */
static void static_mesh_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx);
static void static_mesh_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data);

unsigned int sehle_static_mesh_type = 0;
SehleStaticMeshClass *sehle_static_mesh_class;

unsigned int
sehle_static_mesh_get_type (void)
{
	if (!sehle_static_mesh_type) {
		az_register_type (&sehle_static_mesh_type, (const unsigned char *) "SehleStaticMesh", AZ_TYPE_BLOCK, sizeof (SehleStaticMeshClass), sizeof (SehleStaticMesh), AZ_FLAG_ZERO_MEMORY,
			(void (*) (AZClass *)) static_mesh_class_init,
			(void (*) (const AZImplementation *, void *)) static_mesh_init,
			(void (*) (const AZImplementation *, void *)) static_mesh_finalize);
		sehle_static_mesh_class = (SehleStaticMeshClass *) az_type_get_class (sehle_static_mesh_type);
	}
	return sehle_static_mesh_type;
}

static void
static_mesh_class_init (SehleStaticMeshClass *klass)
{
	az_class_set_num_interfaces ((AZClass *) klass, 1);
	az_class_declare_interface ((AZClass *) klass, 0, SEHLE_TYPE_RENDERABLE,
		ARIKKEI_OFFSET(SehleStaticMeshClass, renderable_impl),
		ARIKKEI_OFFSET(SehleStaticMesh, renderable_inst));
	klass->renderable_impl.display = static_mesh_display;
	klass->renderable_impl.render = static_mesh_render;
}

static void
static_mesh_init (SehleStaticMeshClass *klass, SehleStaticMesh *mesh)
{
	mesh->r2w = EleaMat3x4fIdentity;
	mesh->ninstances = 1;
	mesh->nlods = 1;
}

static void
static_mesh_finalize (SehleStaticMeshClass *klass, SehleStaticMesh *mesh)
{
	if (mesh->va) {
		az_object_unref ((AZObject *) mesh->va);
	}
	if (mesh->instances) free (mesh->instances);
	if (mesh->frags) free (mesh->frags);
	if (mesh->materials) free (mesh->materials);
	if (mesh->sortedindices) free (mesh->sortedindices);
}

static void
static_mesh_display (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleDisplayContext *displayctx)
{
	SehleStaticMesh *mesh = SEHLE_STATIC_MESH_FROM_RENDERABLE_INSTANCE (inst);
	if (!mesh->va) return;
	unsigned int lod = 1;
	float width = (float) (ctx->render_state.viewport.x1 - ctx->render_state.viewport.x0);
	EleaVec3f d, e;
	elea_mat3x4f_get_translation (&d, &mesh->r2w);
	elea_mat3x4f_get_translation (&e, &ctx->v2w);
	d = elea_vec3f_sub (d, e);
	float distance = elea_vec3f_len(d);
	float ppu = ctx->lodBias * sehle_render_context_get_ppu (ctx, distance);
	for (unsigned int i = 0; i < 7; i++) {
		if (ppu >= mesh->lods[i]) break;
		lod <<= 1;
	}
	for (unsigned int i = 0; i < mesh->nfrags; i++) {
		if (!(mesh->frags[i].lod & lod)) continue;
		if (!mesh->frags[i].n_indices) return;
		SehleMaterialImplementation *mat_impl = mesh->materials[mesh->frags[i].mat_idx].impl;
		SehleMaterialInstance *mat_inst = mesh->materials[mesh->frags[i].mat_idx].inst;
		if (!mat_impl) continue;
		unsigned int fragstage = displayctx->stages & mat_inst->render_stages;
		if (!fragstage) continue;
		if (fragstage & displayctx->stages) {
			// Request rendering
			if (mat_inst->properties & SEHLE_MATERIAL_SORT) {
				sehle_render_context_schedule_render_sorted_triangles (ctx,
					impl, inst, &mesh->frags[i],
					mesh->va,
					mesh->frags[i].first, mesh->frags[i].n_indices, mat_impl, mat_inst, &mesh->r2w);
			} else {
				if (mesh->ninstances == 1) {
					sehle_render_context_schedule_render (ctx, impl, inst,
						mat_impl, mat_inst, &mesh->frags[i]);
				} else {
					sehle_render_context_schedule_render (ctx, impl, inst,
						mat_impl, mat_inst, &mesh->frags[i]);
				}
			}
		}
		if (fragstage & SEHLE_STAGE_REFLECTIONS) {
			// Ask for reflection images
			if (mat_inst->properties & SEHLE_MATERIAL_REFLECTION) {
				// Schedule reflection pass
				EleaVec3f p, d;
				elea_mat3x4f_get_translation (&p, &mesh->r2w);
				elea_mat3x4f_get_col_vec (&d, &mesh->r2w, 2);
				EleaPlane3f rplane;
				elea_plane3fp_set_pn (&rplane, &p, &d);
				sehle_render_context_add_reflection (ctx, &rplane, mesh->va, mesh->frags[i].first, mesh->frags[i].n_indices, &mesh->r2w,
					(SehleMaterialReflectingImplementation *) mat_impl, (SehleMaterialReflectingInstance *) mat_inst);
			}
		}
	}
}

static void
static_mesh_render (SehleRenderableImplementation *impl, SehleRenderableInstance *inst, SehleRenderContext *ctx, SehleProgram *prog, unsigned int render_type, void *data)
{
	SehleStaticMesh *mesh = SEHLE_STATIC_MESH_FROM_RENDERABLE_INSTANCE (inst);
	SehleStaticMeshFragment *frag = (SehleStaticMeshFragment *) data;
	SEHLE_CHECK_ERRORS (0);
	EleaMat3x4f o2v;
	elea_mat3x4f_multiply (&o2v, &ctx->w2v, &mesh->r2w);
	sehle_program_setUniformMatrix4fv (prog, SEHLE_UNIFORM_PROJECTION, 1, ctx->proj.c);
	EleaMat4x4f o2v_proj;
	elea_mat4x4f_multiply_mat3x4 (&o2v_proj, &ctx->proj, &o2v);
	sehle_program_setUniformMatrix4fv (prog, SEHLE_UNIFORM_O2V_PROJECTION, 1, o2v_proj.c);
	SEHLE_CHECK_ERRORS (0);
	sehle_program_setUniformMatrix4x3fv (prog, SEHLE_UNIFORM_O2V, 1, 1, o2v.c);

	sehle_vertex_array_render_triangles (mesh->va, 1, frag->first, frag->n_indices);
}

void
sehle_static_mesh_clear (SehleStaticMesh *mesh)
{
	if (mesh->va) {
		az_object_unref ((AZObject *) mesh->va);
		mesh->va = NULL;
	}
	sehle_static_mesh_resize_materials (mesh, 0);
	sehle_static_mesh_resize_fragments (mesh, 0);
	sehle_static_mesh_resize_instances (mesh, 0);
}

void
sehle_static_mesh_set_vertex_array (SehleStaticMesh *mesh, SehleVertexArray *va)
{
	if (mesh->va) az_object_unref ((AZObject *) mesh->va);
	mesh->va = va;
	if (mesh->va) az_object_ref ((AZObject *) mesh->va);
}

void
sehle_static_mesh_set_vertex_data (SehleStaticMesh* mesh, SehleVertexBuffer* vbuf)
{
	if (!mesh->va) mesh->va = sehle_vertex_array_new (vbuf->buffer.resource.engine, NULL);
	for (unsigned int i = 0; i < SEHLE_NUM_VERTEX_BINDINGS; i++) {
		if (vbuf->sizes[i]) {
			sehle_vertex_array_set_vertex_data (mesh->va, i, vbuf);
		}
	}
}

void
sehle_static_mesh_set_index_data (SehleStaticMesh *mesh, SehleIndexBuffer *ibuf)
{
	if (!mesh->va) mesh->va = sehle_vertex_array_new (ibuf->buffer.resource.engine, NULL);
	sehle_vertex_array_set_index_data (mesh->va, ibuf);
}

void
sehle_static_mesh_resize_materials (SehleStaticMesh *mesh, unsigned int nmaterials)
{
	mesh->materials = (SehleMaterialHandle *) realloc (mesh->materials, nmaterials * sizeof (SehleMaterialHandle));
	/* Clear new pointers */
	for (unsigned int i = mesh->nmaterials; i < nmaterials; i++) {
		mesh->materials[i].impl = NULL;
		mesh->materials[i].inst = NULL;
	}
	mesh->nmaterials = nmaterials;
}

void
sehle_static_mesh_set_material (SehleStaticMesh *mesh, unsigned int idx, SehleMaterialImplementation *impl, SehleMaterialInstance *inst)
{
	mesh->materials[idx].impl = impl;
	mesh->materials[idx].inst = inst;
}

void
sehle_static_mesh_resize_fragments (SehleStaticMesh *mesh, unsigned int length)
{
	mesh->frags = (SehleStaticMeshFragment *) realloc (mesh->frags, length * sizeof (SehleStaticMeshFragment));
	if (length > mesh->nfrags) {
		memset (mesh->frags + mesh->nfrags, 0, (length - mesh->nfrags) * sizeof (SehleStaticMeshFragment));
		for (unsigned int i = mesh->nfrags; i < length; i++) mesh->frags[i].lod = 1;
	}
	mesh->nfrags = length;
}

void
sehle_static_mesh_resize_instances (SehleStaticMesh *mesh, unsigned int ninstances)
{
	if (ninstances < 2) {
		mesh->ninstances = 1;
		if (mesh->instances) {
			free (mesh->instances);
			mesh->instances = NULL;
		}
	} else {
		mesh->ninstances = ninstances;
		mesh->instances = (EleaMat3x4f *) realloc (mesh->instances, ninstances * sizeof (EleaMat3x4f));
	}
}

void
sehle_static_mesh_setup (SehleStaticMesh *mesh, SehleEngine *engine, unsigned int layer_mask)
{
	mesh->renderable_inst.layer_mask = layer_mask;
}

void
sehle_static_mesh_release (SehleStaticMesh *mesh)
{
	sehle_static_mesh_clear (mesh);
}

SehleStaticMesh *
sehle_static_mesh_new (SehleEngine *engine, unsigned int layer_mask)
{
	SehleStaticMesh *mesh = (SehleStaticMesh *) az_instance_new (SEHLE_TYPE_STATIC_MESH);
	sehle_static_mesh_setup (mesh, engine, layer_mask);
	return mesh;
}

void
sehle_static_mesh_delete (SehleStaticMesh *mesh)
{
	az_instance_delete (SEHLE_TYPE_STATIC_MESH, mesh);
}

